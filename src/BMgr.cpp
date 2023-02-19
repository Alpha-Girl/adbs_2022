#include "../include/BMgr.h"
#include <string.h>

bFrame buf[DEFBUFSIZE];  // store the frames
BCB buf_bcb[DEFBUFSIZE]; // store the bcbs

unsigned int _hash(unsigned int x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

BMgr::BMgr(std::string filename, int alg)
{
    dsmgr = new DSMgr(filename);

    init_bcb();                                  // init bcb array
    init_free_list();                            // add all frames to free list
    memset(ptof, 0, DEFBUFSIZE * sizeof(BCB *)); // init hash table to be empty

    // choose replace algorithm
    switch (alg)
    {
    case LRU:
        replace_alg = &LRU_replace_alg;
        break;
    case Clock:
        replace_alg = &Clock_replace_alg;
        break;
    case Random:
        replace_alg = &Random_replace_alg;
        break;
    default:
        exit(1);
    }
    replace_alg->init();
    printf("Used Replace Alg: %s\n", replace_alg->name);

    init_statistical_data(); // init statistic
}

BMgr::~BMgr()
{
    delete dsmgr;
    clear_buffer();
}

/*
The prototype for this function is FixPage(Page_id, protection) and it returns a frame_id.
The file and access manager will call this page with the page_id that is in the record_id of
the record. The function looks to see if the page is in the buffer already and returns the
corresponding frame_id if it is. If the page is not resident in the buffer yet, it selects a
victim page, if needed, and loads in the requested page.
*/
int BMgr::FixPage(int page_id, int type)
{
    access_total++; // statistic

    // hit?
    BCB *bcb_ptr = hash_search(page_id);
    if (bcb_ptr != NULL)
    { // hit!
        hit++;
        if (type == 1)
        { // write
            /**
             * write to buffer
             */
            bcb_ptr->dirty = 1; // set dirty
            write++;
        }
        replace_alg->update(bcb_ptr, 0);
        return bcb_ptr->frame_id;
    }

    /*no hit
    1. check if there is free frame
        1. if no
            1. **select victim frame**
            2. writeback (when dirty)
    2. now we get a free frame(from free or victim)
        1. if request is write
            1. write to free frame
        2. if read
            1. call memory manger to read page from disk
            2. write to free frame
    3. update LRU(or other replace algrithm data struct)
    4. map page_id to frame(hash insert)
    */
    bcb_ptr = SelectVictim(); // check free and select victim

    if (type == 1)
    {
        /**
         * write to buffer
         */
        bcb_ptr->dirty = 1; // set dirty
        write++;
    }
    else
    {
        /**
         * call Disk manager to read page, and write to buffer
         * read(page_id, bcb[bcb_ptr->frame_id])
         */
        dsmgr->ReadPage(page_id, &buf[bcb_ptr->frame_id]);
        read_io++;
    }
    bcb_ptr->page_id = page_id;

    hash_insert(bcb_ptr);

    return bcb_ptr->frame_id;
}

/*
The prototype for this function is FixNewPage() and it returns a page_id and a frame_id.
This function is used when a new page is needed on an insert, index split, or object
creation. The page_id is returned in order to assign to the record_id and metadata. This
function will find an empty page that the File and Access Manager can use to store some
data.
*/
int BMgr::FixNewPage(int *page_id_ptr)
{
    BCB *bcb_ptr = SelectVictim();
    int page_id = dsmgr->NewPage();
    *page_id_ptr = page_id;

    bcb_ptr->page_id = page_id;
    hash_insert(bcb_ptr);
    bcb_ptr->count++;

    return bcb_ptr->frame_id;
}

/*
The prototype for this function is UnfixPage(page_id) and it returns a frame_id. This
function is the compliment to a FixPage or FixNewPage call. This function decrements
the fix count on the frame. If the count reduces to zero, then the latch on the page is
removed and the frame can be removed if selected. The page_id is translated to a
frame_id and it may be unlatched so that it can be chosen as a victim page if the count on
the page has been reduced to zero.
*/
int BMgr::UnfixPage(int page_id)
{
    BCB *bcb_ptr = hash_search(page_id);
    if (bcb_ptr == NULL)
    {
        fprintf(stderr, "ERROR: UnfixPage %d failed\n", page_id);
        exit(1);
    }
    if (bcb_ptr->count == 0)
    {
        hash_delete(bcb_ptr);
        release_free(bcb_ptr);
    }
    else
    {
        bcb_ptr->count--;
    }
    return bcb_ptr->frame_id;
}

/*
NumFreeFrames function looks at the buffer and returns the number of buffer pages
that are free and able to be used. This is especially useful for the N-way sort for the
query processor. The prototype for the function looks like NumFreeFrames() and returns
an integer from 0 to BUFFERSIZE-1(1023)*/
int BMgr::NumFreeFrames()
{
    return free_frame_num;
}

/*
Hash function takes the page_id as the parameter and  returns the frame id.
*/
int BMgr::Hash(int page_id)
{
    return page_id % DEFBUFSIZE;
}
void BMgr::clear_buffer()
{
    for (int i = 0; i < DEFBUFSIZE; i++)
    {
        if (buf_bcb[i].dirty == 1)
        {
            /**
             * call Disk manager to write back page
             * writeback(bcb_ptr->page_id, bcb[bcb_ptr->frame_id])
             */
            write_io++;
        }
    }
}

/*
SetDirty function sets the dirty bit for the frame_id.  This dirty bit is used to know
whether or not to write out the frame.  A frame must be written if the contents have been
modified in any way.  This includes any directory pages and data pages.  If the bit is 1, it
will be written.  If this bit is zero, it will not be written.
*/
void BMgr::SetDirty(int frame_id)
{
    buf_bcb[frame_id].dirty = 1;
}

/*
UnsetDirty function assigns the dirty_bit for the corresponding frame_id to zero.  The
main reason to call this function is when the setDirty() function has been called but the
page is actually part of a temporary relation.  In this case, the page will not actually need
to be written, because it will not want to be saved.
*/
void BMgr::UnsetDirty(int frame_id)
{
    buf_bcb[frame_id].dirty = 0;
}

/*
WriteDirtys function must be called when the system is shut down.  The purpose of
the function is to write out any pages that are still in the buffer that may need to be
written.  It will only write pages out to the file if the dirty_bit is one.
*/
void BMgr::WriteDirtys()
{
    for (int i = 0; i < DEFBUFSIZE; i++)
    {
        if (buf_bcb[i].dirty)
        {
            dsmgr->WritePage(buf_bcb[i].page_id, &buf[buf_bcb[i].frame_id]);
            write_io++;
            buf_bcb[i].dirty = 0;
        }
    }
}

/*
PrintFrame function prints out the contents of the frame described by the frame_id.
*/
void BMgr::PrintFrame(int frame_id)
{
    for (int i = 0; i < FRAMESIZE; i++)
    {
        std::cout << buf[frame_id].field[i];
    }
    std::cout << std::endl;
}

/*
SelectVictim function selects a frame to replace. If the dirty bit of the selected frame is
set then the page needs to be written on to the disk.
*/
BCB *BMgr::SelectVictim()
{
    // free frame?
    BCB *bcb_ptr;
    bcb_ptr = get_free();
    int is_free_frame = !(bcb_ptr == NULL);
    if (bcb_ptr == NULL)
    {                                           // no free frame
        bcb_ptr = replace_alg->select_victim(); // select victim
        if (bcb_ptr->dirty)
        { // check dirty
            /**
             * call Disk manager to write back page
             * writeback(bcb_ptr->page_id, bcb[bcb_ptr->frame_id])
             */
            dsmgr->WritePage(bcb_ptr->page_id, &buf[bcb_ptr->frame_id]);
            write_io++;
            bcb_ptr->dirty = 0;
        }
        hash_delete(bcb_ptr); // delete original page_id to frame_id hash map
    }
    replace_alg->update(bcb_ptr, is_free_frame);

    return bcb_ptr;
}

void BMgr::init_bcb()
{
    for (int i = 0; i < DEFBUFSIZE; i++)
    {
        buf_bcb[i].page_id = -1;
        buf_bcb[i].frame_id = i;
        buf_bcb[i].latch = 0;
        buf_bcb[i].count = 0;
        buf_bcb[i].dirty = 0;

        buf_bcb[i].next = NULL;
        buf_bcb[i].free_next = NULL;
        buf_bcb[i].LRU_prev = NULL;
        buf_bcb[i].LRU_next = NULL;
    }
}

void BMgr::init_free_list()
{
    free_frame_num = 0;
    free_list = NULL;
    for (int i = 0; i < DEFBUFSIZE; i++)
    {
        release_free(&buf_bcb[i]);
    }
}

BCB *BMgr::get_free()
{
    BCB *bcb_ptr = free_list;
    if (free_list == NULL)
    {
        return NULL;
    }
    else
    {
        free_list = free_list->free_next;
        free_frame_num--;
        return bcb_ptr;
    }
}

void BMgr::release_free(BCB *bcb_ptr)
{
    bcb_ptr->free_next = free_list;
    free_list = bcb_ptr;

    free_frame_num++;
}

BCB *BMgr::hash_search(int page_id)
{
    int index = _hash(page_id) % DEFBUFSIZE;
    BCB *bcb_ptr = ptof[index];
    while (bcb_ptr != NULL)
    { // chain size not limited
        if (bcb_ptr->page_id == page_id)
        {
            return bcb_ptr;
        }
        bcb_ptr = bcb_ptr->next;
    }
    return NULL;
}

void BMgr::hash_insert(BCB *bcb_ptr)
{
    int index = _hash(bcb_ptr->page_id) % DEFBUFSIZE;
    BCB *bcb_hash_ptr = ptof[index];
    bcb_ptr->next = bcb_hash_ptr;
    ptof[index] = bcb_ptr;
    ftop[bcb_ptr->frame_id] = bcb_ptr->page_id;
}

void BMgr::hash_delete(BCB *bcb_ptr)
{
    int index = _hash(bcb_ptr->page_id) % DEFBUFSIZE;
    BCB *bcb_hash_ptr = ptof[index];
    if (bcb_hash_ptr == bcb_ptr)
    {
        // ptof[index] = NULL;          //fix bug: first hit doesn't mean only one element
        ptof[index] = bcb_ptr->next;
    }
    else
    {
        while (bcb_hash_ptr->next != NULL)
        {
            if (bcb_hash_ptr->next == bcb_ptr)
            {
                bcb_hash_ptr->next = bcb_ptr->next;
                break;
            }
            bcb_hash_ptr = bcb_hash_ptr->next;
        }
    }
}

/*
初始化统计信息
*/
void BMgr::init_statistical_data()
{
    access_total = 0;
    hit = 0;
    write = 0;
    write_io = read_io = 0;
}

/*
测试结束后输出数据结果
*/
void BMgr::print_statistical_data()
{
    printf("Total: %d\n", access_total);
    printf("Total hit: %d\n", hit);
    printf("Hit rate: %.3f%%\n", (float)hit / access_total * 100);
    printf("Total write: %d\n", write);
    printf("write rate: %.3f%%\n", (float)write / access_total * 100);
    printf("read_io: %d\n", read_io);
    printf("write_io: %d\n", write_io);
    printf("total_io: %d\n", read_io + write_io);
}