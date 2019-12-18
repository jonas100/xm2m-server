/*
 * clientsession-cmdline.cpp
 *
 * Created on: Dec 17, 2019
 * Author: jsomers
 *
 * See the header file for a (relatively) complete description.
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <iostream>
using namespace std;

#include "clientsession-cmdline.h"
#include "reportwriter.h"
#include "resultsrepo.h"

/*
 * Most of the work done in the base class is useful here too, so the first few methods
 * simply support 'lockout' - only one command-line console user at a time is permitted.
 */

CommandLineClientSession::CommandLineClientSession(
	const char * description
)
: ClientSession(description, false)	// not UDP based
{
	connected = false;
}

CommandLineClientSession::~CommandLineClientSession()
{
}

void CommandLineClientSession::ConnectionEstablished(int sock)
{
	socket = sock;
	connected = true;
}

void CommandLineClientSession::ConnectionTerminated()
{
	connected = false;
}

bool CommandLineClientSession::IsConnected()
{
	return connected;
}

/*
 * The main specialization in this subclass is the bull-simple command-line processor.
 * It's provided mainly as scaffolding today; many additions are planned for the future.
 */

static ReportWriter writer(cout);

extern bool stopServer;

int CommandLineClientSession::MessageReceived(int socket)
{
	struct sockaddr clientAddress;
	unsigned int size = sizeof(clientAddress);
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
		connected = false;
	}
	else // (n > 0)
	{
		switch (toupper(rxbuffer[0]))
		{
			case 'W':
				resultsRepo.WriteReport(writer);
				n = snprintf(rxbuffer, sizeof(rxbuffer), "Repository write is complete.\nxm2m]");
				break;

			case 'Q':
				stopServer = true;
				n = snprintf(rxbuffer, sizeof(rxbuffer), "Terminating server operations.\nxm2m]");
				break;

			case '?':
			case 'H':
			default:
				n = snprintf(rxbuffer, sizeof(rxbuffer), "Commands:\n W - write all test records\n Q - quit xm2m-server\nxm2m]");
				break;
		}
		n = SendMessage(socket, &clientAddress, size, rxbuffer, n);
	}
	return n;
}

// end of clientsession-cmdline.cpp
