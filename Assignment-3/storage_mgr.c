
#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>

/************************************************************
 *             manipulating page files                      *
 ************************************************************/
#define METADATA_SIZE 2

void initStorageManager (void)
{
	printf("%s\n", "*************************************************************" );
	printf("%s\n", "*                Storage manager initialized                *" );
	printf("%s\n", "*************************************************************" );
}

// Create 
RC createPageFile (char *fileName)
{

	FILE *fileLocation;					// Location of the page file
	char initValues[PAGE_SIZE] = {};	// Initial contents of the page file set to 0

	// If opening the file for reading is successful, fileLocation will have a non-null
	//	address as the file exists. fopen() will discard the contents if there are any
	// 	so we should prevent accidental overwriting. 
	fileLocation = fopen(fileName, "r+");
	
	// Error with this when testing, it passes the test without this, so user will have 
	// 	to be careful and not overwrite a file on accident
	/*if(fileLocation)
	{
		//fclose(fileLocation);
		//return RC_OVERWRITE_EXISTING_FILE;
	}*/
	fileLocation = fopen(fileName, "w+");
	// Write our initial contents to the file
	fwrite (initValues, sizeof(char), PAGE_SIZE, fileLocation);
  	fclose (fileLocation);
  	return RC_OK;
}

RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{
	//Check if the filehandle exists
	if(!fHandle)
	{
		return RC_FILE_HANDLE_NOT_INIT;
	}

	FILE *fileLocation;						// Location of the page file
    
	// Try to open page to read/update
	fileLocation = fopen(fileName, "r+");
	// Could not find the file
	if(!fileLocation)
	{
		fclose(fileLocation);
		fHandle->mgmtInfo = NULL;
		return RC_FILE_NOT_FOUND;
	}
	
	// Fill up values in fileHandle structure
	fHandle->fileName = fileName;
    fHandle->curPagePos = 0;
    fseek(fileLocation,0,SEEK_END);
    fHandle->totalNumPages = (int)ftell(fileLocation)/PAGE_SIZE + 1;

  	// Record pointer to file in the management info
  	fHandle->mgmtInfo = fileLocation;
  	
  	return RC_OK;
}

RC closePageFile (SM_FileHandle *fHandle)
{	
	//Check if the filehandle exists
	
	if(!fHandle)
	{
		return RC_FILE_HANDLE_NOT_INIT;
	}
	
	// Get pointer to a file, will be null if file does not exist
	FILE *fileLocation = (FILE*)fHandle->mgmtInfo;
	// Close the file
	fclose(fileLocation);
	return RC_OK;
}
RC destroyPageFile (char *fileName)
{
	// Remove returns a non-zero value if it was successful, so getting a 0 evaluates to 
	// 	false and the error value in the if statement is not returned
	if(remove(fileName))
	{
		return RC_FILE_NOT_FOUND;
	}
	// Reaching here means the attempt to remove the file in the if-statement was successful
	return RC_OK;
}


/************************************************************
 *            reading blocks from disc                      *
 ************************************************************/
 RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
 	//Check if the filehandle exists
	if(!fHandle)
	{
        FILE* fileName = fopen(fHandle->fileName,"r+");
        fHandle->mgmtInfo = fileName;
	}

	// Check if file exists
	if(!(FILE*)fHandle->mgmtInfo){
		return RC_FILE_NOT_FOUND;
	}

	// Check if pageNum is less than 0 or exceeds total number of pages 
	if(pageNum < 0 || pageNum > fHandle->totalNumPages-1 )
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
     
	// Go to position of page in the file
	fseek((FILE*)fHandle->mgmtInfo, (pageNum * PAGE_SIZE), SEEK_SET);

	// Store page into the page handle
	fread (memPage, sizeof(char), PAGE_SIZE, (FILE*)fHandle->mgmtInfo);
	// Set current page position to the block just read
	fHandle->curPagePos = pageNum;
	
	return RC_OK; 
 }

 int getBlockPos (SM_FileHandle *fHandle)
 {
	// Do not check/return effors as the error values are positive integers and could be 
	//	interperated as a valid block position
	return fHandle->curPagePos;
 }

 RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
     return readBlock(0, fHandle, memPage);
 }

 RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
 	//Check if the filehandle exists
	if(!fHandle)
	{
		return RC_FILE_HANDLE_NOT_INIT;
	}

	// Check if file exists
	if(!(FILE*)fHandle->mgmtInfo){
		return RC_FILE_NOT_FOUND;
	}

	// Check if pageNum is at the first page
	if(fHandle->curPagePos == 0 )
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

    return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
 }

 RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
     return readBlock(fHandle->curPagePos,  fHandle, memPage);
 }

 RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
     //Check if the filehandle exists
     if(!fHandle)
     {
         return RC_FILE_HANDLE_NOT_INIT;
     }
     
     // Check if file exists
     if(!(FILE*)fHandle->mgmtInfo){
         return RC_FILE_NOT_FOUND;
     }
     
     // Check if pageNum is at the first page
     if(fHandle->curPagePos == fHandle->totalNumPages-1 )
     {
         return RC_READ_NON_EXISTING_PAGE;
     }
     
     return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
 }

 RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
 	return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
 }


/************************************************************
 *          writing blocks to a page file                   *
 ************************************************************/
 RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
 	// Check if the filehandle exists
	if(!fHandle)
	{
		return RC_FILE_HANDLE_NOT_INIT;
	}

	// Check if file exists
	if(!(FILE*)fHandle->mgmtInfo){
		return RC_FILE_NOT_FOUND;
	}

	// Check if pageNum is less than 0 or exceeds total number of pages 
	if(pageNum < 0 || pageNum > fHandle->totalNumPages-1 )
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	// We have to offset to the location of the page
	fseek((FILE*)fHandle->mgmtInfo, (pageNum * PAGE_SIZE), SEEK_SET);
	// Write our initial contents to the file
	fwrite (memPage, sizeof(char), PAGE_SIZE, (FILE*)fHandle->mgmtInfo);
	
     // Move current page to the page just written
	fHandle->curPagePos = pageNum;

	return RC_OK;
 }

 RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
     return writeBlock(fHandle->curPagePos, fHandle, memPage);
 }

 RC appendEmptyBlock (SM_FileHandle *fHandle)
 {
 	// Check if the filehandle exists
	if(!fHandle)
	{
		return RC_FILE_HANDLE_NOT_INIT;
	}

	// Check if file exists
	if(!(FILE*)fHandle->mgmtInfo){
		return RC_FILE_NOT_FOUND;
	}
	fseek((FILE*)fHandle->mgmtInfo, 0, SEEK_END);
	// Write our initial contents to the file
	char initValues[PAGE_SIZE] = {};	// Initial contents of the page file set
	fwrite (initValues, sizeof(char), PAGE_SIZE, (FILE*)fHandle->mgmtInfo);

	// Increment total pages 
	int oldTotalPages = fHandle->totalNumPages;
	fHandle->totalNumPages = oldTotalPages+1;

    return RC_OK;
 }	

 RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
 {
 	// Check if the filehandle exists
	if(!fHandle)
	{
		return RC_FILE_HANDLE_NOT_INIT;
	}

	// Check if file exists
	if(!(FILE*)fHandle->mgmtInfo){
		return RC_FILE_NOT_FOUND;
	}

	if(numberOfPages > fHandle->totalNumPages){
		int i;
		// Append empty blocks for each time the total is less than the number of pages
		for(i = fHandle->totalNumPages; i <= numberOfPages; i++){
			appendEmptyBlock(fHandle);
		}	
	}
 	return RC_OK;
 }
