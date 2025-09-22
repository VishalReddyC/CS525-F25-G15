/* buffer_mgr.c
 * Implementation of Buffer Manager (FIFO and LRU) for Assignment 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>  // for INT_MAX
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"
#include "dt.h"

/* bookkeeping structure stored in bm->mgmtData */
typedef struct FrameInfo {
    PageNumber pageNum;   // page number stored in this frame (NO_PAGE if empty)
    char *data;           // pointer to page data (PAGE_SIZE bytes)
    int fixCount;         // number of pins
    bool dirty;           // dirty flag
    int lastUsed;         // timestamp/counter for LRU
} FrameInfo;

typedef struct BM_MgmtData {
    FrameInfo *frames;    // array of frames (size numPages)
    int numPages;
    int nextFIFO;         // index for FIFO replacement pointer
    int timeCounter;      // incremented on each access to support LRU
    int numReadIO;        // stats
    int numWriteIO;       // stats
} BM_MgmtData;

/* Helper: get mgmt data */
static BM_MgmtData* getMgmt(BM_BufferPool *const bm) {
    return (BM_MgmtData *) bm->mgmtData;
}

/* Helper: open file handle for the pool's page file */
static RC openHandle(const char *fileName, SM_FileHandle *fHandle) {
    if (openPageFile((char *)fileName, fHandle) != RC_OK) {
        if (createPageFile((char *)fileName) != RC_OK) return RC_FILE_NOT_FOUND;
        if (openPageFile((char *)fileName, fHandle) != RC_OK) return RC_FILE_NOT_FOUND;
    }
    return RC_OK;
}

/* Helper: find a frame index holding pageNum, or -1 if not found */
static int findFrameIndex(BM_MgmtData *m, PageNumber pageNum) {
    for (int i = 0; i < m->numPages; i++) {
        if (m->frames[i].pageNum == pageNum) return i;
    }
    return -1;
}

/* Helper: pick victim frame index using FIFO or LRU strategy.
   Returns index on success, or -1 if no evictable frame found (all pinned). */
static int pickVictim(BM_BufferPool *const bm) {
    BM_MgmtData *m = getMgmt(bm);
    ReplacementStrategy strat = bm->strategy;

    // prefer empty frames first
    for (int i = 0; i < m->numPages; i++) {
        if (m->frames[i].pageNum == NO_PAGE) return i;
    }

    int victim = -1;

    if (strat == RS_FIFO) {
        for (int i = 0; i < m->numPages; i++) {
            int idx = (m->nextFIFO + i) % m->numPages;
            if (m->frames[idx].fixCount == 0) {
                victim = idx;
                m->nextFIFO = (idx + 1) % m->numPages;
                break;
            }
        }
    } else if (strat == RS_LRU || strat == RS_LRU_K) {
        int oldestTime = INT_MAX;
        for (int i = 0; i < m->numPages; i++) {
            if (m->frames[i].fixCount == 0 && m->frames[i].pageNum != NO_PAGE) {
                if (m->frames[i].lastUsed < oldestTime) {
                    oldestTime = m->frames[i].lastUsed;
                    victim = i;
                }
            }
        }
    } else { // default fallback FIFO
        for (int i = 0; i < m->numPages; i++) {
            int idx = (m->nextFIFO + i) % m->numPages;
            if (m->frames[idx].fixCount == 0) {
                victim = idx;
                m->nextFIFO = (idx + 1) % m->numPages;
                break;
            }
        }
    }

    return victim;
}

/* Write back frame to disk if dirty */
static RC writeFrameToDisk(BM_BufferPool *const bm, int frameIdx) {
    BM_MgmtData *m = getMgmt(bm);
    FrameInfo *f = &m->frames[frameIdx];
    if (f->pageNum == NO_PAGE || !f->dirty) return RC_OK;

    SM_FileHandle fh;
    RC rc = openHandle(bm->pageFile, &fh);
    if (rc != RC_OK) return rc;

    if (f->pageNum >= fh.totalNumPages) {
        ensureCapacity(f->pageNum + 1, &fh);
    }

    rc = writeBlock(f->pageNum, &fh, f->data);
    if (rc != RC_OK) { closePageFile(&fh); return rc; }

    m->numWriteIO++;
    f->dirty = FALSE;
    closePageFile(&fh);
    return RC_OK;
}

/* Read page from disk into frame->data */
static RC readPageToFrame(BM_BufferPool *const bm, PageNumber pageNum, int frameIdx) {
    BM_MgmtData *m = getMgmt(bm);
    FrameInfo *f = &m->frames[frameIdx];

    SM_FileHandle fh;
    RC rc = openHandle(bm->pageFile, &fh);
    if (rc != RC_OK) return rc;

    if (pageNum >= fh.totalNumPages) {
        rc = ensureCapacity(pageNum + 1, &fh);
        if (rc != RC_OK) { closePageFile(&fh); return rc; }
    }

    rc = readBlock(pageNum, &fh, f->data);
    if (rc != RC_OK) { closePageFile(&fh); return rc; }

    m->numReadIO++;
    closePageFile(&fh);

    f->pageNum = pageNum;
    f->dirty = FALSE;
    f->fixCount = 0;
    f->lastUsed = ++m->timeCounter;

    return RC_OK;
}

/* Initialize buffer pool */
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy, void *stratData) {
    if (bm == NULL || pageFileName == NULL || numPages <= 0) return RC_INVALID;
    bm->pageFile = (char *) malloc(strlen(pageFileName) + 1);
    strcpy(bm->pageFile, pageFileName);
    bm->numPages = numPages;
    bm->strategy = strategy;

    BM_MgmtData *m = (BM_MgmtData *) malloc(sizeof(BM_MgmtData));
    m->numPages = numPages;
    m->frames = (FrameInfo *) malloc(sizeof(FrameInfo) * numPages);
    m->nextFIFO = 0;
    m->timeCounter = 0;
    m->numReadIO = 0;
    m->numWriteIO = 0;

    for (int i = 0; i < numPages; i++) {
        m->frames[i].pageNum = NO_PAGE;
        m->frames[i].fixCount = 0;
        m->frames[i].dirty = FALSE;
        m->frames[i].lastUsed = -1;
        m->frames[i].data = (char *) malloc(PAGE_SIZE);
        memset(m->frames[i].data, 0, PAGE_SIZE);
    }

    bm->mgmtData = m;

    SM_FileHandle fh;
    initStorageManager();
    if (openPageFile((char *)bm->pageFile, &fh) == RC_OK) {
        closePageFile(&fh);
    } else {
        createPageFile((char *)bm->pageFile);
    }

    return RC_OK;
}

/* Shutdown buffer pool */
RC shutdownBufferPool(BM_BufferPool *const bm) {
    if (bm == NULL) return RC_INVALID;
    BM_MgmtData *m = getMgmt(bm);
    if (m == NULL) return RC_INVALID;

    // if pinned pages exist, cannot shutdown
    for (int i = 0; i < m->numPages; i++) {
        if (m->frames[i].fixCount > 0) 
            return RC_PINNED_PAGES;
    }

    // flush dirty pages
    for (int i = 0; i < m->numPages; i++) {
        if (m->frames[i].dirty) {
            RC rc = writeFrameToDisk(bm, i);
            if (rc != RC_OK) return rc;
        }
        free(m->frames[i].data);
    }

    free(m->frames);
    free(bm->mgmtData);
    bm->mgmtData = NULL;
    free(bm->pageFile);
    bm->pageFile = NULL;

    return RC_OK;
}

/* Flush all dirty pages with fixCount 0 */
RC forceFlushPool(BM_BufferPool *const bm) {
    if (bm == NULL) return RC_INVALID;
    BM_MgmtData *m = getMgmt(bm);
    if (m == NULL) return RC_INVALID;

    for (int i = 0; i < m->numPages; i++) {
        if (m->frames[i].dirty && m->frames[i].fixCount == 0) {
            RC rc = writeFrameToDisk(bm, i);
            if (rc != RC_OK) return rc;
        }
    }
    return RC_OK;
}

/* Mark page as dirty */
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
    if (bm == NULL || page == NULL) return RC_INVALID;
    BM_MgmtData *m = getMgmt(bm);
    int idx = findFrameIndex(m, page->pageNum);
    if (idx < 0) return RC_PAGE_NOT_FOUND;
    m->frames[idx].dirty = TRUE;
    return RC_OK;
}

/* Unpin page */
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    if (bm == NULL || page == NULL) return RC_INVALID;
    BM_MgmtData *m = getMgmt(bm);
    int idx = findFrameIndex(m, page->pageNum);
    if (idx < 0) return RC_PAGE_NOT_FOUND;
    if (m->frames[idx].fixCount > 0) m->frames[idx].fixCount--;
    return RC_OK;
}

/* Force page to disk */
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    if (bm == NULL || page == NULL) return RC_INVALID;
    BM_MgmtData *m = getMgmt(bm);
    int idx = findFrameIndex(m, page->pageNum);
    if (idx < 0) return RC_PAGE_NOT_FOUND;
    return writeFrameToDisk(bm, idx);
}

/* Pin a page into buffer pool */
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
    if (bm == NULL || page == NULL || pageNum < 0) return RC_INVALID;
    BM_MgmtData *m = getMgmt(bm);
    if (m == NULL) return RC_INVALID;

    int idx = findFrameIndex(m, pageNum);
    if (idx >= 0) {
        m->frames[idx].fixCount++;
        m->frames[idx].lastUsed = ++m->timeCounter;
        page->pageNum = pageNum;
        page->data = m->frames[idx].data;
        return RC_OK;
    }

    int victim = pickVictim(bm);
    if (victim < 0) {
        return RC_NO_FREE_SLOT;   // correct error when all pinned
    }

    if (m->frames[victim].pageNum != NO_PAGE) {
        if (m->frames[victim].dirty) {
            RC rc = writeFrameToDisk(bm, victim);
            if (rc != RC_OK) return rc;
        }
    }

    RC rc = readPageToFrame(bm, pageNum, victim);
    if (rc != RC_OK) return rc;

    m->frames[victim].fixCount = 1;
    m->frames[victim].lastUsed = ++m->timeCounter;
    page->pageNum = pageNum;
    page->data = m->frames[victim].data;

    return RC_OK;
}

/* === Stats === */
PageNumber *getFrameContents(BM_BufferPool *const bm) {
    if (bm == NULL) return NULL;
    BM_MgmtData *m = getMgmt(bm);
    PageNumber *arr = malloc(sizeof(PageNumber) * m->numPages);
    for (int i = 0; i < m->numPages; i++) arr[i] = m->frames[i].pageNum;
    return arr;
}

bool *getDirtyFlags(BM_BufferPool *const bm) {
    if (bm == NULL) return NULL;
    BM_MgmtData *m = getMgmt(bm);
    bool *arr = malloc(sizeof(bool) * m->numPages);
    for (int i = 0; i < m->numPages; i++) arr[i] = m->frames[i].dirty;
    return arr;
}

int *getFixCounts(BM_BufferPool *const bm) {
    if (bm == NULL) return NULL;
    BM_MgmtData *m = getMgmt(bm);
    int *arr = malloc(sizeof(int) * m->numPages);
    for (int i = 0; i < m->numPages; i++) arr[i] = m->frames[i].fixCount;
    return arr;
}

int getNumReadIO(BM_BufferPool *const bm) {
    if (bm == NULL) return -1;
    BM_MgmtData *m = getMgmt(bm);
    return m->numReadIO;
}

int getNumWriteIO(BM_BufferPool *const bm) {
    if (bm == NULL) return -1;
    BM_MgmtData *m = getMgmt(bm);
    return m->numWriteIO;
}
