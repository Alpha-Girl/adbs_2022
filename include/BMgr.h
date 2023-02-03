#pragma once
#ifndef BMGR_H
#define BMGR_H

#include <iostream>

#include "setting.h"
#include "DSMgr.h"
class BMgr
{
public:
    BMgr();
    // Interface functions
    int FixPage(int page_id, int prot);
    void NewPage FixNewPage();
    int UnfixPage(int page_id);
    int NumFreeFrames();
    // Internal Functions
    int SelectVictim();
    int Hash(int page_id);
    void RemoveBCB(BCB *ptr, int page_id);
    void RemoveLRUEle(int frid);
    void SetDirty(int frame_id);
    void UnsetDirty(int frame_id);
    void WriteDirtys();
    PrintFrame(int frame_id);

private:
    // Hash Table
    int ftop[DEFBUFSIZE];
    BCB *ptof[DEFBUFSIZE];
};
#endif