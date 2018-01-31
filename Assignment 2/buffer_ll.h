#include "dberror.h"
#include "dt.h"
#include "buffer_mgr.h"

// stores internal frame data
typedef struct BM_FrameInternal {
    BM_PageHandle *pageHandle;
    bool dirty;
    int fixCount;
} BM_FrameInternal;

/* A linked list for queueing and dequeing 
 * frames for different replacement strategies.
 * contains additional info may be needed by 
 * some replacement strategies.
 */

typedef struct BM_Frame    {
    struct BM_FrameInternal *fi;
    struct BM_Frame *next;
    struct BM_Frame *prev;
    bool cb;    // for "Clock Bit"
} BM_Frame;

/* Structure to hold hash map contents*/
typedef struct BM_HashBookElements  {
    PageNumber key;
    BM_Frame *value;
    struct BM_HashBookElements *next;
} BM_HashBookElements;

typedef struct BM_MgmtData_Manager { 
    BM_Frame *head;
   // Array representing a hashmap
   BM_HashBookElements **HMapTable;
    int maxFrames;
    int availFrames;
    int writeCount;
    int readCount;
} BM_MgmtData_Manager;

//hash function
int BM_pageHash(PageNumber pageNum);

/* Returns a pointer to a frame or NULL 
 * if it does not exist in the buffer */
BM_Frame *lookupBufferFrame(  BM_MgmtData_Manager *bdm,
                        PageNumber pageNum);

//Initialization methods
RC BM_FrameInternal_Init(BM_FrameInternal *fi, BM_PageHandle *pageHandle);
RC BM_Frame_Init(BM_Frame *frame, BM_PageHandle *pageHandle);
RC BM_HashBookElements_Init(BM_HashBookElements *bpfe);
BM_Frame_Archive_Page(BM_Frame *frame, BM_PageHandle *pageHandle);

//Based on replacement strategy init buffer frame manager
RC BM_MgmtData_Manager_Init(   BM_MgmtData_Manager *bdm,
                            int numFrames, 
                            ReplacementStrategy strategy);


//debugging ease functions
RC print_Hashindex(int index, BM_MgmtData_Manager *bdm);
void print_Frame( BM_Frame *frame);
void print_Queue( BM_MgmtData_Manager *bdm);
void print_Qmatch( BM_MgmtData_Manager *bdm, BM_Frame *frame);
RC print_HashMap(BM_MgmtData_Manager *bdm);
