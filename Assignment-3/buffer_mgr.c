#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "test_helper.h"
#include <stdio.h>
#include <stdlib.h>

// Hold data in a doubly linked list
typedef struct Page{
    SM_PageHandle data;
    PageNumber pageNum;
    bool isDirty;
    int fixCount;
    struct Page *prevPage;
    struct Page *nextPage;
    bool clockUsed;
} Page;

typedef struct PageFrame{
    Page *firstPage;
    Page *lastPage;
    Page *pageArray;
    int numWriteIO;
    int numReadIO;
    int currentActivePages;
    int *lruHelper;
} PageFrame;

void printPageBuffer(BM_BufferPool *const bm)
{
    int i = 0;
    PageFrame *pages = bm->mgmtData;
    
    printf("==----======------==\n");
    for (i = 0; i < bm->numPages; i++)
    {
        printf("%s fixCount:%d dirty:%d num:%d\n", pages->pageArray[i].data, pages->pageArray[i].fixCount, pages->pageArray[i].isDirty, pages->pageArray[i].pageNum);
    }
    printf("==----======------==\n");
}

void printHelperBuffer(BM_BufferPool *const bm)
{
    int i = 0;
    PageFrame *pages = bm->mgmtData;
    printf("==---------=========--------==\n");
    for(i = 0; i < bm->numPages; i++)
    {
        printf("[] %d []\n", pages->lruHelper[i]);
    }
    printf("==---------=========--------==\n");
    
}
Page* movePageToNext(PageFrame *frame, Page *curPage)
{
        Page *returnPtr;
        if(curPage->nextPage == NULL)
        {
            returnPtr = frame->pageArray;
        }
        else
        {
            returnPtr = curPage->nextPage;
        }
        return returnPtr;
}

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData)
{
    int i = 0;
    // Allocate our structure with pages
    PageFrame *pages = (PageFrame *) malloc(sizeof(PageFrame));
    Page *pageArray = (Page *) malloc(sizeof(Page) * numPages);
    pages->lruHelper = (int *) malloc(sizeof(int) * numPages);
    
    pages->pageArray = pageArray;
    pages->numWriteIO = 0;
    pages->numReadIO = 0;
    pages->currentActivePages = 0;
    
    pages->firstPage = &pages->pageArray[0];
    pages->lastPage = &pages->pageArray[numPages-1];
    
    // Allocate page array for page frame
    for( i = 0; i < numPages; i++)
    {
        // Linked elements so we can navigate for the pinning algorithms
        if( i == 0){
            pages->pageArray[i].prevPage = NULL;
            pages->pageArray[i].nextPage = &pages->pageArray[i+1];
            pages->pageArray[i].clockUsed = false;
        }
        else if(i == numPages-1)
        {
            pages->pageArray[i].prevPage = &pages->pageArray[i-1];
            pages->pageArray[i].nextPage = NULL;
            pages->pageArray[i].clockUsed = false;
        }
        else
        {
            pages->pageArray[i].prevPage = &pages->pageArray[i-1];
            pages->pageArray[i].nextPage = &pages->pageArray[i+1];
            pages->pageArray[i].clockUsed = false;
        }
        
        // Instantiate rest of page data
        pages->pageArray[i].data = (char *) malloc(sizeof(char) * PAGE_SIZE);
        pages->pageArray[i].fixCount = 0;
        pages->pageArray[i].isDirty = false;
        pages->pageArray[i].pageNum = NO_PAGE;
        
        pages->lruHelper[i] = NO_PAGE;
     }
    
    // Store data in the BM_BufferPool
    bm->pageFile = (char *) pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;
    bm->mgmtData = pages;
    ASSERT_TRUE((bm->mgmtData == NULL),"buffermgmt is null");
    
    return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm)
{
    int i = 0;
    SM_FileHandle fileHandle;
    openPageFile(bm->pageFile, &fileHandle);      // Open the page file in preperation for writing
    
    // Get the page frame
    PageFrame *pages = (PageFrame *)bm->mgmtData;
    // Go through pages in the page frame and see if any pages are dirty.
    //	If there is a dirty page wrtie the pages to the buffer
    int numberPages = bm->numPages;
    for( i = 0; i < numberPages; i++){
        if(pages->pageArray[i].fixCount != 0)
        {
            return RC_WRITE_FAILED;
        }
        if(pages->pageArray[i].isDirty)
        {
            ensureCapacity((pages->pageArray[i].pageNum), &fileHandle);
            writeBlock (pages->pageArray[i].pageNum, &fileHandle, pages->pageArray[i].data);
            pages->pageArray[i].isDirty = false;
            pages->numWriteIO++;
        }
    }
    closePageFile(&fileHandle);
    
    free(pages->pageArray);
    free(pages->lruHelper);
    free(pages);
    return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm)
{
    int i = 0;
    SM_FileHandle fileHandle;
    openPageFile(bm->pageFile, &fileHandle);      // Open the page file in preperation for writing
    
    BM_PageHandle *pageHandle = (BM_PageHandle *) malloc(sizeof(BM_PageHandle));
    PageFrame *pages = bm->mgmtData;
    Page *curPage;
    
    for ( i = 0;i < pages->currentActivePages; i++)
    {
        // If the page is dirty, write to file
        curPage = pages->firstPage;
        if(curPage->isDirty)
        {
            pageHandle->data = curPage->data;
            pageHandle->pageNum = curPage->pageNum;
            forcePage(bm, pageHandle);
        }
        
        // Move the first and last page down the list
        if(pages->firstPage->nextPage != NULL && pages->lastPage->nextPage != NULL)
        {
            pages->firstPage = pages->firstPage->nextPage;
            pages->lastPage = pages->lastPage->nextPage;
        }
        else
        {
            if(pages->firstPage->nextPage == NULL)
            {
                pages->firstPage = pages->pageArray;
                pages->lastPage = pages->lastPage->nextPage;
            }
            else
            {
                pages->firstPage = pages->firstPage->nextPage;
                pages->lastPage = pages->pageArray;
            }
        }
    }
    closePageFile(&fileHandle);
    free(pageHandle);
    return RC_OK;
}

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    int i = 0; 
    PageFrame *pages = (PageFrame*)bm->mgmtData;
    Page *curPage;
    
    // Get the page that should be set as dirty, have to iterate through our array to find it
    for (i = 0;i < pages->currentActivePages; i++)
    {
        curPage = &pages->pageArray[i];
        if(curPage->pageNum == page->pageNum)
        {
            //curPage->isDirty = true;
            pages->pageArray[i].isDirty = true;
            // Copy data into the buffer so we write the update info to the file
            strcpy(pages->pageArray[i].data, page->data);
            return RC_OK;
        }
    }
    
    return RC_OK;
}

// Client is responsible for telling the buffer if the page is dirty
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    int i = 0;
    if(page->pageNum < 0){
        return NO_PAGE;
    }
    
    PageFrame *pages = bm->mgmtData;
    Page *curPage;
    
    for (i = 0; i < pages->currentActivePages; i++)
    {
        curPage = &pages->pageArray[i];
        if(curPage->pageNum == page->pageNum)
        {
            curPage->fixCount--;
            break;
        }
    }
    
    return RC_OK;
}

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    int i = 0;
    if(page->pageNum < 0){
        return NO_PAGE;
    }
    
    SM_FileHandle fileHandle;
    openPageFile(bm->pageFile, &fileHandle);      // Open the page file in preperation for writing
    PageFrame *pages = bm->mgmtData;
    Page *curPage;
    
    for (i = 0;i < pages->currentActivePages; i++)
    {
        curPage = &pages->pageArray[i];
        if(curPage->pageNum == page->pageNum)
        {
            ensureCapacity((curPage->pageNum), &fileHandle);
            writeBlock (curPage->pageNum, &fileHandle, curPage->data);
            curPage->isDirty = false;
            pages->numWriteIO++;
            break;
        }
    }
    closePageFile(&fileHandle);
    return RC_OK;
}

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
            const PageNumber pageNum)
{
    int i = 0,j = 0; 
    if(page->pageNum < 0)
    {
        return NO_PAGE;
    }
    
    SM_FileHandle fileHandle;
    PageFrame *pages = (PageFrame*)bm->mgmtData;
    BM_PageHandle *pageHandle;
    Page *curPage;
    Page *temp;
    page->data = (char *) malloc(sizeof(char) * PAGE_SIZE);
    
    switch (bm->strategy) {
        case RS_FIFO:
            // Search through buffer first
            for(i = 0; i < bm->numPages; i++){
                curPage = &pages->pageArray[i];
                
                // Found the page in the buffer
                if(curPage->pageNum == pageNum){
                    strcpy(page->data, curPage->data);
                    page->pageNum = curPage->pageNum;
                    curPage->fixCount++;
                    // Don't assign a value and break out of loops/switch, returning is easier and less
                    //  confusing for this part
                    return RC_OK;
                }
                
                // Open space in the buffer
                if(curPage->pageNum == NO_PAGE){
                    openPageFile(bm->pageFile, &fileHandle);
                    readBlock(pageNum, &fileHandle, curPage->data);
                    curPage->pageNum = pageNum;
                    curPage->fixCount++;
                    curPage->isDirty = false;
                    
                    // Write data to buffer page handle
                    strcpy(page->data, curPage->data);
                    page->pageNum = pageNum;
                    
                    // Update pageFrame metadata
                    pages->currentActivePages++;
                    pages->numReadIO++;
                    
                    closePageFile(&fileHandle);
                    return RC_OK;
                }
            }// End of for loop
            // At this point, the buffer is full and does not contain the page we need
            
            pageHandle = (BM_PageHandle *) malloc(sizeof(BM_PageHandle));
            temp = pages->firstPage;
            
            // Check if the first page can be unpinned
            if(temp->fixCount == 0)
            {
                // If the page is dirty, write to file
                if(temp->isDirty)
                {
                    pageHandle->data = temp->data;
                    pageHandle->pageNum = temp->pageNum;
                    forcePage(bm, pageHandle);
                }
                
                // Move the first and last page down the list
                if(pages->firstPage->nextPage != NULL && pages->lastPage->nextPage != NULL)
                {
                    pages->firstPage = pages->firstPage->nextPage;
                    pages->lastPage = pages->lastPage->nextPage;
                }
                else
                {
                    if(pages->firstPage->nextPage == NULL)
                    {
                        pages->firstPage = pages->pageArray;
                        pages->lastPage = pages->lastPage->nextPage;
                    }
                    else
                    {
                        pages->firstPage = pages->firstPage->nextPage;
                        pages->lastPage = pages->pageArray;
                    }
                }
            }
            // First page is not unpinned
            // Move the first page to the end and add the page to be pinned to the buffer
            else
            {
                // The first page becomes the second oldest and the pinned page becomes the oldest
                // We end up "removing" two elements from the buffer, the first element gets removed
                //  and added into the rear and we add the page we want to pin
                /*
                 |1|2|3|4|
                 Before:|F| | |L|
                 Pinned:|P| | | |
                 
                 |1|2|3|4|
                 After: | |L|F| |
                 Pinned:|P| | | |
                 
                 */
                if(pages->firstPage->nextPage == NULL)
                {
                    pages->lastPage = pages->pageArray;
                    temp = pages->lastPage;
                }
                else
                {
                    pages->lastPage = pages->firstPage->nextPage;
                    temp = pages->lastPage;
                }
                // If the page is dirty, write to file
                if(temp->isDirty)
                {
                    pageHandle->data = temp->data;
                    pageHandle->pageNum = temp->pageNum;
                    forcePage(bm, pageHandle);
                }
                
                // Move the first page down the list, the last page has already been moved
                if(pages->firstPage->nextPage->nextPage != NULL)
                {
                    pages->firstPage = pages->firstPage->nextPage->nextPage;
                }
                else
                {
                    pages->firstPage = pages->pageArray;
                }
            }
            
            openPageFile(bm->pageFile, &fileHandle);
            readBlock(pageNum, &fileHandle, temp->data);
            temp->pageNum = pageNum;
            temp->fixCount++;
            temp->isDirty = false;
            
            strcpy(page->data, temp->data);
            page->pageNum = pageNum;
            
            pages->numReadIO++;
            
            closePageFile(&fileHandle);
            free(pageHandle);
            
            return RC_OK;
            break;
            
        case RS_LRU:
            // Search through buffer first
            //int j; 
            //int i = 0, pos = 0, j = 0; 
            for( i = 0; i < bm->numPages; i++){
                // Found the page in the buffer
                if(pages->pageArray[i].pageNum == pageNum){
                    // Update page info
                    pages->pageArray[i].fixCount++;
                    
                    // Shift list up until element in front of the one we found
                    // Shift rest of pages down the list
                    int pos;
                    for(pos = 0; pos < pages->currentActivePages-2; pos++)
                    {
                        if(pages->lruHelper[pos] == pageNum)
                        {
                            break;
                        }
                        //pages->lruHelper[pos] = pages->lruHelper[pos+1];
                    }
                    
                    for( j = pos; j < pages->currentActivePages-1; j++)
                    {
                        pages->lruHelper[j] = pages->lruHelper[j+1];
                    }
                    pages->lruHelper[bm->numPages-1] = pageNum;

                    
                    pages->lruHelper[pages->currentActivePages-1] = pageNum;
                    
                    // Copy into page
                    strcpy(page->data, pages->pageArray[i].data);
                    page->pageNum = pages->pageArray[i].pageNum;
                    
                    
                    return RC_OK;
                }
                
                // Open space in the buffer
                if(pages->pageArray[i].pageNum == NO_PAGE){
                    // Point end of page to current position
                    pages->lastPage = &pages->pageArray[i];
                    
                    openPageFile(bm->pageFile, &fileHandle);
                    readBlock(pageNum, &fileHandle, pages->lastPage->data);
                    pages->lastPage->pageNum = pageNum;
                    pages->lastPage->fixCount++;
                    pages->lastPage->isDirty = false;

                    
                    // Write data to buffer page handle
                    strcpy(page->data, pages->lastPage->data);
                    page->pageNum = pageNum;
                 
                    // Update pageFrame metadata
                    pages->currentActivePages++;
                    pages->numReadIO++;
                    
                    pages->lruHelper[i] = pageNum;
                    
                    closePageFile(&fileHandle);
                    
                    return RC_OK;
                }
            }// End of for loop
            // At this point, the buffer is full and does not contain the page we need
            
            // Find the page to remove in the buffer
            
            curPage = &pages->pageArray[0];
            for(i = 0; i < bm->numPages; i++)
            {
                curPage = &pages->pageArray[i];
                if(pages->lruHelper[0] == curPage->pageNum)
                {
                    // If the page is dirty, write to file
                    if(curPage->isDirty)
                    {
                        pageHandle->data = temp->data;
                        pageHandle->pageNum = temp->pageNum;
                        forcePage(bm, pageHandle);
                    }
                    break;
                }
            }
            
            // Shift rest of pages down the list
            int pos;
            for(pos = 0; pos < bm->numPages-2; pos++)
            {
                if(pages->lruHelper[pos] == pageNum)
                {
                    break;
                }
                pages->lruHelper[pos] = pages->lruHelper[pos+1];
            }
            
            for(j = pos; j < bm->numPages-1; j++)
            {
                pages->lruHelper[j] = pages->lruHelper[j+1];
            }
            pages->lruHelper[bm->numPages-1] = pageNum;
            
            pageHandle = (BM_PageHandle *) malloc(sizeof(BM_PageHandle));
        
            openPageFile(bm->pageFile, &fileHandle);
            readBlock(pageNum, &fileHandle, curPage->data);
            curPage->pageNum = pageNum;
            curPage->fixCount++;
            curPage->isDirty = false;
            
            strcpy(page->data, curPage->data);
            page->pageNum = pageNum;
            
            pages->numReadIO++;
            
            closePageFile(&fileHandle);
            free(pageHandle);
            
            return RC_OK;
            break;
            
        case RS_CLOCK:
            for(i = 0; i < bm->numPages*2+1; i++){
                curPage = pages->firstPage;
                // Found the page in the buffer
                if(curPage->pageNum == pageNum){
                    // Update page info
                    curPage->fixCount++;
                    curPage->clockUsed = true;
                    
                    // Copy into page
                    strcpy(page->data, curPage->data);
                    page->pageNum = curPage->pageNum;
                    
                    pages->firstPage = movePageToNext(pages, pages->firstPage);
                    return RC_OK;
                }
                // Empty space
                if(curPage->pageNum == NO_PAGE)
                {
					openPageFile(bm->pageFile, &fileHandle);
                    readBlock(pageNum, &fileHandle, curPage->data);
                    curPage->pageNum = pageNum;
                    curPage->fixCount++;
                    curPage->isDirty = false;
                    curPage->clockUsed = true;
                    
                    // Write data to buffer page handle
                    strcpy(page->data, curPage->data);
                    page->pageNum = pageNum;
                    // Update pageFrame metadata
                    pages->currentActivePages++;
                    pages->numReadIO++;
                    
                    closePageFile(&fileHandle);
                
                    pages->firstPage = movePageToNext(pages, pages->firstPage);
                    
                    return RC_OK;
                }
                // No empty space
                else
                {
	                if(curPage->clockUsed)
	                {	
	                    curPage->clockUsed = false;
	                    pages->firstPage = movePageToNext(pages, pages->firstPage);
	                }
	                else
	                {
	                	
	                	// If the page is dirty, write to file
	                    if(curPage->isDirty)
	                    {
	                        pageHandle->data = curPage->data;
	                        pageHandle->pageNum = curPage->pageNum;
	                        forcePage(bm, pageHandle);
	                    }

	                    if(curPage->fixCount == 0)
	                    {
		                    openPageFile(bm->pageFile, &fileHandle);
		                    readBlock(pageNum, &fileHandle, curPage->data);
		                    curPage->pageNum = pageNum;
		                    curPage->fixCount++;
		                    curPage->isDirty = false;
		                    curPage->clockUsed = true;
		                    
		                    // Write data to buffer page handle
		                    strcpy(page->data, curPage->data);
		                    page->pageNum = pageNum;
		                    // Update pageFrame metadata
		                    pages->currentActivePages++;
		                    pages->numReadIO++;
		                    
		                    closePageFile(&fileHandle);
	                    
		                    pages->firstPage = movePageToNext(pages, pages->firstPage);
		                    
		                    return RC_OK;
						}	
						// Cannot remove page from buffer
						pages->firstPage = movePageToNext(pages, pages->firstPage);
	                }

                }


            }
            // No empty space in the buffer anymore
            for( i = 0; i < bm->numPages; i++)
            {
                curPage = pages->firstPage;
                // Found a page to overwrite
                if(!curPage->clockUsed)
                {
                    
                }
                movePageToNext(pages, pages->firstPage);
            }
            
            return RC_OK;
            break;
        default:
            return RC_OK;
            break;
    }
    
}

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
    int i = 0;
    PageFrame *pages = bm->mgmtData;
    Page *curPage;
    PageNumber *data = (PageNumber *) malloc(sizeof(PageNumber *) * bm->numPages);
    
    for (i = 0;i < bm->numPages; i++)
    {
        curPage = &pages->pageArray[i];
        if(curPage->pageNum == NO_PAGE)
        {
            data[i] = NO_PAGE;
        }
        else{
            data[i] = curPage->pageNum;
        }
    }
    return data;
}

bool *getDirtyFlags (BM_BufferPool *const bm)
{
    int i = 0;
    PageFrame *pages = bm->mgmtData;
    Page *curPage;
    bool *data = (bool *) malloc(sizeof(bool *) * bm->numPages);
    
    for ( i = 0;i < bm->numPages; i++)
    {
        curPage = &pages->pageArray[i];
        data[i] = curPage->isDirty;
    }
    return data;
}

int *getFixCounts (BM_BufferPool *const bm)
{
    int i = 0;
    PageFrame *pages = bm->mgmtData;
    Page *curPage;
    int *data = (int *) malloc(sizeof(int *) * bm->numPages);
    
    for (i = 0;i < bm->numPages; i++)
    {
        curPage = &pages->pageArray[i];
        data[i] = curPage->fixCount;
    }
    return data;
}

int getNumReadIO (BM_BufferPool *const bm)
{
    PageFrame *pages = bm->mgmtData;
    
    return pages->numReadIO;
}

int getNumWriteIO (BM_BufferPool *const bm)
{
    PageFrame *pages = bm->mgmtData;
    
    return pages->numWriteIO;
    
}
