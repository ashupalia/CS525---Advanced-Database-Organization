#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage_mgr.h"
#include "dberror.h"

#include "buffer_mgr.h"
#include "dt.h"
#include "test_helper.h"
#include "buffer_ll.h"

RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const pageHandle) {

    RC err = RC_OK;
    BM_MgmtData_Manager *bdm;
    BM_Frame *frame;
    if (err = assertBM_BufferPool(bm) != RC_OK)
        return err;
    if (err = assertBM_PageHandle(pageHandle) != RC_OK)
        return err;
    bdm = (BM_MgmtData_Manager *) bm->mgmtData;
    frame = lookupBufferFrame(bdm, pageHandle->pageNum);
    if (frame == NULL) {
        return RC_BM_FRAME_NOT_FOUND;
    }
    frame->fi->fixCount--;
    if (frame->fi->fixCount < 0)
        frame->fi->fixCount = 0;

    return RC_OK;
}

//Generating hash key for hash map
int BM_pageHash(PageNumber pageNumber) {
    return (pageNumber * pageNumber);
}

//asserting bm before it is de-referenced
RC assertBM_BufferPool(BM_BufferPool *bm) {
    if(bm == NULL)  return RC_BM_INVALID_POOL;
    if(bm->pageFile == NULL) {
       return RC_FILE_NOT_FOUND;
    }
    if(bm->mgmtData == NULL)    return RC_BM_INVALID_FRAME_MANAGER;
    return RC_OK;
}

//asserting pageframe handler
RC assertBM_PageHandle(BM_PageHandle *pageHandle) {

    if (pageHandle == NULL || pageHandle->data == NULL)
        return RC_BM_INVALID_PAGE_HANDLE;
    if (pageHandle->pageNum < 0)
        return RC_BM_INVALID_PAGE_NUM;
    return RC_OK;
}


RC markDirty(BM_BufferPool *bm, BM_PageHandle *pageHandle) {

    RC err = RC_OK;
    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) bm->mgmtData;
    BM_Frame *frame;
    if (err = assertBM_BufferPool(bm) != RC_OK)
        return err;
    if (err = assertBM_PageHandle(pageHandle) != RC_OK)
        return err;
    //looking up in hash table if we have this element 
    frame = lookupBufferFrame(bdm, pageHandle->pageNum);
    
    if (frame == NULL) {

        return RC_BM_FRAME_NOT_FOUND;
 } else {
    frame->fi->dirty = TRUE;
 }
    return RC_OK;
}
BM_Frame *lookupBufferFrame(BM_MgmtData_Manager *bdm, PageNumber PageNum) {

    if (bdm == NULL) {
        return NULL;
    }

    if (bdm->HMapTable == NULL) {
        return NULL;
    }
    if (PageNum < 0) {
        return NULL;
   }
    int bucket;
    BM_HashBookElements *elt;

    //total buckets is maxFrames

    bucket = BM_pageHash(PageNum) % bdm->maxFrames;
    elt = *(bdm->HMapTable + bucket);
    while (elt != NULL && elt->key!=-1) {
     if (elt->value->fi->pageHandle->pageNum==PageNum) {
        return elt->value;
     }
    elt = elt->next;
    }

    return NULL;

}






RC shutdownBufferPool(BM_BufferPool *const bm) {

    RC err = RC_OK;
    BM_MgmtData_Manager *bdm;
    if (err = assertBM_BufferPool(bm) != RC_OK)
        return err;
    if (err = BM_MgmtData_Manager_Wipe(bm->mgmtData, bm->strategy) != RC_OK)
        return err;
    
    free(bm->pageFile);
    return RC_OK;
}




// Initializing Buffer Manager Pool
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		  const int numPages, ReplacementStrategy strategy, void *stratData)  {

    if (pageFileName == NULL) {
        return RC_FILE_NOT_FOUND;
    }
    if (numPages < 1)
	return RC_BM_INVALID_NUM_FRAMES;
    RC err;
    
    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) malloc(sizeof(BM_MgmtData_Manager));
    bm->pageFile = (char *) malloc(sizeof(char)*strlen(pageFileName) + 1);
    if (bm->pageFile == NULL)
	return RC_MALLOC_FAILED;
    strcpy(bm->pageFile, pageFileName);
    bm->numPages = numPages;
    bm->strategy = strategy;
    bm->mgmtData = (BM_MgmtData_Manager *)malloc(sizeof(BM_MgmtData_Manager));
    
    if(bm->mgmtData == NULL) {
       return RC_MALLOC_FAILED;
   }
    err = RC_OK;
    if(err = BM_MgmtData_Manager_Init(bdm, numPages, strategy) != RC_OK)
        return err;
    bm->mgmtData = (BM_MgmtData_Manager *)bdm;
       if(bm->mgmtData == NULL) {
       return RC_MALLOC_FAILED;
   }
    initStorageManager();

    return RC_OK;
}


bool *getDirtyFlags (BM_BufferPool *const bm)   {
 
    int i = 0;
    RC err = RC_OK;
    if (bm == NULL) {
        return NULL;
    }
    bool *dirtyindex;    
    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) bm->mgmtData;
    BM_Frame *frame;
    dirtyindex = (bool *)malloc(sizeof(bool) *bm->numPages);
    if (dirtyindex == NULL){
       return NULL;
    }
    /*if farame number is not present in linked list  initialize dirty index array */
    frame = bdm->head;
     while (i < bm->numPages) {
        dirtyindex[i] = false;
         i++;
    }
    i = 0;
    while(frame != NULL)   {
        if (frame->fi == NULL || frame->fi->pageHandle == NULL)
            break;
        dirtyindex[i++] = frame->fi->dirty;
        frame = frame->next;
    }
   return dirtyindex;
}
int  *getFixCounts(BM_BufferPool *const bm)   {

    int i = 0;
    RC err = RC_OK;
    if (bm == NULL) {
        return NULL;
    }
    int *fixcountindex;
    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) bm->mgmtData;
    BM_Frame *frame;
    fixcountindex = (int *)malloc(sizeof(int) *bm->numPages);
    if (fixcountindex == NULL)
        return NULL;

    /*if frame number is not present in linked list  initialize fixcount array */
    frame = bdm->head;
     while (i < bm->numPages) {
        fixcountindex[i] = 0;
         i++;
    }
    i = 0;
    // filling frame contents overriding wherever required
     while(frame != NULL)   {
        if (frame->fi == NULL || frame->fi->pageHandle == NULL)
            break;
        fixcountindex[i++] = frame->fi->fixCount;
        frame = frame->next;
    }

    return fixcountindex;


}

int getNumReadIO (BM_BufferPool *const bm) {
    RC err = RC_OK;
    if(bm == NULL)  
       return 0;

    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) bm->mgmtData;
   
    if (bdm != NULL)  
    return bdm->readCount;
    return 0;
}



int getNumWriteIO (BM_BufferPool *const bm) {
    RC err = RC_OK;
    if(bm == NULL)  return err;

    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) bm->mgmtData;
    if (bdm != NULL)  
    return bdm->writeCount;
    return -1;
}

PageNumber *getFrameContents (BM_BufferPool *const bm) {

    int i = 0;
    RC err = RC_OK;
    if (bm == NULL) {
        return NULL;
    }
    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) bm->mgmtData;
    BM_Frame *frame;

    PageNumber *Contents = (PageNumber *)malloc(sizeof(PageNumber)*bm->numPages);
    if (Contents == NULL)
        return NULL;
    frame = bdm->head;
    /* initializing page array */
    while (i < bm->numPages) {
         Contents[i] = NO_PAGE;
         i++; 
    }
    i = 0;
      
    while(frame != NULL)   {
        if (frame->fi == NULL || frame->fi->pageHandle == NULL)
            break;
        Contents[i++] = frame->fi->pageHandle->pageNum;
        frame = frame->next;
    }
    return Contents;
}
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)   {
    RC err = RC_OK;
    if(err = assertBM_BufferPool(bm) != RC_OK)  return err;
    if(err = assertBM_PageHandle(page) != RC_OK)  return err;

    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) bm->mgmtData;
    SM_FileHandle fHandle;
    BM_Frame *frame;
    frame = lookupBufferFrame(bdm, page->pageNum);
    if(frame == NULL)
        return RC_BM_FRAME_NOT_FOUND;

    if(err = openPageFile(bm->pageFile, &fHandle) != RC_OK)
        return err;

    if(err = writeBlock(page->pageNum,
                            &fHandle,
                            page->data) != RC_OK)
        return err;

    bdm->writeCount++;
    frame->fi->dirty = FALSE;

    return RC_OK;
}
RC forceFlushPool(BM_BufferPool *const bm) {
  
    RC err = RC_OK;
    if(err = assertBM_BufferPool(bm) != RC_OK)  return err;
    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) bm->mgmtData;
    BM_Frame *cur = bdm->head;
    SM_FileHandle fHandle;
    BM_PageHandle *pageHandle;
    if(err = openPageFile(bm->pageFile, &fHandle) != RC_OK)
        return err;
     while(cur != NULL) {
        if(cur->fi->fixCount == 0 && cur->fi->dirty == TRUE)  {
            pageHandle = cur->fi->pageHandle;
            if(err = writeBlock(pageHandle->pageNum,
                                    &fHandle,
                                    pageHandle->data) != RC_OK)
                return err;
            bdm->writeCount++;
            cur->fi->dirty = FALSE;
        }
        cur = cur->next;
       
    }
    return RC_OK;
}


RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
   const PageNumber pageNum) {
    RC err = RC_OK;
    int flag = 0;
    if (err = assertBM_BufferPool(bm)!= RC_OK)
        return err;

    BM_MgmtData_Manager *bdm = (BM_MgmtData_Manager *) bm->mgmtData;
    
    BM_Frame *frame, *frame1;
    SM_FileHandle fHandle;
    frame = lookupBufferFrame(bdm, pageNum);
    page->pageNum = pageNum;
   //frame is not present in main memory will have to fetch from disk 
   if (frame == NULL) {
        char *other=(char *) malloc((sizeof(char) * PAGE_SIZE)+1);
        flag = 1;

        page->pageNum = pageNum;
        page->data = (char *) malloc((sizeof(char) * PAGE_SIZE)+1);
       
        if(page->data == NULL)
            return RC_MALLOC_FAILED;
        if(err = openPageFile(bm->pageFile, &fHandle) != RC_OK)
            return err;
        if(err = ensureCapacity(pageNum+1, &fHandle) != RC_OK){
            return err;
          }
        if(err = readBlock(pageNum,&fHandle,other) != RC_OK) {
            return err;
        }
        sprintf(page->data,"%s-%i","Page",pageNum);
        bdm->readCount++;

        if(err = closePageFile(&fHandle) != RC_OK)
          return err;

   frame = (BM_Frame *) malloc(sizeof(BM_Frame));
   if(frame == NULL)
       return RC_MALLOC_FAILED;
   if (err = BM_Frame_Archive_Page(frame, page)!= RC_OK){
       return err;
   }

   frame->fi->fixCount++;
   //all frames are occupied time to replace using "strategy
   if (bdm->availFrames <= 0) {
       if(err = evict(bdm, bm->strategy, &fHandle) != RC_OK){
           return err;
   }
   }
   //enqueing now that we have one free frame to hold new page
        if (err = enqueue(bdm, bm->strategy, frame) != RC_OK) {
            return err;
      }
        
        bdm->availFrames--;
        frame1 = lookupBufferFrame(bdm,pageNum);
        bm->mgmtData=(BM_MgmtData_Manager *) bdm;

       return RC_OK;
}
   // re-referenced page since we did find this frame in hash map
   page->pageNum = pageNum;
   page->data = frame->fi->pageHandle->data;
   frame->fi->fixCount++;

   //in LRU with linked list data structure only difference w.r.t FIFO
   // is adjust linked list for next deque
    if (bm->strategy == RS_LRU) { 
        //moving the most recently referenced frame to head for complying
        //with next deque inorder to serve the purpose of LRU
        if (err = BM_Move_To_Front(bdm, bm->strategy, pageNum) != RC_OK)
            return err;
   }

    bm->mgmtData=(BM_MgmtData_Manager *) bdm;
    return RC_OK;
}
