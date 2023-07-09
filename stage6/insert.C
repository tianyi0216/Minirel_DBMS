/** @author 
 * Student - id:
 * 
 * The file contains actual implementation of insert operator for minirel db of stage 6 of project.
*/

#include "catalog.h"
#include "query.h"
#include <stdlib.h>
#include <unistd.h>

/*
 * Inserts a record into the specified relation.
 *
 * Implemented by Dominic Robson
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Insert(const string & relation, const int attrCnt, const attrInfo attrList[]) {
	// variables
	AttrDesc *attrs; // relation attribute descriptors
	int relAttrs; // num attributes
	Status status; // status
	int size = 0; // size of record

	// get relation
	InsertFileScan rel(relation, status);

	// return if not OK
	if(status != OK) {
		return RELNOTFOUND;
	}

	// get relation information and sets relAttrs
	status = attrCat->getRelInfo(relation, (int&)relAttrs, attrs);

	// return if status not OK
	if(status != OK)
		return status;

	// return if the number of attributes passed is not the 
	// same amount of attributes in the relation
	if(attrCnt != relAttrs) {
		return BADCATPARM;
	}
	
	// find total size via AttrDesc
	for(int i = 0; i < relAttrs; i++) {
		size += attrs[i].attrLen;
	}

	// create data buff
	char data[size];
	
	// go through all the attributes and ensure they are not NULL
	for(int i = 0; i < relAttrs; i++) {
		attrInfo attr = attrList[i];

		if (attr.attrValue == NULL) {
			return ATTRNOTFOUND;
		}
	}

	// go through attributes and populate record
	for(int i = 0; i < relAttrs; i++) {
		// index to add into
		int index = i;
		// description of that attribute
		AttrDesc desc = attrs[i];

		// find correct index
		for(int j = 0; j < relAttrs; j++) {
			attrInfo attr = attrList[j];

			if(strcmp(attr.attrName, desc.attrName) == 0) {
				index = j;
				break;
			}
		}
		
		// get info of data about to insert
		attrInfo insert = attrList[index];

		// vars for casting
		char *val;
		int tempI;
		float tempF;
		
		// cast to right type by casting original val 
		// and making a string of the reference
		switch (insert.attrType) {
			case STRING:
				val = (char *)insert.attrValue;
				break;
			case INTEGER:
				int intVal;
				tempI = atoi((char*)insert.attrValue);
				val = (char*)&tempI;
				break;
			case FLOAT:
				tempF = atof((char*)insert.attrValue);
				val = (char*)&tempF;
				break;
		}
		
		// copy into data buff
		memcpy(data + desc.attrOffset, val, desc.attrLen);
	}

	// create record
	RID rid;
	Record record;

	// set size
	record.length = size;
	// set data
	record.data = (void *)data;

	// insert into relation
	status = rel.insertRecord(record, rid);

	// return if status not OK
	if(status != OK)
		return status;

	return OK;
}