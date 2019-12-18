/*
 * resultsrepo.cpp
 *
 * Created on: Dec 17, 2019
 * Author: jsomers
 *
 * See the header file for a (relatively) complete description.
 */

#include <iostream>
using namespace std;

#include <stdlib.h>
#include <string.h>				// for memset() and memcpy()
#include "resultsrepo.h"
#include "reportwriter.h"

ResultsRepository::ResultsRepository()
{
	head = 0;
	totalTestRecords = 0;
	testRecords = NULL;
}

ResultsRepository::~ResultsRepository()
{
	if (testRecords)
	{
		free(testRecords);
	}
}

void ResultsRepository::Init(int howManyRecordsToKeep)
{
	if (testRecords)
	{
		cerr << "ResultsRepository: already initialized" << endl;
	}
	else
	{
		totalTestRecords = howManyRecordsToKeep;
		testRecords = (TestRecord *)malloc(sizeof(TestRecord) * totalTestRecords);
		memset(testRecords, 0, sizeof(TestRecord) * totalTestRecords);
	}
}

void ResultsRepository::StoreRecord(TestRecord& record)
{
	if (testRecords)
	{
		memcpy(&(testRecords[head]), &record, sizeof(TestRecord));
		head++;
		if (head >= totalTestRecords)
		{
			head = 0;
		}
	}
}

void ResultsRepository::WriteReport(ReportWriter &writer)
{
	if (testRecords)
	{
		writer.Begin();

		/*
		 * If there is already a record at 'head', then we've wrapped around at least one time, and the record at head
		 * is currently the oldest record on file, so let's write all of the records from 'head' to the end of the array
		 * first.
		 */

		unsigned int i;
		if (testRecords[head].transactionNumber > 0)
		{
			for (i = head; i < totalTestRecords; i++)
			{
				writer.WriteRecord(testRecords[i]);
			}
		}

		/*
		 * Now we'll add the newest records (from 0 to head)
		 */

		for (i = 0; i < head; i++)
		{
			writer.WriteRecord(testRecords[i]);
		}
		writer.End();
	}
}

// the sole global instance, in this build anyway

ResultsRepository resultsRepo;

// end of ResultsRepository.cpp
