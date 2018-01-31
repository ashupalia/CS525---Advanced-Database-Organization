1. Problem Statement

	The goal of this assignment is to implement a simple storage manager - a module that is capable of reading blocks 
	from a file on disk into memory and writing blocks from memory to a file on disk. The storage manager deals with 
	pages (blocks) of fixed size (PAGE_SIZE). In addition to reading and writing pages from a file, it provides methods 
	for creating, opening, and closing files. The storage manager has to maintain several types of information for an 
	open file: The number of total pages in the file, the current page position (for reading and writing), the file 
	name, and a POSIX file descriptor or FILE pointer.
	
2. Steps for running the script 

        2.1 To run the test cases we need to go to the assign1 directory
            run mk
            run ./test_assign1
        2.2 Additional test cases
            after all the existing test cases are run the program is expected to halt for 5 seconds and display, post which it will run rest added cases:-

            BASIC TEST CASES DONE NOW SLEEP FOR 5 seconds and additional testcases wil run
        2.3  Append 1st page and assert the current page postion and total number of pages
	     Append 2nd page and assert the current page postion and total number of pages
              
            
 		

3. Logic
	
	3.1 manipulating page files 	
	
		3.1.1 initStorageManager (void); 
		3.1.2 fileExists(char *fileName) 
				// The function checks for the existance of the file and checks for its accessibility.
		3.1.3 assertHandle(SM_FileHandle *fHandle) 
				// The function checks whether the file handler is initialised or not.
		3.1.4 assertMempage(SM_PageHandle memPage) 
				// The function checks whether the page handler is initialised or not.
		3.1.5 createPageFile (char *fileName); 
				// The function creates a new page file of size PAGE_SIZE and initialises its contents to '\0' 
				   else overrides if the pagefile is already present. 
		3.1.6 openPageFile (char *fileName, SM_FileHandle *fHandle); 
				// Opens a file it exists or else it will return an error. 
				   Also it initialises the data members of the file handler. 
		3.1.7 closePageFile (SM_FileHandle *fHandle); 
				// Closes the file and clears the memory associated with that file. 
		3.1.8 destroyPageFile (char *fileName); 
				// Destroies the file if the file of that name exists. 
                3.1.9 assertPageBounds(int PageNum, int lowerBound,int upperBound, SM_FileHandle *fHandle)
				//asserts upper and lower bound of the file
	
	3.2 reading blocks from disc
		
		3.2.1  startRead(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function does all the heavy lifting of the read functions.
		3.2.2  assertInit(SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function checks whether the file handler and page handler are initialised or not.
		3.2.3  readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function reads the block present in the file specified by the pageNum.
		3.2.4  assertPageBounds(int PageNum, int lowerBound, int upperBound, SM_FileHandle *fHandle) 
					// The function asserts whether the pageNum falls within the range of lowerBound and Upperbound
		3.2.5  getBlockPos (SM_FileHandle *fHandle) 
					// The function gets the value of current page position pointed by the file handler.
		3.2.6  readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function reads the first block present in the file specified by the file handler, 
					   as opposed to the readBlock function where the pageNum is implicitly passed.
		3.2.7  readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function reads the block previous to the block specified by the current page position of the file handler, 
					   as opposed to the readBlock function where the pageNum is implicitly passed.
		3.2.8  readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function reads the block specified by the current page position of the file handler, 
					   as opposed to the readBlock function where the pageNum is implicitly passed.
		3.2.9  readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function reads the block next to the block specified by the current page position of the file handler, 
					   as opposed to the readBlock function where the pageNum is implicitly passed.
		3.2.10 readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function reads the last block present in the file specified by the file handler, 
					   as opposed to the readBlock function where the pageNum is implicitly passed.
	
	3.3 writing blocks to a page file
	
		3.3.1 writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function writes from the data present in the page handler memPage to 
					   the block specified by pageNum.
		3.3.2 startWrite(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage, char *mode) 
					//	The function does all the heavy lifting of the write functions.
		3.3.3 writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
					// The function writes from the data present in the page handler memPage to the block specified 
					   by the current page position of the file handler and overwrites the data present in the block.
		3.3.4 appendEmptyBlock (SM_FileHandle *fHandle) 
					// The function appends an empty block at the end of the file specified by the file handler. 
		3.3.5 ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) 
					// The function checks whether the file has at least numberOfPages Pages and appends empty blocks until the limit is met.
	
4. New Errors introduced.

	4.1 RC_FILE_REMOVE_FAILED
	4.2 RC_FILE_INIT_FAILED
	4.3 RC_FILE_CLOSE_ERROR
	4.4 RC_READ_ERROR
	4.5 RC_FILE_CLOSE_ERROR
	4.6 RC_FILE_CLOSE_ERROR
	4.7 RC_FILE_REMOVE_FAILED
	4.8 RC_READ_ERROR
	4.9 RC_WRITE_OUT_OF_BOUND_INDEX
	4.10 RC_WRITE_OUT_OF_BOUND_INDEX
	4.11 RC_WRITE_OUT_OF_BOUND_INDEX


							