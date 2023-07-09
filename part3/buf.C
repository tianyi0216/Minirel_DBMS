/** @author 
 * Student 
 * 
 * The file contains actual implementation of buffer manager class of stage 3 of project.
 * The buffer manager is responsible for deciding read/flush file into buffer pools.
*/

#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include "page.h"
#include "buf.h"
#include "error.h"


#define ASSERT(c)  { if (!(c)) { \
		       cerr << "At line " << __LINE__ << ":" << endl << "  "; \
                       cerr << "This condition should hold: " #c << endl; \
                       exit(1); \
		     } \
                   }

//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

// new error obj to print error messages
Error *e = new Error();

BufMgr::BufMgr(const int bufs)
{
    numBufs = bufs;

    bufTable = new BufDesc[bufs];
    memset(bufTable, 0, bufs * sizeof(BufDesc));
    for (int i = 0; i < bufs; i++) 
    {
        bufTable[i].frameNo = i;
        bufTable[i].valid = false;
    }

    bufPool = new Page[bufs];
    memset(bufPool, 0, bufs * sizeof(Page));

    int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
    hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

    clockHand = bufs - 1;
}


BufMgr::~BufMgr() {

    // flush out all unwritten pages
    for (int i = 0; i < numBufs; i++) 
    {
        BufDesc* tmpbuf = &bufTable[i];
        if (tmpbuf->valid == true && tmpbuf->dirty == true) {

#ifdef DEBUGBUF
            cout << "flushing page " << tmpbuf->pageNo
                 << " from frame " << i << endl;
#endif

            tmpbuf->file->writePage(tmpbuf->pageNo, &(bufPool[i]));
        }
    }

    delete [] bufTable;
    delete [] bufPool;
}

/**
 * Implemented by Tianyi Xu
 * Allocate a frame in the buffer pool for inserting a page using clock algorithm.
 * @param frame - a reference to the frame number, would bne the frame no for allocated frame.
 * @return status of allocating a frame. OK if success and en Error if not.
*/
const Status BufMgr::allocBuf(int & frame) 
{
    // keep track of number of frame visited. Maximum is 2* number of frames.
    int i = 0;

    // executing clock algorithm, we know we couldn't find one of maximum 2 iterations of all framnes.
    while(i < 2*numBufs){

        // get all the relevant information of current frame
        bool valid_bit = bufTable[clockHand].valid;
        bool ref_bit = bufTable[clockHand].refbit;
        bool dirt_bit = bufTable[clockHand].dirty;
        int pin_count = bufTable[clockHand].pinCnt;

        // if not valid - can use directly.
        if(!valid_bit){
            frame = clockHand;
            return OK;
        } else if(ref_bit){
            // if referenced, dereference the frame and move on
            bufTable[clockHand].refbit = false;
            bufStats.accesses++;
            advanceClock();
            i++;
        } else if(pin_count > 0){
            // if pin count not 0 - cannot use.
            advanceClock();
            i++;
        } else if(dirt_bit){
            // page's pin count is 0 - can be used, but it is dirty so we need to write current file on frame to disk and then allocate the frame
            Status status = bufTable[clockHand].file->writePage(bufTable[clockHand].pageNo, &(bufPool[clockHand]));
            bufStats.diskwrites++; 
            if(status != OK){
                e->print(status);
                return status;
            }

            // remove corresponding entry from hashtable.
            hashTable->remove(bufTable[clockHand].file, bufTable[clockHand].pageNo);
            
            // allocate and return OK (clear frame)
            bufTable[clockHand].Clear();
            frame = clockHand;
            return OK;
        } else{
            // not dirty - can be allocated directly.
            hashTable->remove(bufTable[clockHand].file, bufTable[clockHand].pageNo);
            bufTable[clockHand].Clear();
            frame = clockHand;
            return OK;
        }
    }

    // all pages have been pinned - couldn't find a buffer to allocate
    return BUFFEREXCEEDED;
}

/**
 * Implemented by Layton Liu
 * This function reads a page from a file into a buffer frame in the buffer pool and returns a pointer to 
 * the buffer pool frame containing the page if successful.
 * @param file - the file to read the page from
 * @param PageNo - the page number of the page to read
 * @param page - a pointer to a Page object that will be set to the buffer pool frame containing the page
 * @return status of reading a page (OK if successful, an error code otherwise)
*/	
const Status BufMgr::readPage(File* file, const int PageNo, Page*& page)
{
    // Look up for the page
    int initialFrame = -1;
    int& frame = initialFrame;
    Status status = hashTable->lookup(file, PageNo, frame);

    if(status != OK){
        status = allocBuf(frame);
        if(status != OK){
            e->print(status);
            return status;
        }

        // Read the page from the disk into the buffer pool frame
        status = file->readPage(PageNo, &bufPool[frame]);
        bufStats.diskreads++;
        if(status != OK){
            e->print(status);
            return status;
        }

        // Set up the page
        BufDesc* target = nullptr;

        for(int i = 0; i < numBufs; i++){
            if(bufTable[i].frameNo == frame){
                target = &bufTable[i];
                break;
            }
        }
        target->Set(file, PageNo);

        page = &bufPool[frame];
        
        // Insert the page into the hashtable
        status = hashTable->insert(file, PageNo, frame);
        if(status != OK){
            e->print(status);
            return status;
        }

        
        return OK;
    } else{
        // Handle the case that the page exists
        BufDesc* target = nullptr;

        for(int i = 0; i < numBufs; i++){
            if(bufTable[i].frameNo == frame){
                target = &bufTable[i];
                break;
            }
        }

        // Increment the pin count for the page
        target->pinCnt += 1;
        // Set reference bit to true to indicate that the page is recently access
        target->refbit = true;

        page = &bufPool[frame];
        return OK;
    }
}

/**
 * Implemented by Tianyi
 * Unpin a page from the buffer pool.
 * @param file - the file to unpin page from
 * @param pageNo - the page number of page to unpin from
 * @param dirty - whether the page to unpin is dirty or not.
 * @return OK if no errors, else specific error
*/
const Status BufMgr::unPinPage(File* file, const int PageNo, 
			       const bool dirty) 
{
    // check if frame is in hashTable
    int initialFrame = -1;
    int& frame = initialFrame;
    Status status = hashTable->lookup(file, PageNo, frame);
    
    // return OK if in, otherwise error
    if(status != OK){
        e->print(status);
        return status;
    }

    // check if pinCOunt is 0 already, if 0 then return Error.
    if(bufTable[frame].pinCnt == 0){
        return PAGENOTPINNED;
    }

    // decrement pincount by 1
    bufTable[frame].pinCnt --;

    // set to dirty if specified in the parameter.
    if(dirty == true){
        bufTable[frame].dirty = true;
    }
    
    return OK;
}

/**
 * Implemented by Dominic Robson
 * 
 * Allocates an empty page in a file to the  
 *      buffer pool and sets it in the hashtable.
 * 
 * @param file - the file to allocate a page for
 * @param pageNo - pageNo of the page in the file
 * @param page - the actual page to allocate
 * 
 * @return OK if no errors, else specific error
*/
const Status BufMgr::allocPage(File* file, int& pageNo, Page*& page) 
{
    // status object to get status returns from function calls
    Status status;

    // allocate empty page in file and get status
    status = file->allocatePage(pageNo);

    // check status to ensure it is OK
    if(status != OK) {
        e->print(status);
        return status;
    }

    // initialize the frame number to -1;
    int initialFrame = -1;
    int& frame = initialFrame;

    // request frame from the buffer pool
    status = allocBuf(frame);

    // check status to ensure it is OK
    if(status != OK) {
        e->print(status);
        return status;
    }

    // set frame in bufTable with file and pageNo
    bufTable[frame].Set(file, pageNo);
    
    // set page to that from
    page = &bufPool[frame];

    // insert file, pageNo, and frame into hashtable
    status = hashTable->insert(file, pageNo, frame);

    // check status to ensure it is OK
    if(status != OK) {
        e->print(status);
        return status;
    }

    return OK;
}

const Status BufMgr::disposePage(File* file, const int pageNo) 
{
    // see if it is in the buffer pool
    Status status = OK;
    int frameNo = 0;
    status = hashTable->lookup(file, pageNo, frameNo);
    if (status == OK)
    {
        // clear the page
        bufTable[frameNo].Clear();
    }
    status = hashTable->remove(file, pageNo);

    // deallocate it in the file
    return file->disposePage(pageNo);
}

const Status BufMgr::flushFile(const File* file) 
{
  Status status;

  for (int i = 0; i < numBufs; i++) {
    BufDesc* tmpbuf = &(bufTable[i]);
    if (tmpbuf->valid == true && tmpbuf->file == file) {

      if (tmpbuf->pinCnt > 0)
	  return PAGEPINNED;

      if (tmpbuf->dirty == true) {
#ifdef DEBUGBUF
	cout << "flushing page " << tmpbuf->pageNo
             << " from frame " << i << endl;
#endif
	if ((status = tmpbuf->file->writePage(tmpbuf->pageNo,
					      &(bufPool[i]))) != OK)
	  return status;

	tmpbuf->dirty = false;
      }

      hashTable->remove(file,tmpbuf->pageNo);

      tmpbuf->file = NULL;
      tmpbuf->pageNo = -1;
      tmpbuf->valid = false;
    }

    else if (tmpbuf->valid == false && tmpbuf->file == file)
      return BADBUFFER;
  }
  
  return OK;
}


void BufMgr::printSelf(void) 
{
    BufDesc* tmpbuf;
  
    cout << endl << "Print buffer...\n";
    for (int i=0; i<numBufs; i++) {
        tmpbuf = &(bufTable[i]);
        cout << i << "\t" << (char*)(&bufPool[i]) 
             << "\tpinCnt: " << tmpbuf->pinCnt;
    
        if (tmpbuf->valid == true)
            cout << "\tvalid\n";
        cout << endl;
    };
}


