// #pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#define FRAMESIZE 4096
#define MAXPAGES 4096
typedef struct bFrame
{
    char field[FRAMESIZE];
} bFrame;

#define DEFBUFSIZE 1024

extern bFrame buf[DEFBUFSIZE]; // or the size that the user defined by the input parameter

// Buffer Control Block
typedef struct BCB
{
    BCB();
    int page_id;
    int frame_id;
    int latch;
    int count;
    int dirty;
    struct BCB *next;
    struct BCB *free_next; // free list

    // Clock
    char clock_bit;

    // LRU
    struct BCB *LRU_prev;
    struct BCB *LRU_next;

} BCB;

BCB::BCB()
{
    page_id = -1;
    frame_id = -1;
    latch = 0;
    count = 0;
    dirty = 0;

    next = NULL;
    free_next = NULL;
    LRU_prev = NULL;
    LRU_next = NULL;
}
extern BCB buf_bcb[DEFBUFSIZE];
#endif