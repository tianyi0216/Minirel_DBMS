/** @author
 * Student - id:
 * 
 * The file contains actual implementation of buffer manager class of stage 4 of project.
 * The heap file is responsible storing records and pages of a database efficiently.
*/

#include "heapfile.h"
#include "error.h"

// routine to create a heapfile
/**
 * implemented by Tianyi Xu
 * routine to create a heapfile
 * 
 * @param fileName - the file name for the new heap file
 * @return status of execution, and also create a heapfile with specified fileName
*/
const Status createHeapFile(const string fileName)
{
    File* 		file;
    Status 		status;
    FileHdrPage*	hdrPage;
    int			hdrPageNo;
    int			newPageNo;
    Page*		newPage;

    // try to open the file. This should return an error
    status = db.openFile(fileName, file);
    if (status != OK)
    {
		// file doesn't exist. First create it and allocate
		// an empty header page and data page.
        
        // create the file and open
		status = db.createFile(fileName);
        if (status != OK){
            return status;
        }
        status = db.openFile(fileName, file);
        if (status != OK){
            return status;
        }

        // allocate a header page
        status = bufMgr->allocPage(file, hdrPageNo, newPage);
        if (status != OK){
            return status;
        }
        hdrPage = (FileHdrPage*) newPage;

        // allocate first data page here
        status = bufMgr->allocPage(file, newPageNo, newPage);
        if (status != OK){
            return status;
        }
		newPage->init(newPageNo);
		
        // initialize hdr page's data based on what we got
        hdrPage->firstPage = newPageNo;
        hdrPage->lastPage = newPageNo;
        hdrPage->pageCnt = 1;
        hdrPage->recCnt = 0;
        hdrPage->fileName[fileName.length()] = '\0';

        for(int i = 0; i < fileName.length(); i++){
            hdrPage->fileName[i] = fileName[i];
        }

        // here, we unpin and flush the page since it is no longer in use
        status = bufMgr->unPinPage(file, hdrPageNo, true);
        if (status != OK){
            return status;
        }
        status = bufMgr->unPinPage(file, newPageNo, true);
        if (status != OK){
            return status;
        }

        // we need to flush the file and the close it after we done
        status = bufMgr->flushFile(file);
        if (status != OK){
            return status;
        }
        status = db.closeFile(file);
        if (status != OK){
            return status;
        }
		return status;
    }
    return (FILEEXISTS);
}

// routine to destroy a heapfile
const Status destroyHeapFile(const string fileName)
{
	return (db.destroyFile (fileName));
}

// constructor opens the underlying file
/**
 * implemented by Tianyi Xu
 * constructor opens the underlying file. 
 * 
 * @param fileName - the name of the fileName
 * @param returnStatus - status to return during execution of constructing heapfile
 * @return Nothing by the constructor, but will set returnStatus reference
*/
HeapFile::HeapFile(const string & fileName, Status& returnStatus)
{
    Status 	status;
    Page*	pagePtr;

    cout << "opening file " << fileName << endl;

    // open the file and read in the header page and the first data page
    if ((status = db.openFile(fileName, filePtr)) == OK)
    {   
        // get the header page's information, first get pageNo, then pagePtr
        status = filePtr->getFirstPage(headerPageNo);
        if (status != OK){
            returnStatus = status;
            return;
        }
        status = bufMgr->readPage(filePtr, headerPageNo, pagePtr);
        if (status != OK){
            returnStatus = status;
            return;
        }
        headerPage = (FileHdrPage* ) pagePtr;
        hdrDirtyFlag = false;

        // read in first page now for current page
        curPageNo = headerPage->firstPage;
        status = bufMgr->readPage(filePtr, curPageNo, curPage);
        if (status != OK){
            returnStatus = status;
            return;
        }

        // initialize the information for the heapfile.
        curDirtyFlag = false;
        curRec = NULLRID;
        returnStatus = status;
        return;
    }
    else
    {
    	cerr << "open of heap file failed\n";
		returnStatus = status;
		return;
    }
}

// the destructor closes the file
HeapFile::~HeapFile()
{
    Status status;
    cout << "invoking heapfile destructor on file " << headerPage->fileName << endl;

    // see if there is a pinned data page. If so, unpin it 
    if (curPage != NULL)
    {
    	status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
		curPage = NULL;
		curPageNo = 0;
		curDirtyFlag = false;
		if (status != OK) cerr << "error in unpin of date page\n";
    }
	
	 // unpin the header page
    status = bufMgr->unPinPage(filePtr, headerPageNo, hdrDirtyFlag);
    if (status != OK) cerr << "error in unpin of header page\n";
	
	// status = bufMgr->flushFile(filePtr);  // make sure all pages of the file are flushed to disk
	// if (status != OK) cerr << "error in flushFile call\n";
	// before close the file
	status = db.closeFile(filePtr);
    if (status != OK)
    {
		cerr << "error in closefile call\n";
		Error e;
		e.print (status);
    }
}

// Return number of records in heap file

const int HeapFile::getRecCnt() const
{
  return headerPage->recCnt;
}

// retrieve an arbitrary record from a file.
// if record is not on the currently pinned page, the current page
// is unpinned and the required page is read into the buffer pool
// and pinned.  returns a pointer to the record via the rec parameter
/**
 * implemented by Tianyi Xu
 * retrieve an arbitrary record from a file.
 * if record is not on the currently pinned page, the current page 
 * is unpinned and the required page is read into the buffer pool
 * and pinned.  returns a pointer to the record via the rec parameter
 * 
 * @param rid - a RID class that has record's page number and slot number to retrieve.
 * @param rec - the record to return into
 * @return error status if execution of this code lead to any error, OK otherwise.
 * 
*/
const Status HeapFile::getRecord(const RID & rid, Record & rec)
{
    Status status;

    // cout<< "getRecord. record (" << rid.pageNo << "." << rid.slotNo << ")" << endl;
    // if the page number is the same, just simply get the record from the current page
    if (curPageNo == rid.pageNo){
        status = curPage->getRecord(rid, rec);
    } else{
    // else, we need to read the correct page and then retrieve the record
    
    // first, unpin the current page (if it is not NULL)
        if (curPage != NULL){
            status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
            if (status != OK){
                return status;
            }
        }

        // read the page into the buffer pool has the record
        status = bufMgr->readPage(filePtr, rid.pageNo, curPage);
        if (status != OK){
            return status;
        }
        curPageNo = rid.pageNo;
        curDirtyFlag = false;
        // retrieve the record
        status = curPage->getRecord(rid, rec);
    }
    // check if found record, then set current record the one retireved
    if (status != OK){
        return status;
    }
    curRec = rid;
    return OK;
}

HeapFileScan::HeapFileScan(const string & name,
			   Status & status) : HeapFile(name, status)
{
    filter = NULL;
}

const Status HeapFileScan::startScan(const int offset_,
				     const int length_,
				     const Datatype type_, 
				     const char* filter_,
				     const Operator op_)
{
    if (!filter_) {                        // no filtering requested
        filter = NULL;
        return OK;
    }
    
    if ((offset_ < 0 || length_ < 1) ||
        (type_ != STRING && type_ != INTEGER && type_ != FLOAT) ||
        (type_ == INTEGER && length_ != sizeof(int)
         || type_ == FLOAT && length_ != sizeof(float)) ||
        (op_ != LT && op_ != LTE && op_ != EQ && op_ != GTE && op_ != GT && op_ != NE))
    {
        return BADSCANPARM;
    }

    offset = offset_;
    length = length_;
    type = type_;
    filter = filter_;
    op = op_;

    return OK;
}


const Status HeapFileScan::endScan()
{
    Status status;
    // generally must unpin last page of the scan
    if (curPage != NULL)
    {
        status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
        curPage = NULL;
        curPageNo = 0;
		curDirtyFlag = false;
        return status;
    }
    return OK;
}

HeapFileScan::~HeapFileScan()
{
    endScan();
}

const Status HeapFileScan::markScan()
{
    // make a snapshot of the state of the scan
    markedPageNo = curPageNo;
    markedRec = curRec;
    return OK;
}

const Status HeapFileScan::resetScan()
{
    Status status;
    if (markedPageNo != curPageNo) 
    {
		if (curPage != NULL)
		{
			status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
			if (status != OK) return status;
		}
		// restore curPageNo and curRec values
		curPageNo = markedPageNo;
		curRec = markedRec;
		// then read the page
		status = bufMgr->readPage(filePtr, curPageNo, curPage);
		if (status != OK) return status;
		curDirtyFlag = false; // it will be clean
    }
    else curRec = markedRec;
    return OK;
}


/**
* Implemented by Layton Liu
*
* Scan the file one page at a time. Convert the rid to a pointer to the record 
* data and invoke matchRec() to determine if record satisfies the filter associated
* with the scan. If so, store the rid in curRec and return curRec.
*
* @param outRid - record id for record being inserted
* @return OK if everything went well, else returns an error
*/


const Status HeapFileScan::scanNext(RID& outRid)
{
    Status 	status = OK;
    RID		nextRid;
    RID		tmpRid;
    int 	nextPageNo;
    Record  rec;

    // first check whether the current Page is valid
    if(!curPage) {
        // set last page as curPage
        curPageNo = headerPage->firstPage;

        // if current page is End- EOF
        if (curPageNo == -1){
            return FILEEOF;
        }

        curDirtyFlag = false;

        // set curPage to actual page and read into buffer
        status = bufMgr->readPage(filePtr, curPageNo, curPage);
        if (status != OK) {
            return status;
        }

        // start from first page
        status = curPage->firstRecord(tmpRid);

        // If first page is empty, then no records - EOF
        if (status == NORECORDS){
            return FILEEOF;
        }
        if(status != OK)
            return status;
        curRec = tmpRid;

        // access the first record
        status = curPage->getRecord(curRec, rec);
        if (status != OK){
            return status;
        }

        // check whether the record match
        if (matchRec(rec) == true){
            outRid = curRec;
            return OK;
        }
    }

    // handle the case where the current Page is valid
    while(true){
        // check whether the Page has next record
        status = curPage->nextRecord(curRec, nextRid);

         // handle the case that the current Page has no more records
        if(status != OK){
            // check whether the current Page has a next Page
            Status  pageStatus = NORECORDS;

            // we keep looking for next page until we find the valid one.
            while(pageStatus == NORECORDS){
                // check whether the current Page has a next Page
                status = curPage->getNextPage(nextPageNo);
                if (status != OK){
                    return status;
                }
                if (nextPageNo == -1)
                    return FILEEOF;
                // unpin the page
                status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
                if (status != OK)
                    return status;
            
                curDirtyFlag = false;
                // get the next page
                curPageNo = nextPageNo;
                status = bufMgr->readPage(filePtr, curPageNo, curPage);
                if (status != OK)
                    return status;

                // get the first record of the new Page
                pageStatus = curPage->firstRecord(curRec);
                // we found a valid new page, break
                if (pageStatus == OK) break;
            }
        } else{
            curRec = nextRid;
        }

        // if the current Page has next record, check whether it match
        status = curPage->getRecord(curRec, rec);
        if (status != OK)
            return status;
        
        // check whether the record match
        if(matchRec(rec)){
            outRid = curRec;
            return OK;
        }
    }

}

// returns pointer to the current record.  page is left pinned
// and the scan logic is required to unpin the page 

const Status HeapFileScan::getRecord(Record & rec)
{
    return curPage->getRecord(curRec, rec);
}

// delete record from file. 
const Status HeapFileScan::deleteRecord()
{
    Status status;

    // delete the "current" record from the page
    status = curPage->deleteRecord(curRec);
    curDirtyFlag = true;

    // reduce count of number of records in the file
    headerPage->recCnt--;
    hdrDirtyFlag = true; 
    return status;
}


// mark current page of scan dirty
const Status HeapFileScan::markDirty()
{
    curDirtyFlag = true;
    return OK;
}

const bool HeapFileScan::matchRec(const Record & rec) const
{
    // no filtering requested
    if (!filter) return true;

    // see if offset + length is beyond end of record
    // maybe this should be an error???
    if ((offset + length -1 ) >= rec.length)
	return false;

    float diff = 0;                       // < 0 if attr < fltr
    switch(type) {

    case INTEGER:
        int iattr, ifltr;                 // word-alignment problem possible
        memcpy(&iattr,
               (char *)rec.data + offset,
               length);
        memcpy(&ifltr,
               filter,
               length);
        diff = iattr - ifltr;
        break;

    case FLOAT:
        float fattr, ffltr;               // word-alignment problem possible
        memcpy(&fattr,
               (char *)rec.data + offset,
               length);
        memcpy(&ffltr,
               filter,
               length);
        diff = fattr - ffltr;
        break;

    case STRING:
        diff = strncmp((char *)rec.data + offset,
                       filter,
                       length);
        break;
    }

    switch(op) {
    case LT:  if (diff < 0.0) return true; break;
    case LTE: if (diff <= 0.0) return true; break;
    case EQ:  if (diff == 0.0) return true; break;
    case GTE: if (diff >= 0.0) return true; break;
    case GT:  if (diff > 0.0) return true; break;
    case NE:  if (diff != 0.0) return true; break;
    }

    return false;
}

InsertFileScan::InsertFileScan(const string & name,
                               Status & status) : HeapFile(name, status)
{
  //Do nothing. Heapfile constructor will bread the header page and the first
  // data page of the file into the buffer pool
}

InsertFileScan::~InsertFileScan()
{
    Status status;
    // unpin last page of the scan
    if (curPage != NULL)
    {
        status = bufMgr->unPinPage(filePtr, curPageNo, true);
        curPage = NULL;
        curPageNo = 0;
        if (status != OK) cerr << "error in unpin of data page\n";
    }
}

// Insert a record into the file
// implemented by Dominic Robson
/**
* Implemented by Dominic Robson
*
* Inserts record into a page. This page may be the current page
*       or a new page. If it is a new page it will be inserted 
*       into the heap file
*
* @param rec - record to add
* @param outRid - record id for record being inserted
* @return OK if everything went well, else returns an error
*/
const Status InsertFileScan::insertRecord(const Record & rec, RID& outRid)
{
    Page*	newPage;
    int		newPageNo;
    Status	status;
    // RID		rid;

    // check for very large records
    if ((unsigned int) rec.length > PAGESIZE-DPFIXED)
    {
        // will never fit on a page, so don't even bother looking
        return INVALIDRECLEN;
    }

    if(!curPage) {
        // set last page as curPage
        curPageNo = headerPage->lastPage;

        // set curPage to actual page and read into buffer
        status = bufMgr->readPage(filePtr, curPageNo, curPage);
        if (status != OK) {
            return status;
        }
        // set curPage to page returned by readpage
        // curPage = newPage;
        curDirtyFlag = false;
    }
    
    status = curPage->insertRecord(rec, outRid);
  
    if(status != OK) {
        // link new page
        status = bufMgr->allocPage(filePtr, newPageNo, newPage); // error is here ? why
        
        if(status != OK) {
            return status;
        }
        
        newPage->init(newPageNo);

        headerPage->lastPage = newPageNo;
        headerPage->pageCnt++;
        hdrDirtyFlag =  true;

        status = curPage->setNextPage(newPageNo);
        if (status != OK){
            return status;
        }

        // modify header page
        status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
        
        if(status != OK) {
            return status;
        }

        // make curPage newPage
        curPage = newPage;
        curPageNo = newPageNo;

        status = bufMgr->readPage(filePtr, curPageNo, curPage);
        if (status != OK){
            return status;
        }

        // insert record
        status = curPage->insertRecord(rec, outRid);

        if(status != OK) {
            printf("No space\n");
            return status;
        }
        status = curPage->setNextPage(-1);
        // unpin the page no longer used.
        status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
        if(status != OK) {
            return status;
        }
    }

    // do bookkeeping
    headerPage->recCnt++;
    hdrDirtyFlag = true;
    curDirtyFlag = true;
    curRec = outRid;
  
    return OK;
}


