/*
 * clientsession.cpp
 *
 * Created on: Dec 16, 2019
 * Author: jsomers
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <iostream>
using namespace std;

#include "resultsrepo.h"
#include "clientsession.h"

/*
 * We maintain a global transaction ID which increases monotonically
 * (until MAXINT is reached, anyway) to uniquely identify each transaction
 * performed during a single run of the server.
 */

static int transactionNumber = 1;

ClientSession::ClientSession(
	const char * desc,
	bool udp
){
	description = strdup(desc);
	useUDP = udp;
}

ClientSession::~ClientSession()
{
	if (description)
	{
		free(description);
		description = NULL;
	}
}

int ClientSession::MessageReceived(int socket)
{
	struct sockaddr clientAddress;
	struct sockaddr_in *inaddr = (sockaddr_in *)&clientAddress;
	unsigned int size = sizeof(clientAddress);
	getpeername(socket, &clientAddress, &size);

	memset(rxbuffer, 0, sizeof(rxbuffer));
	int n = recvfrom(socket, (void *)rxbuffer, sizeof(rxbuffer), 0, &clientAddress, &size);
	if (n < 0)
	{
		if (errno != EWOULDBLOCK)
		{
			cerr << "Socket receive failure" << endl;
		}
	}
	else if (n == 0)
	{
		cout << "Session ended normally (how polite)." << endl;
	}
	else // (n > 0)
	{
		cout << description << ": Message arrived"
			 << " from " << inet_ntoa(inaddr->sin_addr)
		     << ": " << rxbuffer
			 << endl;

		// record info about the transaction

		TestRecord testRecord;
        testRecord.transactionNumber = transactionNumber++;
        gettimeofday(&testRecord.startTime, NULL);
        testRecord.ipAddress = inaddr->sin_addr;
        testRecord.port = inaddr->sin_port;

        memset(testRecord.dataReceived, 0, sizeof(testRecord.dataReceived));
        memset(testRecord.dataSent, 0, sizeof(testRecord.dataSent));

        memcpy(testRecord.dataReceived, rxbuffer, n);
        memcpy(testRecord.dataSent, rxbuffer, n);

		// process and send the packet

		for (int i = 0; i < n; i++)
		{
			testRecord.dataSent[i] = toupper(testRecord.dataSent[i]);
		}
		n = SendMessage(socket, &clientAddress, size, testRecord.dataSent, n);

		// record our information about the transaction

		resultsRepo.StoreRecord(testRecord);
	}
	return n;
}

int ClientSession::SendMessage(
	int socket,
	struct sockaddr *clientAddress,
	int addrLength,
	char * buffer,
	int bufferLength
){
	int rc = 0;

	if (addrLength == 0)
	{
		clientAddress = NULL;
	}
	rc = sendto(socket, (void *)buffer, bufferLength, 0, clientAddress, addrLength);
	if (rc > 0)
	{
		cout << "Sent " << rc << " bytes: " << buffer << endl;
	}
	else
	{
		cerr << "Unable to send reply: " << rc << " (" << errno << ")" << endl;
	}
	return rc;
}

// end of clientsession.cpp
