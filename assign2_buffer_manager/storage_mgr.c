#include "storage_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 4096  // define size of each page

// initialize storage manager (does nothing for now)
void initStorageManager(void) { }

// create a page file with 1 empty page
RC createPageFile(char *fileName) {
    FILE *fp = fopen(fileName, "w+");
    if (!fp) return RC_FILE_NOT_FOUND;
    char buffer[PAGE_SIZE];
    memset(buffer, 0, PAGE_SIZE);
    fwrite(buffer, sizeof(char), PAGE_SIZE, fp);
    fclose(fp);
    return RC_OK;
}

// open an existing page file
RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    FILE *fp = fopen(fileName, "r+");
    if (!fp) return RC_FILE_NOT_FOUND;
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fHandle->fileName = fileName;
    fHandle->totalNumPages = size / PAGE_SIZE;
    fHandle->curPagePos = 0;
    fHandle->mgmtInfo = fp;
    return RC_OK;
}

// close page file
RC closePageFile(SM_FileHandle *fHandle) {
    FILE *fp = (FILE *)fHandle->mgmtInfo;
    fclose(fp);
    return RC_OK;
}

// delete page file
RC destroyPageFile(char *fileName) {
    return remove(fileName) == 0 ? RC_OK : RC_FILE_NOT_FOUND;
}

// read page from disk into memory
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    FILE *fp = (FILE *)fHandle->mgmtInfo;
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) return RC_READ_NON_EXISTING_PAGE;
    fseek(fp, pageNum * PAGE_SIZE, SEEK_SET);
    fread(memPage, sizeof(char), PAGE_SIZE, fp);
    fHandle->curPagePos = pageNum;
    return RC_OK;
}

// return current page position
int getBlockPos(SM_FileHandle *fHandle) {
    return fHandle->curPagePos;
}

// write page to disk
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    FILE *fp = (FILE *)fHandle->mgmtInfo;
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) return RC_WRITE_FAILED;
    fseek(fp, pageNum * PAGE_SIZE, SEEK_SET);
    fwrite(memPage, sizeof(char), PAGE_SIZE, fp);
    fHandle->curPagePos = pageNum;
    return RC_OK;
}

// write to current page
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

// add empty page to end of file
RC appendEmptyBlock(SM_FileHandle *fHandle) {
    FILE *fp = (FILE *)fHandle->mgmtInfo;
    char buffer[PAGE_SIZE];
    memset(buffer, 0, PAGE_SIZE);
    fwrite(buffer, sizeof(char), PAGE_SIZE, fp);
    fHandle->totalNumPages += 1;
    return RC_OK;
}

// ensure file has at least numberOfPages
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    while (fHandle->totalNumPages < numberOfPages) {
        appendEmptyBlock(fHandle);
    }
    return RC_OK;
}
