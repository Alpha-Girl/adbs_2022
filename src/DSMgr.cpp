#include "../include/DSMgr.h"

DSMgr::DSMgr(){
    numPages=0;
}

DSMgr::DSMgr(std::string filename){
    OpenFile(filename);
}

DSMgr::~DSMgr(){
    CloseFile();
}

/*
OpenFile function is called anytime a file needs to be opened for reading or writing.
The prototype for this function is OpenFile(String filename) and returns an error code.
The function opens the file specified by the filename.
*/
int DSMgr::OpenFile(std::string filename){
    currFile = fopen(filename.c_str(), "r+");
    if(currFile==NULL){    //文件不存在，创建文件
        currFile = fopen(filename.c_str(), "w+");
    }
    fseek(currFile, 0L, SEEK_END);
    long size = ftell(currFile);       //获得文件大小
    numPages = size / FRAMESIZE;

    return currFile!=NULL; //返回是否打开文件成功
}

/*
CloseFile function is called when the data file needs to be closed. The protoype is
CloseFile() and returns an error code. This function closes the file that is in current use.
This function should only be called as the database is changed or a the program closes.
*/
int DSMgr::CloseFile(){
    return fclose(currFile);
}

/*
GetFile function returns the current file.
*/
FILE * DSMgr::GetFile(){
    return currFile;
}

/*
Seek function moves the file pointer.
*/
int DSMgr::Seek(int offset, int pos){
    return fseek(currFile, (long)offset, pos);
}


/*
ReadPage function is called by the FixPage function in the buffer manager. This
prototype is ReadPage(page_id, bytes) and returns what it has read in. This function calls
fseek() and fread() to gain data from a file.
*/
void DSMgr::ReadPage(int page_id, bFrame *frm){
    if(page_id > numPages){
        fprintf(stderr, "Error: DSMgr ReadPage cannot find page: %d\n", page_id);
        exit(1);
    }
    Seek(page_id * FRAMESIZE, SEEK_SET);
    fread((void *)frm, 1, FRAMESIZE, currFile);
}

/*
WritePage function is called whenever a page is taken out of the buffer. The
prototype is WritePage(frame_id, frm) and returns how many bytes were written. This
function calls fseek() and fwrite() to save data into a file.
*/
void DSMgr::WritePage(int page_id, bFrame *frm){
    if(page_id > numPages){
        fprintf(stderr, "Error: DSMgr WritePage cannot find page: %d\n", page_id);
        exit(1);
    }
    Seek(page_id * FRAMESIZE, SEEK_SET);
    fwrite((void *)frm, 1, FRAMESIZE, currFile);
}

/*
IncNumPages function increments the page counter.
*/
void DSMgr::IncNumPages(){
    numPages++;
}


/*
GetNumPages function returns the page counter.
*/
int DSMgr::GetNumPages(){
    return numPages;
}

int DSMgr::NewPage() {
    char buf[FRAMESIZE];    //随机初始化
    IncNumPages();
    fseek(currFile, 0L, SEEK_END);
    fwrite(buf, 1, FRAMESIZE, currFile);
    return GetNumPages()-1;
}

/*
SetUse function looks sets the bit in the pages array. This array keeps track of the
pages that are being used. If all records in a page are deleted, then that page is not really
used anymore and can be reused again in the database. In order to know if a page is
reusable, the array is checked for any use_bits that are set to zero. The fixNewPage
function firsts checks this array for a use_bit of zero. If one is found, the page is reused.
If not, a new page is allocated.
*/
void DSMgr::SetUse(int page_id, int use_bit){
    if(page_id > numPages){
        fprintf(stderr, "Error: DSMgr WritePage cannot find page: %d\n", page_id);
        exit(1);
    }
    pages[page_id]=use_bit;
}

/*
GetUse function returns the current use_bit for the corresponding page_id.
*/
int DSMgr::GetUse(int page_id){
    return pages[page_id];
}