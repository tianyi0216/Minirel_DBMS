/** @author 
 * Student - id:
 * 
 * The file contains actual implementation of delete operator for minirel db of stage 6 of project.
*/

#include "catalog.h"
#include "query.h"

/*
 * Deletes records from a specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Delete(const string &relation,
					   const string &attrName,
					   const Operator op,
					   const Datatype type,
					   const char *attrValue)
{

	Status status;
	// Construct a heapFileScan object using the relation name.
	HeapFileScan *hfs = new HeapFileScan(relation, status);
	if (status != OK)
	{
		return status;
	}

	// To call startScan, we need more information such as length and offset.
	// Thus, we need to get these information by calling getInfo and get a AttrDesc object.
	AttrDesc attrDesc;
	attrCat->getInfo(relation, attrName, attrDesc);
	
	// declare the pointer and address to call select. For some reason, need to put it in for a separate pointer
	// so it works. Likely because memory issues.
	void *search_value;
	int search_val;
	float search_fval;

	// Next, we call startScan() with different parameters for different value types.
	// Before that, we first handle the case when attrName is NULL.
	if (attrName == "")
	{
		status = hfs->startScan(0, 0, STRING, NULL, EQ);
	}
	else
	{
		switch (type)
		{
		case STRING:
			status = hfs->startScan(attrDesc.attrOffset, attrDesc.attrLen, type, attrValue, op);
			break;

		case INTEGER:
			search_val = atoi(attrValue);
			search_value = &search_val;
			status = hfs->startScan(attrDesc.attrOffset, attrDesc.attrLen, type, (char*)search_value, op);
			break;

		case FLOAT:
			search_fval = atof(attrValue);
			search_value = &search_fval;
			status = hfs->startScan(attrDesc.attrOffset, attrDesc.attrLen, type, (char*)search_value, op);
			break;
		}
	}

	// If status of startScan() is not OK, delete the heapFileScan object.
	if (status != OK)
	{
		delete hfs;
		return status;
	}

	// Start the scan successcully, then we can try to find and delete the target record.
	RID rid;
	while (status == OK)
	{
		status = hfs->scanNext(rid);
		if (status != OK)
			break;
		
		status = hfs->deleteRecord();
		if (status != OK)
			return status;
	}

	// Finally, we end the scan and cleanup the heapFileScan object.
	hfs->endScan();
	delete hfs;

	return OK;
}