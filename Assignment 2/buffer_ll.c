#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dt.h"
#include "dberror.h"

#include "storage_mgr.h"
#include "buffer_ll.h"
#include "test_helper.h"

void print_Frame(BM_Frame *f);

//Debugging ease: prints all the frames in given Hash key(bucket)
RC print_Hashindex(int index, BM_MgmtData_Manager *bdm) {

    if (bdm == NULL || bdm->head == NULL || bdm->HMapTable == NULL)
        return RC_BM_INVALID_FRAME_MANAGER;
    BM_HashBookElements *elt = bdm->HMapTable[index];
    while (elt != NULL) {
    printf ("HMapTable[%d]\n stores frame number :%d\n", index, elt->value->fi->pageHandle->pageNum);
    elt = elt->next;
    }

 return RC_OK;

}

//Debugging ease: prints the entire hash map @ given
//point of time to monitor/debug hashmap
RC print_HashMap(BM_MgmtData_Manager *bdm) {
    int i = 0;
     if (bdm == NULL || bdm->head == NULL || bdm->HMapTable == NULL)
        return RC_BM_INVALID_FRAME_MANAGER;

    for (i = 0; i < bdm->maxFrames; i++) {
    if (bdm->HMapTable[i]->value == NULL) 
       continue;
    BM_HashBookElements *elt = bdm->HMapTable[i];
    printf("\n\nbucket %d:",i);
    while(elt != NULL)
        {
        printf("\nkey :%d Frame Number:%d",elt->key,elt->value->fi->pageHandle->pageNum);
        elt = elt->next;
        }
    }
    return RC_OK;
}

//Debugging ease: prints the entire queue with base address of queue
void print_Queue(BM_MgmtData_Manager *bdm) {

    ASSERT_TRUE((bdm != NULL), "bdm is not NULL");
    ASSERT_TRUE((bdm->head != NULL), "bdm->head is not NULL");
    BM_Frame * cur;
    cur  = bdm->head;
    while (cur != NULL) {
    printf("cur address is:%d pageNum:%d data:%s\n", cur, cur->fi->pageHandle->pageNum, cur->fi->pageHandle->data);
    cur = cur->next;
    }
}

//Debugging ease: to check if frame is present in queue(linked list)
void print_Qmatch(BM_MgmtData_Manager *bdm, BM_Frame *frame) {
    //ASSERT_TRUE((bdm != NULL), "bdm is NULL");
    ASSERT_TRUE((bdm->head != NULL), "bdm->head is NULL");
    //ASSERT_TRUE((frame != NULL), "bdm is NULL");
    BM_Frame * cur = bdm->head;
    while (cur != NULL && cur->fi->pageHandle->pageNum != frame->fi->pageHandle->pageNum){
        cur = cur->next;
  }
    ASSERT_TRUE((cur->fi->pageHandle->pageNum == frame->fi->pageHandle->pageNum),"match found success");
}

//debugging ease: to check frame contents at any given point of time.
void print_Frame(BM_Frame *f) {
    printf("=============Inside print Frame=========\n");
    if (f == NULL)
	printf ("Frame with NULL content\n");
    if (f->fi == NULL)
        printf ("Frame info NULL content\n");
    if (f->fi->pageHandle == NULL)
        printf ("frame with invalid page content\n");
    printf("Frame contents:\n");
    printf("Frame number:%d, Frame content:%s\n",f->fi->pageHandle->pageNum, f->fi->pageHandle->data);
    printf("===============End of print Frame============\n");

}

BM_Frame * deque(BM_MgmtData_Manager *bdm, ReplacementStrategy strategy);

//Hash table entry
RC BM_HashBookElements_Add(BM_MgmtData_Manager *bdm, BM_Frame *frame) {

    int bucket, err = RC_OK;;
    BM_HashBookElements  *elt, *new_elt;
    if (bdm == NULL)
        return RC_BM_INVALID_FRAME_MANAGER;
    if (frame == NULL)
         return RC_BM_INVALID_FRAME;
    if (frame->fi == NULL)
        return RC_BM_INVALID_FRAME_INFO;
    if (frame->fi->pageHandle == NULL || frame->fi->pageHandle->data == NULL)
        return RC_BM_INVALID_PAGE_HANDLE;
    if (lookupBufferFrame(bdm, frame->fi->pageHandle->pageNum) != NULL)
        return RC_OK;
    //calculating bucket/ hash value using X*X%total number of pages 
    //from hash function BM_pageHash
    bucket = BM_pageHash(frame->fi->pageHandle->pageNum) % bdm->maxFrames;
    
    if (bdm->HMapTable[bucket] == NULL) {
        bdm->HMapTable[bucket] = (BM_HashBookElements *)malloc(sizeof(BM_HashBookElements));
        if (bdm->HMapTable[bucket] == NULL)
            return RC_MALLOC_FAILED;
        if (err = BM_HashBookElements_Init(bdm->HMapTable[bucket]) != RC_OK)
            return err;
    }
    elt = bdm->HMapTable[bucket];
    // Init new hash book element to be stored in hash map
    new_elt = (BM_HashBookElements *) malloc(sizeof(BM_HashBookElements));
    if (new_elt == NULL)
        return RC_MALLOC_FAILED;
    new_elt->key = bucket;
    new_elt->value = frame;
    new_elt->value->fi->pageHandle->pageNum = frame->fi->pageHandle->pageNum;
    new_elt->value->fi->pageHandle->data = frame->fi->pageHandle->data;
    
     //not first element in bucket i.e. first element of linked list on index=bucket
    //so making it to point to elt is fine or else
    //we end up having a dummy hashbookelement with value=-1

    if (elt->key != -1)
    {
        new_elt->next = elt;
    }
    else
    {
        new_elt->next = NULL;
    }
    //newly added element forms the first element of linked list
    //tied to index= bucket
    *(bdm->HMapTable + bucket) = new_elt;

    return RC_OK;
}

/*enqueue on head no matter if its FIFO or LRU
 *in case of LRU only difference in queueing process
 *lies in; when we re-reference a already cached frame
 *in which case we move that frame to head. While dequeing
 *in both the cases we remove element from tail side.
 * With this implementation we get the exact frames for both
 *the replacement strategies.
 */

RC enqueue( BM_MgmtData_Manager *bdm,
        ReplacementStrategy strategy,
            BM_Frame *frame)  {

    if (bdm == NULL) return RC_BM_INVALID_FRAME_MANAGER;
    if (frame == NULL) return RC_BM_FRAME_NOT_FOUND;
    BM_Frame *h_frame = bdm->head;

    RC err = RC_OK;
    if(strategy == RS_LRU || strategy == RS_FIFO) {
     bdm->head = frame;
    if (h_frame == NULL) {
        bdm->head->prev = NULL;
        bdm->head->next = NULL;
    } else if (h_frame!= NULL) {
        bdm->head->next = h_frame;
        h_frame->prev = bdm->head;
    }
    bdm->head->prev =  NULL;
    if(err = BM_HashBookElements_Add(bdm, frame) != RC_OK) {
        return err;
    }
  }
    return RC_OK;

}


//for complying LRU requirement on our simple data structure
RC BM_Move_To_Front(BM_MgmtData_Manager *bdm,
		      ReplacementStrategy strategy,
 		        PageNumber pageNum) {
    BM_Frame *frame , *frame1, *cur, *cur1 ;
    RC err = RC_OK;
    if (bdm == NULL) {
        return RC_BM_INVALID_FRAME_MANAGER;
    }
    cur =  bdm->head;
    cur1 = NULL;
    
    frame = lookupBufferFrame(bdm, pageNum);

    if (frame == NULL)
       return RC_BM_FRAME_NOT_FOUND;
     frame1 = (BM_Frame *)malloc(sizeof(BM_Frame));
     if (frame1 == NULL)
        return RC_MALLOC_FAILED;
     if (err = BM_Frame_Archive_Page(frame1, frame->fi->pageHandle)!= RC_OK){
       return err;
   }

    // remove found frame and make other link adjustments.
    while (cur != NULL && cur->fi->pageHandle->pageNum != frame->fi->pageHandle->pageNum) {
        cur1 = cur;
        cur = cur->next;
    }
     
    if (cur != bdm->head) {
    if (cur == NULL) {
        return RC_BM_FRAME_NOT_FOUND;
    }
    
   if (cur->next != NULL && cur->prev != NULL) {
   
         cur1->next = cur->next;
         (cur->next)->prev = cur1;
   }
   if (cur->next == NULL) {
       cur1->next = NULL;

   } 
    
    if (err = enqueue(bdm, strategy, frame1)!= RC_OK)
         return RC_OK;
  }

     return RC_OK;
}
bool isDirty(BM_Frame *frame) {

    return frame->fi->dirty;

}

//deleting frame book keeping data when it is deque-ed as part of 
//replacement algorithm.
RC BM_HashBookElements_Del(BM_MgmtData_Manager *bdm, PageNumber pageNum) {

    RC err = RC_OK;
    int bucket;
    if (bdm == NULL)
        return RC_BM_INVALID_FRAME_MANAGER;
    if (pageNum < 0)
        return RC_BM_INVALID_PAGE_NUM;
    bucket = BM_pageHash(pageNum) % bdm->maxFrames;
    BM_HashBookElements * elt, *elt_previous;
    elt_previous = NULL;
    elt = *(bdm->HMapTable + bucket);
    //print_Hashindex(bucket, bdm);
    while (elt != NULL) {
        if (elt->value->fi->pageHandle->pageNum == pageNum) {
            if (elt_previous == NULL && elt->next != NULL) {
                *(bdm->HMapTable + bucket) = elt->next;
            }
            else if (elt_previous == NULL && elt->next == NULL) {
                 if (err = BM_HashBookElements_Init(elt) != RC_OK)
                    return err;
                 return RC_OK;
            }     
            else if(elt_previous != NULL) {
                     elt_previous->next = elt->next;
                     free(elt);
           }
            return RC_OK;
        }
         elt_previous = elt;
         elt = elt->next;
    }
//return NULL if pageframe not found in main memory
     return RC_BM_INVALID_PAGE_HANDLE;
 }
     
//deleting internal frame data
RC BM_FrameInternal_Del(BM_FrameInternal *fi) {

    RC err = RC_OK;

    if (fi == NULL || fi->pageHandle == NULL){
        return RC_BM_INVALID_FRAME_INFO;
   }
   if (fi->pageHandle->data == NULL) {
     free(fi->pageHandle->data);
   }
    free(fi->pageHandle);
    free(fi);
    return RC_OK;

}
// freeing the frame now
RC BM_FrameElt_Free( BM_Frame *frame) {

    RC err = RC_OK;

    if (frame == NULL) {
        return RC_BM_FRAME_NOT_FOUND;
    }
    if (err = BM_FrameInternal_Del(frame->fi) != RC_OK) {
        return err;
    }
    free(frame);
    return RC_OK;
}
//for flushing the buffer pool deleting mgmtData manager 
RC BM_MgmtData_Manager_Wipe(BM_MgmtData_Manager *bdm, ReplacementStrategy strategy) {

    int i;
    RC err = RC_OK;
    if (bdm == NULL)
        return RC_BM_INVALID_FRAME_MANAGER;
    BM_Frame *frame;
    BM_HashBookElements *elt, *temp_elt;
    frame = bdm->head;
    while (bdm->head != NULL) {
        frame = deque(bdm, strategy);
        if (frame == NULL || frame->fi == NULL || frame->fi->pageHandle == NULL) {
            return RC_BM_FRAME_NOT_FOUND;
        }
        if (err = BM_HashBookElements_Del(bdm, frame->fi->pageHandle->pageNum) != RC_OK)
            return err;
        if (err = BM_FrameElt_Free(frame) != RC_OK)
            return err;
        }
      

     for(i = 0; i < bdm->maxFrames; i++) {
        elt = bdm->HMapTable[i];
        while(elt != NULL)  {
            temp_elt = elt;
            elt = elt->next;
            free(temp_elt);
        }
    }
    free(bdm->HMapTable);

    free(bdm);

    return RC_OK;

}


// no matter what algorithm is used for replacement only deque from tail
// or rear end.
BM_Frame *deque(BM_MgmtData_Manager *bdm, ReplacementStrategy strategy)   {
 if(bdm == NULL || bdm->head == NULL)   return NULL;

    SM_FileHandle fHandle;
    BM_Frame *last = NULL, *cur1 = NULL, *trav = bdm->head,  *cur, *remove;
    cur = bdm->head;
if(strategy == RS_LRU || strategy == RS_FIFO){
 if (cur == NULL) {
        return NULL;
    }
 while (cur->next != NULL) {

        last = cur;
        cur = cur->next;
    }

 /*we need to dequeue from tail but if and
  *only if its fixcount = 0 so move from
  *tail to head now looking for frame with 
  *fixcount 0.
  */

 while (cur != NULL && cur->fi->fixCount > 0) {
        cur1 = cur;
        cur = cur->prev;
    }
    if (cur == NULL) {
        printf("went past first one\n");
        last->next = NULL;
        return cur;
    }
    if (cur->prev == NULL) {
        remove = bdm->head;
      bdm->head = remove->next;
        remove->prev = NULL;
        return remove;
    }
    if (cur->next == NULL){
         last->next = NULL;
         return cur;
    } else {
     trav = cur->prev;
     trav->next = cur->next;
     (trav->next)->prev = trav;
     return cur;
  }
}
    return NULL;

}                            


//evicting frames cached from memory when availframes is 0
RC evict(BM_MgmtData_Manager *bdm, ReplacementStrategy strategy, SM_FileHandle *fHandle) {

    RC err = RC_OK;
    if (bdm == NULL)
        return RC_BM_INVALID_FRAME_MANAGER;
    if (fHandle == NULL)
        return RC_FILE_HANDLE_NOT_INIT;

    BM_Frame *frame;
   //for clock implementation later have this infra
    switch (strategy) {

        case RS_FIFO:
        case RS_LRU:
                    frame = deque(bdm, strategy);
                    break;
    }

    if (frame == NULL || frame->fi == NULL || frame->fi->pageHandle == NULL) {
        return RC_BM_FRAME_NOT_FOUND;
    } 
       
    
    //if marked dirty(modified in theory) 
    //then we must write the content to disk before 
    //removing from main memory
    if (isDirty(frame) == TRUE) {
        writeBlock( frame->fi->pageHandle->pageNum,
                    fHandle,frame->fi->pageHandle->data);
        bdm->writeCount++;
        frame->fi->dirty = FALSE;
    }

    // remove this frame from hash map now.
    
    if (err = BM_HashBookElements_Del(bdm,frame->fi->pageHandle->pageNum )!= RC_OK) {
          return err;
    }
    if (err = BM_FrameElt_Free(frame) != RC_OK) {
         return err;
     }
    
    bdm->availFrames++;
    return RC_OK;
}
//Archiving internal info of page to frame

RC BM_FrameInternal_Init(BM_FrameInternal *fi,  BM_PageHandle *pageHandle) {

    RC err = RC_OK;
    if(pageHandle == NULL || pageHandle->data == NULL)
        return RC_BM_INVALID_PAGE_HANDLE;
    if(pageHandle->pageNum < 0)    return RC_BM_INVALID_PAGE_NUM;
    fi->pageHandle = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
    fi->pageHandle->pageNum = pageHandle->pageNum;
    fi->pageHandle->data = pageHandle->data;
    fi->dirty = FALSE;
    fi->fixCount = 0;

    return RC_OK;
}


//Archiving new page into frame i.e. first read into memory

RC BM_Frame_Archive_Page(BM_Frame *frame, BM_PageHandle *pageHandle ) {
    RC err = RC_OK;
    if (pageHandle == NULL || pageHandle->data == NULL)
        return RC_BM_INVALID_PAGE_HANDLE;
    if (pageHandle->pageNum < 0)
        return RC_BM_INVALID_PAGE_NUM;

    frame->fi = (BM_FrameInternal *)malloc(sizeof(BM_FrameInternal));
    if(frame->fi == NULL)
        return RC_MALLOC_FAILED;
    if(err = BM_FrameInternal_Init(frame->fi, pageHandle) != RC_OK) {
        return err;
    }
      frame->next = NULL;
      frame->prev = NULL;
      frame->cb = 1;
      return RC_OK;
}
/*Initialize individual page frame unit */
RC BM_HashBookElements_Init(BM_HashBookElements * bpfe) {

    bpfe->key = -1;
    bpfe->value = NULL;
    bpfe->next = NULL;
    return RC_OK;
}

/* Initialize Frame Manager based on stragegy */
RC BM_MgmtData_Manager_Init(   BM_MgmtData_Manager *bdm,
                            int numFrames,
                            ReplacementStrategy strategy)  {


    int i;
    if(numFrames < 1)   return RC_BM_INVALID_NUM_FRAMES;

    RC err = RC_OK;

    bdm->maxFrames = bdm->availFrames = numFrames;
    bdm->writeCount = bdm->readCount = 0;
    bdm->head = NULL;
    bdm->HMapTable = (BM_HashBookElements **)malloc(sizeof(BM_HashBookElements)*numFrames);
    if (bdm->HMapTable == NULL)
        return RC_MALLOC_FAILED;

 // init all the keys and corresponding chain in hashmap

    for (i=0; i<numFrames; i++) {
        bdm->HMapTable[i] = (BM_HashBookElements *)malloc(sizeof(BM_HashBookElements));
	if (bdm->HMapTable[i] == NULL)
	   return RC_MALLOC_FAILED;
        if (err = BM_HashBookElements_Init(bdm->HMapTable[i]) != RC_OK)
            return err;
    }

    return RC_OK;
}

