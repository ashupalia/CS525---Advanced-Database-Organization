
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"
extern void  initStorageManager(void) {

}
/*To check if the file exists.*/

extern int fileExists(char *fileName) {
	
	if ((access(fileName, F_OK)) != -1) 
	     return 1;
	 
    return 0;
}

//Asserting initialization of file handler.
extern assertHandle(SM_FileHandle *fHandle) {
	
	FILE *f;
	if (fHandle == NULL)
	    return RC_FILE_HANDLE_NOT_INIT;
	f = (FILE *) fHandle->mgmtInfo;
	if (f == NULL) {
                printf (" file not found");
		return RC_FILE_NOT_FOUND;
        }
	
	return RC_OK;
}

//Asserting initialization of Page handler.
extern assertMempage(SM_PageHandle memPage) {
    if(memPage == NULL) return RC_MEMPAGE_NOT_INIT;
    return RC_OK;
}
	
//Create a page file.
extern RC createPageFile(char *fileName)   {

    FILE *f;
    char *initpage;
    int bytes_streamed = 0;
    if (fileExists(fileName))   
        if (remove(fileName)!= 0)
            return RC_FILE_REMOVE_FAILED;

    f = fopen(fileName, "w+");

    if (f == NULL)
        return RC_FILE_INIT_FAILED;
	//Overwrites a file if it is present.
    initpage = (char *)calloc(PAGE_SIZE, sizeof(char));
    if ((bytes_streamed =
            fwrite(initpage, sizeof(char), PAGE_SIZE,f))< PAGE_SIZE)
        return RC_WRITE_FAILED;
	
	if (fflush(f)!= 0) 
		return RC_WRITE_FAILED;

	if(fclose(f)!= 0)    
	    return RC_FILE_CLOSE_ERROR;
	
	//deallocating memory once all the other operations are completed
	free(initpage);

	return RC_OK;

}
	
//Opens a file if it exists otherwise returns an error.	
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle) {

	FILE *f;

	if (!fileExists(fileName))
		return RC_FILE_NOT_FOUND;

	if (fHandle == NULL) 
		return RC_FILE_HANDLE_NOT_INIT;

	f = fopen(fileName, "r+");

	if (f == NULL) 
		return RC_FILE_NOT_FOUND;
	
	/*initializes the data members of the file handle*/
	fHandle->fileName = (char *) malloc(sizeof(char)*strlen(fileName)+1);
	if (fHandle->fileName == NULL) 
		return RC_MALLOC_FAILED;
	strcpy(fHandle->fileName, fileName);
	if (fseek(f, 0L, SEEK_SET)!= 0) 
		return RC_READ_ERROR;
	fseek(f, 0, SEEK_END);
	/*calculates the total number of pages by dividing total number of bytes by PAGE_SIZE*/
	fHandle->totalNumPages = (int) (ftell(f)/PAGE_SIZE);
	rewind(f);
 
	fHandle->curPagePos = 0;
	fHandle->mgmtInfo = f;
 
	return RC_OK;
 
}

//Closes the file and clears the memory associated with that file.
extern RC closePageFile (SM_FileHandle *fHandle)    {
 
	FILE *f;
	
	if(fHandle == NULL) 
	    return RC_FILE_HANDLE_NOT_INIT;
	
	f = (FILE *)fHandle->mgmtInfo;
	
	if (f == NULL) 
		return RC_FILE_NOT_FOUND;
	if (fclose(f)!= 0)
		return RC_FILE_CLOSE_ERROR;
	//clears the memory associated with the file.
	free(fHandle->fileName);
	
	return RC_OK;
}

//Destroys the file, if the name exists.	
extern RC destroyPageFile (char *fileName)    {
	FILE *f = fopen(fileName, "r");
	if (f == NULL)
		return RC_FILE_NOT_FOUND;
	if (fclose(f)!=0) 
		return RC_FILE_CLOSE_ERROR;
	
	if (remove(fileName) != 0) 
		return RC_FILE_REMOVE_FAILED;
}

//Using startRead function for heavy lifting of read intensive functions..
extern RC startRead(int pageNum, 
					SM_FileHandle *fHandle, SM_PageHandle memPage) {
	
	FILE *f = (FILE *)fHandle->mgmtInfo;
	int seekSuccess = 0;
	int bytes_accessed = 0;
	if (f == NULL) 
	    return RC_FILE_NOT_FOUND;  
	if (seekSuccess = fseek(fHandle->mgmtInfo, pageNum*PAGE_SIZE, SEEK_SET)!=0)
		return RC_READ_ERROR;
		//Checking if seekSuccess passed.
        if (!seekSuccess) {
	bytes_accessed = fread(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
	if (bytes_accessed < PAGE_SIZE) 
		return RC_READ_ERROR;
        fHandle->curPagePos = pageNum;
        printf("%d t %d c after read\n", fHandle->totalNumPages,fHandle->curPagePos);

	} 
	
	return RC_OK;
	
}

//Assert initialization.
extern RC assertInit(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC err = RC_OK;
	if ((err = assertHandle(fHandle))!=RC_OK) 
		return  err;
	if(err = assertMempage(memPage) != RC_OK)
		return err;
	
	return err;
}

extern RC readBlock (int pageNum, 
					SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC err = RC_OK;
	/*checks whether the file handle and page handle are initialized*/
	err = assertInit(fHandle,memPage);
	if (err != RC_OK) {
	    return err;
	}
	/*checks whether the PageNum falls within the range of lowerBound and upperBound*/
	if (pageNum<0 || pageNum>fHandle->totalNumPages) {
		return RC_READ_NON_EXISTING_PAGE;
	}
	
	err = startRead(pageNum, fHandle, memPage);
	
	return err;
	
}

extern RC assertPageBounds(int PageNum, int lowerBound, 
				int upperBound, SM_FileHandle *fHandle)  {
	
	/*checks whether the PageNum falls within the range of lowerBound and upperBound*/
	if (PageNum < lowerBound || 
		PageNum  > upperBound-1) {
		return RC_READ_NON_EXISTING_PAGE;
	}
	return RC_OK;
}

/*gets the current page position pointed to by the file handle*/
extern int getBlockPos (SM_FileHandle *fHandle) {
	
	int err = RC_OK;
	if (err = assertHandle(fHandle)!= RC_OK) {
		return err;
	}
	
	return (fHandle->curPagePos);
}					


extern RC readFirstBlock (SM_FileHandle *fHandle, 
						      SM_PageHandle memPage) {
	RC err = RC_OK;
	/*checks whether the file handle and page handle are initialized*/
	err = assertInit(fHandle, memPage);
	if (err != RC_OK) 
		return err;
/*  checks whether the total number of pages is atleast 1*/	
	 if(fHandle->totalNumPages < 1)  
		 return RC_READ_NON_EXISTING_PAGE;
	 
	err = startRead(0,fHandle, memPage);
	return err;
	
}


extern RC readPreviousBlock(SM_FileHandle *fHandle, 
								SM_PageHandle memPage) {
	RC err = RC_OK;
	int curPagePos = 0;
	/*checks whether the file handle and page handle are initialized*/
	err = assertInit(fHandle, memPage);
	if (err != RC_OK) 
		return err;
	/*checks the limits for not overstepping previous block bounds.The curPagePos value must be greater than the first page and less than the total number of pages*/
	if(err = assertPageBounds(fHandle->curPagePos,1, fHandle->totalNumPages, fHandle) != RC_OK)
		return err;
	fHandle->curPagePos-=1;
	curPagePos = fHandle->curPagePos;
	err = startRead(curPagePos, fHandle, memPage);
	return err;
}

extern RC readCurrentBlock(SM_FileHandle *fHandle,
								SM_PageHandle memPage) {
									
	RC err = RC_OK;
	int curPagePos = 0;
	/*checks whether the file handle and page handle are initialized*/
	err = assertInit(fHandle, memPage);
	if (err!=RC_OK)
		return err;
	/*checks that the curPagePos value lies between 0 and total number of pages*/
	err = assertPageBounds(fHandle->curPagePos,  0,fHandle->totalNumPages, fHandle);
	if (err!=RC_OK)
		return err;
	curPagePos = fHandle->curPagePos;
	err = startRead(curPagePos, fHandle, memPage);
	return err;
}
extern RC readNextBlock (SM_FileHandle *fHandle, 
							SM_PageHandle memPage) {
	RC err = RC_OK;
	int curPagePos = 0;
	/*checks whether the file handle and page handle are initialized*/
	err = assertInit(fHandle, memPage);
	if (err!=RC_OK)
		return err;
	/*checks that the curPagePos value must be atleast the last but one page,and also atleast greater than 0*/
	err = assertPageBounds( fHandle->curPagePos,0, fHandle->totalNumPages-1 ,fHandle);
	if (err!= RC_OK)
		return err;
	fHandle->curPagePos += 1;
	curPagePos =fHandle->curPagePos;
	err = startRead(curPagePos, fHandle, memPage);
	return err;
}

extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	
	RC err = RC_OK;
	int curPagePos = 0;
	/*checks whether the file handle and page handle are initialized*/
	err = assertInit(fHandle, memPage);
	if (err!=RC_OK)
		return err;
		/*checks whether the total number of pages is less than one*/
		if(fHandle->totalNumPages < 1)  
		return RC_READ_NON_EXISTING_PAGE;
        curPagePos = fHandle->totalNumPages-1; 
	err = startRead(curPagePos, fHandle, memPage);

	
	if (err != RC_OK)
		return err;
	
}

								
							
extern RC writeBlock (int pageNum, 
					SM_FileHandle *fHandle, SM_PageHandle memPage) {

	RC err = RC_OK;
	/*checks whether the file handle and page handle are initialized*/
	err = assertInit(fHandle,memPage);
	if (err != RC_OK) {
	    return err;
	}
	/*checks whether an attempt is made to write outside the bounds of available pages*/
	if (assertPageBounds(pageNum, 0,fHandle->totalNumPages,fHandle)) {
		return RC_WRITE_OUT_OF_BOUND_INDEX;
	}
	err = startWrite(pageNum, fHandle, memPage,"r+");
	return err;
    
}

/*does all the heavy lifting of all the write intensive operations*/
extern RC startWrite(int pageNum, 
					SM_FileHandle *fHandle, SM_PageHandle memPage, char *mode) {
	
	FILE *f = fopen(fHandle->fileName, mode);
	int bytes_streamed =0;
	if (f == NULL) 
	    return RC_FILE_NOT_FOUND;  
	if (fseek(f, pageNum*PAGE_SIZE, SEEK_SET)!=0)
		return RC_READ_ERROR;
	
	bytes_streamed = fwrite(memPage,sizeof(char),PAGE_SIZE, f);
	if (bytes_streamed != PAGE_SIZE) 
		return RC_WRITE_FAILED;
	if(fclose(f)!=0) 
		return RC_FILE_CLOSE_ERROR;
	
	return RC_OK;
	
}

/*writes to the block pointed to by curPagePos of the file handle,as opposed to writeBlock,where the page number is implicitly specified*/
extern RC writeCurrentBlock(SM_FileHandle *fHandle,
                            SM_PageHandle memPage)  {
    RC err = RC_OK;
    err = assertInit(fHandle,memPage);
	if (err!=0)
		return err;
	/*checks whether an attempt is made to write outside the bounds of available pages*/
	err = assertPageBounds( fHandle->curPagePos,0,fHandle->totalNumPages,fHandle);
	if (err!=0)
		return RC_WRITE_OUT_OF_BOUND_INDEX;
	
    return startWrite(fHandle->curPagePos, fHandle, memPage, "r+");
}

/*appends an empty block to the end of the file and fills it with '\0' */
extern RC appendEmptyBlock (SM_FileHandle *fHandle) {
    RC err = RC_OK;
	int flag = 0;
	/*allocates a memory block of PAGE_SIZE and initializes it to '\0'*/
	char *initpage = (char *)calloc(PAGE_SIZE, sizeof(char));
    if(err = assertHandle(fHandle) != 0)    
		return err;

    /*checks whether the file is non-empty*/
    if(fHandle->totalNumPages < 0)  
		return RC_WRITE_OUT_OF_BOUND_INDEX;

    
   
    err = startWrite(fHandle->totalNumPages,
                        fHandle, initpage, "r+");
	if (err!= RC_OK) 
		return err;
    else {
		fHandle->curPagePos = fHandle->totalNumPages;
		fHandle->totalNumPages++;
		flag = 1;
		printf("\nappendsuccessful\n");
	}
	free(initpage);
    return RC_OK;
}
/*Ensures that the file has atleast numberOfPages pages by appending empty blocks until the limit is met*/
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
    
	RC err = RC_OK;
    	err = assertHandle(fHandle);
	if (err!= RC_OK)
		return err;

    while(fHandle->totalNumPages < numberOfPages)   {
        appendEmptyBlock(fHandle);
    }
	return RC_OK;
}
