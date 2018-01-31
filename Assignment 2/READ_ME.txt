

Problem Statement   
-----------------

The goal of this assignment is to create a record manager. The record manager handles tables with a fixed schema. 
Clients can insert records, delete records, update records, and scan through the records in a table. A scan is 
associated with a search condition and only returns records that match the search condition. Each table should 
be stored in a separate page file and record manager should access the pages of the file through the buffer 
manager implemented in the last assignment.


How To Run The Script
---------------------


With default test cases:
------------------------
Compile : make assign3_1
Run : ./test_assign3_1

Compile : make expr
Run : ./expr

With additional test cases:
--------------------------
Compile : make assign3_2
Run : ./assign3_2


To revert:
----------
On terminal : make clean



Logic
-----
Data Structures and Design :
————————————————————————————

For the record manager,a structure called tableInfo is implemented. The structure has:
*schemaLength    - the length of the schema.
*recordStartPage - page number of the first record in a table
*recordLastPage  - page number of the last record in a table.
*numTuples       - number of tuples in a table
*slotSize        - size of a slot holding a record.
*maxSlots        - maximum number of slots.
*tstone_root     - the head node of the tstone list.

For scanning through a table to fetch records matching certain criteria,a structure called recordInfo is implemented.The structure has:
*cond     - condition defined by the client.
*curSlot  - current slot of a record.
*curPage  - current page of a record.
*numPages - total number of pages.
*numSlots - total number of slots.

To track a deleted record,a structure called recordInfo tNode is implemented.The structure has:

id   - stores page id, slot id and tombstone id of a record.
next - stors the next node in the linked list.

Table and Record Manager Functions :
-----------------------------------

initRecordManager:


shutdownRecordManager:
*This function frees the resources assigned to record manager.

createTable:
*creates the table with given name.
*checks if the table file already exists.Throws an error in that case.
*writes the tableInfo on the page 0 and schema on page 1 if new file is created.
*The recordStartPage is set to page 2.

openTable:
*opens the file with given table name.
*checks if the file already exists.Throws an error if not.
*initializes the buffer manager with given filename.
*It then reads the page 0 and page 1, and loads the tableInfo and schema into memory.

closeTable:
*closes the file and the buffermanager of a given file.
*throws an error if file doesn't exist
*frees all the resources assigned with table.

deleteTable:
*deletes the table file.
*throws error if the table doesn't exist.
*deletes the file,and destroys the buffer manager associated with it.

getNumTuples:
*stored in memory as part of tableInfo.
*stored on page 0 when written to file, and loaded to memory.
*updated every time a successful insert and delete is called.


Record Functions :
------------------

insertRecord:
*checks to see if there are any RID's in the tstone list. If there are, one of these are used 
and the record's attributes are updated accordingly.
*if not,a new slot's value must be computed. If the new slot's location is equal 
to the maximum number of slots for a page, a new page is created, otherwise the current page is used. The record's values
for page and slot are updated accordingly.
*serializeRecord is used to get the record in the proper format.
* the buffer pool is updated using pinPage, markDirty, unpinPage, and forcePage.Also,the number of 
tuples are increased, and the resulting table is written to file.

deleteRecord:
*checks to see if the tstone list is empty. If it is, it simply creates a new tNode, updates 
its contents with the values from RID, and adds it to the tstone list.
*If the list is not empty, then tstone_iter is used to go to the end of the list and add a new tNode 
(with RID's contents) there.
*Finally, the number of tuples are reduced and the resulting table is written to file.

updateRecord:
*serializeRecord is used to get the record in the proper format.
*The functions pinPage, markDirty, unpinPage, and forcePage are then used to update the buffer pool, and the 
resulting table is written to file.

getRecord:
*uses the given RID value to return a record to the user.
*checks to make sure that the RID is not in the tstone list. If not,another check is used to see if the tuple number 
is greater than the number of tuples in the table (this is an 
error and so RC_RM_NO_MORE_TUPLES is returned).
*pinPage and unpinPage is used to update the buffer pool.
*deserializeRecord is used to obtain a valid record from the record string which was retrieved. 
record->data is then updated accordingly.

Scan Functions:
---------------

startScan:
*initializes the RM_ScanHandle data structure passed as an argument to it.
*initializes a node storing the information about the record to be searched and the condition to be evaluated.
*the node initialed in step 2 is assigned to scan->mgmtData.

next:
*starts by fetching a record as per the page and slot id.
*checks tombstone id for a deleted record.
*it checks for the slot number of the record to see if it 
is the last record if the bit is set and the record is a deleted one.
*slot id is set to be 0 to start  next scan from the begining of the next page in case of the last record.
*the slot number is increased by one to proceed to the next record in case the record is not the last one.
*updated record id parameters are assigned to the scan mgmtData and next function is called.
*the given condition is evaluated to check if the record 
is the one required by the client after verifying the tombstone parameters of the record. 
*if the record fetched is not the required one then next function is called again with the updated record id parameters.
*it also returns RC_RM_NO_MORE_TUPLES once the scan is complete and RC_OK otherwise if no error occurs.


closeScan:
*set scan handler to be free indicating the record manager that all associated resources are cleaned up.

Schema Functions:
-----------------

getRecordSize:
*returns the size of the record.

createSchema:
*creates the schema object an assigns the memory.


freeSchema:
*This function frees all the memory assigned to schema object :
   a. DataType
   b. AttributeNames
   c. AttributeSize

Attribute Functions
-----------------

createRecord:
*memory allocation takes place for record and record data for creating a new record. Memory 
allocation happens as per the schema.

freeRecord:
*free the memory space allocated to record and its data.

getAttr:
*starts by allocating the space to the value data structre where the attribute values are to be fetched.
*attrOffset function is called to get the offset value of different attributes as per the attribute numbers.
*attribute data is assigned to the value pointer as per different data types. Therefore, 
fetching the attribute values of a record.

setAttr:
*starts by calling the attrOffset function to get the offset value of different attributes as 
per the attribute numbers.
*attribute values are set with the values provided by the client as per the attributes datatype.
*setting the attribute values of a record.


Helper Functions:
-----------------

tableInfoToFile :
*writes the tablenInfo to the file.
*tableInfo is written on page 0. The keyinfo is written on the page 2.

deserializeRecord :
*reads the record from the table file, parses it, and returns a record data object.

deserializeSchema :
*reads the schema from the table file, parses it, and returns a schema object.

slotSize :
*calculates the slot size required to write one record on the file based on the serializeRecord
function.

tableInfoToStr and strToTableInfo :
*converts the tableInfo to a string to write on the file, and converts the read data from the file
to tableInfo object.

keyInfoToStr and strToKeyInfo :
*converts the keyAttributeInfo to a string to write on the file, and converts the read data from the file
to keyList.



Additional Features
-------------------


Tombstones :
-----------

*Tombstones are stored in a linked list tableInfo->tstone_root. The list consists of tNodes where each tNode 
cotains an RID and a pointer to the next tNode. This list is used in the record functions.

*Furthermore, an RID has another attribute, namely tstone (boolean), which is true for the given RID if that 
RID is a tombstone. This helps the scan functions to easily check to see which values need to be skipped when 
traversing records. The RID struct was altered in tables.h.

Additional Error checks :
-------------------------
Below error cases are checked and tested :
*Create a table which already exists.
*Open or close a table which does not exist.
*Close a table which is not open.
*delete or update a record which does not exist.




   
