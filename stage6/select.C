/** @author 
 * Student - id:
 * 
 * The file contains actual implementation of select operator for minirel db of stage 6 of project.
*/

#include "catalog.h"
#include "query.h"
#include <unistd.h>

// forward declaration
const Status ScanSelect(const string & result, 
			const int projCnt, 
			const AttrDesc projNames[],
			const AttrDesc *attrDesc, 
			const Operator op, 
			const char *filter,
			const int reclen);

/*
 * Selects records from the specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Select(const string & result, 
		       const int projCnt, 
		       const attrInfo projNames[],
		       const attrInfo *attr, 
		       const Operator op, 
		       const char *attrValue)
{
   // Qu_Select sets up things and then calls ScanSelect to do the actual work
    cout << "Doing QU_Select " << endl;

	// define variables we need to use
	Status status;
	AttrDesc projDescNames[projCnt];
	AttrDesc *attrDesc = NULL;

	// transform attrInfo into attrDesc
	for(int i = 0; i < projCnt; i++){
		status = attrCat->getInfo(projNames[i].relName, projNames[i].attrName, projDescNames[i]);
		if(status != OK){
			return status;
		}
	}

	// update attrDesc and get rectlen
	if(attr != NULL){
		//initialize attrDesc so we can use
		attrDesc = new AttrDesc();
		status = attrCat->getInfo(attr->relName, attr->attrName, *attrDesc);
		if(status != OK){
			return status;
		}
	}
	
	int reclen = 0;
	// get reclen
	for(int i = 0; i < projCnt; i++){
		reclen += projDescNames[i].attrLen;
	}

	// call scanSelect
	// if attr is null, then we do unconditioned scan
	if(attr == NULL){
		status = ScanSelect(result, projCnt, projDescNames, attrDesc, EQ, NULL, reclen);
		if(status != OK){
			if (attrDesc != NULL) {
    			delete attrDesc;
			}
			return status;
		}
	} else {
		status = ScanSelect(result, projCnt, projDescNames, attrDesc, op, attrValue, reclen);
		if(status != OK){
			if (attrDesc != NULL) {
   	 			delete attrDesc;
			}
			return status;
		}
	}

	// we delete attrDesc object for potential memory issue
	if (attrDesc != NULL) {
    	delete attrDesc;
	}
	return status;
}


const Status ScanSelect(const string & result, 
#include "stdio.h"
#include "stdlib.h"
			const int projCnt, 
			const AttrDesc projNames[],
			const AttrDesc *attrDesc, 
			const Operator op, 
			const char *filter,
			const int reclen)
{
    cout << "Doing HeapFileScan Selection using ScanSelect()" << endl;
	Status status;

	//hfs scan
	HeapFileScan *hfs = new HeapFileScan(projNames[0].relName, status);
	if(status != OK){
		return status;
	}

	// the insert file to insert record into result.
	InsertFileScan rel(result, status);
	
	if(status != OK){
		delete hfs;
		return status;
	}

	// declare the pointer and address to call select. For some reason, need to put it in for a separate pointer
	// so it works. Likely because memory issues.
	void *search_value;
	int search_val;
	float search_fval;

	// start-scan, 2 cases for no attribute or have attribute
	if(attrDesc == NULL){
		// This is the case when attr is null, so unconditioned scan
		status = hfs->startScan(0, 0, STRING, NULL, EQ);
		if(status != OK){
			delete hfs;
			return status;
		}
	} else {
		// we check the attribute type and then do the corresponding start scan and type casting
		switch(attrDesc->attrType){
			// string, no need conversion
			case 0:{
				search_value = (char*)filter;
				status = hfs->startScan(attrDesc->attrOffset, attrDesc->attrLen, STRING, (char*)search_value, op);
				if(status != OK){
					delete hfs;
					return status;
				}
				break;
			}
			// int and float, need conversion
			// for both cases, first covert filter to corresponding type, then store in the void pointer and use that void pointer as
			// the filter to startScan. So it does not overwritte or create memory issues.
			case 1:{
				search_val = atoi(filter);
				search_value = &search_val;
				status = hfs->startScan(attrDesc->attrOffset, attrDesc->attrLen, INTEGER, (char *)&search_val, op);
				if(status != OK){
					delete hfs;
					return status;
				}
				break;
			}
			case 2:{
				search_fval = atof(filter);
				search_value = &search_fval;
				status = hfs->startScan(attrDesc->attrOffset, attrDesc->attrLen, FLOAT, (char *)&search_fval, op);
				if(status != OK){
					delete hfs;
					return status;
				}
				break;
			}
		}
	}
	
	// we insert record here, first declare all the variables.
	char data[reclen]; // store found data (like a record's data)
	int offset; // offset when we insert into the data record
	RID rid;  //rid for current record
	Record rec; // current record
	Status nextStatus = OK; // if not OK, we stop

	// scan next record
	while(nextStatus == OK){
		nextStatus = hfs->scanNext(rid);
		if(nextStatus != OK){
			break;
		}

		// get the record
		status = hfs->getRecord(rec);
		if(status != OK){
			// not OK, end scan and delete scan file
			hfs->endScan();
			delete hfs;
			return status;
		}

		// go through found record to copy each attribute to the data, we use offset to track where to copy.
		offset = 0;
		for (int i = 0; i < projCnt; i++){
			memcpy(data + offset, (char*)(rec.data + projNames[i].attrOffset), projNames[i].attrLen);
			offset += projNames[i].attrLen;
		}

		// here is the data we want to use, use a different variable to avoid memory issues.
		RID insertRid;
		Record insertRecord;
		insertRecord.length = reclen;
		insertRecord.data = (void *)data;

		// insert into result using insertFileScan
		status = rel.insertRecord(insertRecord, insertRid);

		if(status != OK){
			hfs->endScan();
			delete hfs;
			return status;
		}
	}

	// if we done, endscan and delete the hfs file.
	hfs->endScan();
	delete hfs;

	return status;
}
