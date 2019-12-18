/*
 * resultsrepo.h
 *
 * Created on: Dec 17, 2019
 * Author: jsomers
 *
 * See comments below for descriptions of structures and classes.
 */

#ifndef RESULTSREPO_H_
#define RESULTSREPO_H_

#include <time.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "clientsession.h"	// only for RX_BUFFER_SIZE

/*
 * A TransactionRecord could be turned into a class, but at this time, I don't have a
 * compelling reason to do so. Perhaps one reason to create a class would be to provide
 * friendly accessor methods for things like formatting the start time.
 * Not 'compelling' today though.
 */

typedef struct _TestRecord
{
	unsigned int transactionNumber;
	struct timeval startTime;
	struct in_addr ipAddress;
	unsigned short port;
	char dataReceived[RX_BUFFER_SIZE];
	char dataSent[RX_BUFFER_SIZE];
} TestRecord;

class ReportWriter;	// circular reference avoidance

/*
 * The ResultsRepository class is a base class that represents a relatively nonvolatile
 * repository for TestRecords. This base class just keeps a size-configurable FIFO buffer
 * of records in memory (agreed, that's volatile). In this version, oldest records are silently
 * discarded without warning by design.
 *
 * Derived classes could be written to implement features like:
 * - a backing MySQL database - perhaps keeping the base class's ring FIFO for buffering or cacheing
 * - automatically writing reports once a day, or whenever the ring fills
 * - issuing some kind of warning or counter of how often the ring fills
 */

class ResultsRepository
{
	friend class ReportWriter;

public:
	ResultsRepository();
	virtual ~ResultsRepository();

	virtual void Init(int howManyRecordsToKeep);
	virtual void StoreRecord(TestRecord& record);
	virtual void WriteReport(ReportWriter& writer);

protected:
	TestRecord *testRecords;
	unsigned int totalTestRecords;	// set at allocation time, during Init()
	unsigned int head;				// head of the FIFO - newest record is here

private:
};

/*
 * Not a true singleton - just a convenient global instance.
 * Theoretically you could decide to create one instance per connected client,
 * for example (in fact I think no class modifications are needed at all to
 * make that change).
 */

extern ResultsRepository resultsRepo;

#endif /* RESULTSREPO_H_ */
