//============================================================================
// Name        : xm2m-server.cpp
// Author      : Jonathan Somers
// Version     : 0.0
// Copyright   : Copyright (C) 2019 by Jonathan Somers
// Description : Startup code for xm2m-server transaction processing daemon
//============================================================================

#include <unistd.h>
#include <stdlib.h>
#include <string.h>				// for memset, etc
#include <errno.h>
#include <getopt.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <netinet/in.h>			// for sockaddr, etc

#include <iostream>
using namespace std;

#include "clientsession.h"			// various classes for tracking client session information
#include "clientsession-cmdline.h"	// specialized variant for our command line
#include "resultsrepo.h"

/*
 * The following globals are parameters that can be configured from the Linux command line at startup
 */

int transactionPort = 9900;
int consolePort = 1900;
int totalRepositoryRecords = 1000;
int totalConcurrentSessions = 13;	// NOTE: three are reserved for the listener sockets, leaving 10 concurrent TCP sessions

/*
 *  In the future, you might want to use Housekeeping() to do
 *  infrequent processing that doesn't depend on a strict schedule -
 *  cleaning up dead sessions or other resources.  Currently it's called
 *  once each time the poll() statement times out (after a minute of
 *  inactivity).
*/
void Housekeeping()
{
}

/*
 * Usage() and ParseCommandLine() are pretty straightfoward stuff...
 */

void Usage()
{
	cout << "\nusage: xm2m-server [--help][--port transactPort][--portCon consolePort]\n"
		<< "\t--port transactPort - which TCP and UDP port to listen for transactions (default:9900)\n"
		<< "\t--portCon consolePort - which TCP port to listen for console commands (default:1900)\n"
		<< "\t--repoSize size - how many repository records to keep in FIFO (default:1000)\n"
		<< "\t--sessions n - how many concurrent TCP transaction sessions to allow (default:10)\n"
		<< "\t--help - this usage information" << endl;
}

void ParseCommandLine(int argc, char *argv[])
{
	static struct option longOptions[] = {
		{ "help",		no_argument,		0,	0 },
		{ "port",		required_argument,	0,	1 },	// port to listen and 'echo' on
		{ "portCon",	required_argument,	0,	2 },	// port to listen for command console
		{ "repoSize",	required_argument,	0,	3 },		// how many records of transaction info are kept in repository
		{ "sessions",	required_argument,	0,	4 }		// how many records of transaction info are kept in repository
	};
	int optionIndex = 0;
	while (1)
	{
		int option = getopt_long(argc, argv, "", longOptions, &optionIndex);
		if (option == -1)
		{
			break;
		}
		if (option >= argc)
		{
			cerr << "Argument error" << endl;
			Usage();
		}
		switch (option)
		{
			case 0:
				Usage();
				break;

			case 1:			// which port to use for transactions
				transactionPort = atoi(optarg);
				if (
					(transactionPort < 0) ||
					(transactionPort > 65535)
				){
					cerr << "Port numbers should be from 0 to 65535" << endl;
					Usage();
					exit(-1);
				}
				cout << "Using port " << transactionPort << " for transactions" << endl;
				break;

			case 2:			// which port to use for mgmt console
				consolePort = atoi(optarg);
				if (
					(consolePort < 0) ||
					(consolePort > 65535)
				){
					cerr << "Port numbers should be from 0 to 65535" << endl;
					Usage();
					exit(-1);
				}
				cout << "Using port " << consolePort << " for command console" << endl;
				break;

			case 3:
				totalRepositoryRecords = atoi(optarg);
				if (totalRepositoryRecords <= 0)
				{
					cerr << "Must have at least one record in your repository" << endl;
					Usage();
					exit(-1);
				}
				else if (totalRepositoryRecords > 1000000)
				{
					cerr << "Warning: Are you sure you want over a million repository records?" << endl;
					Usage();
				}
				cout << "Creating " << totalRepositoryRecords << " transaction records in repository" << endl;
				break;

			case 4:
				totalConcurrentSessions = atoi(optarg);
				if (totalConcurrentSessions <= 0)
				{
					cerr << "Must allow at least one TCP transaction session" << endl;
					Usage();
					exit(-1);
				}
				else if (totalConcurrentSessions > 100)
				{
					cerr << "Warning: Are you sure you want over a hundred concurrent TCP transaction sessions?" << endl;
					Usage();
				}
				cout << "Allowing " << totalConcurrentSessions << " concurrent TCP transaction sessions" << endl;
				totalConcurrentSessions += 3;
				break;
		}
	}
}

bool InitSocket(
	int& sock,					// the socket to initialize
	int style,					// SOCK_DGRAM or SOCK_STREAM, e.g.
	struct sockaddr_in &addr	// the address(es) on which to accept connections
){
	sock = socket(AF_INET, style, 0);
	if (sock < 0)
	{
		cerr << "Unable to create socket" << endl;
		return false;
	}

	// let the address be re-used by future bind() operations (mainly after this process ends)

	int one = 1;
	int rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (rc < 0)
	{
		cerr << "Warning: Unable to set socket options for future re-use" << endl;
		// but this is non-fatal, so let's see if we can continue anyway
	}

	// bind to an address and port

	rc = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (rc < 0)
	{
		cerr << "Unable to bind socket to address and port" << endl;
		close(sock);
		return false;
	}

	// if this is a TCP socket, start listening for connections

	if (SOCK_STREAM == style)
	{
		rc = listen(sock, 1);	// 1 == don't allow connections to sit in pending state while awaiting next listen
		if (rc < 0)
		{
			cerr << "Unable to listen on socket" << endl;
			close(sock);
			return false;
		}
	}

	return true;
}

bool stopServer = false;		// can be set by command interpreter to stop the server

int main(int argc, char *argv[])
{
	cout << "xm2m-server: Build date " __DATE__  << endl;

	ParseCommandLine(argc, argv);

	/*
	 * Initialize the repository that stores info about transactions
	 */

	resultsRepo.Init(totalRepositoryRecords);

	/*
	 * Let's begin by setting up each of the receiving sockets we'll offer
	 */

	int cmdsock, tcptranssock, udptranssock;
	struct sockaddr_in cmdaddr, tcpaddr, udpaddr;

	memset(&cmdaddr, 0, sizeof(cmdaddr));
	cmdaddr.sin_family = AF_INET;
	cmdaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	cmdaddr.sin_port = htons(consolePort);

	if (!InitSocket(cmdsock, SOCK_STREAM, cmdaddr))
	{
		cerr << "Could not create command-line interface socket." << endl;
		exit(-1);
	}

	memset(&tcpaddr, 0, sizeof(cmdaddr));
	tcpaddr.sin_family = AF_INET;
	tcpaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	tcpaddr.sin_port = htons(transactionPort);

	if (!InitSocket(tcptranssock, SOCK_STREAM, tcpaddr))
	{
		cerr << "Could not create TCP transaction socket." << endl;
		exit(-1);
	}

	memset(&udpaddr, 0, sizeof(cmdaddr));
	udpaddr.sin_family = AF_INET;
	udpaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	udpaddr.sin_port = htons(transactionPort);

	if (!InitSocket(udptranssock, SOCK_DGRAM, udpaddr))
	{
		cerr << "Could not create UDP transaction socket." << endl;
		exit(-1);
	}

	/*
	 * Next, let's multiplex them onto one poll
	 */

	struct pollfd *pollfds = (struct pollfd *)malloc(sizeof(struct pollfd) * totalConcurrentSessions);
	if (pollfds == NULL)
	{
		cerr << "Insufficient memory";
		exit(-1);
	}
	memset(pollfds, 0, sizeof(struct pollfd));
	pollfds[0].fd = cmdsock;
	pollfds[0].events = POLLIN;

	pollfds[1].fd = udptranssock;
	pollfds[1].events = POLLIN;

	pollfds[2].fd = tcptranssock;
	pollfds[2].events = POLLIN;

	/*
	 * For now, let's create one of each class object so we can start folding code into it.
	 */

	CommandLineClientSession cmdlineClientSession("Command line client");
	ClientSession udpClientSession("UDP clients", true);
	ClientSession tcpClientSession("TCP client", false);

	/*
	 * The next loop is the main workhorse of the xm2m-server app.  It calls poll()
	 * to obtain events one by one, and maintains the list of active sockets and their requests.
	 * It calls the appropriate ClientSession object as needed.
	 */

	int fds = 3;
	int timeout = 60000;	// poll operation will take at most one minute
	do
	{
		int rc = poll(pollfds, fds, timeout);
		if (rc < 0)
		{
			cerr << "poll() failure " << rc << endl;
			exit(-1);
		}
		else if (rc == 0)
		{
			/*
			 *  Timer expired; that's normal, we'll just poll again; but if we've timed out,
			 *  then the system is relatively quiet, so now might be a good time to do some...
			 */
			Housekeeping();
		}
		else
		{
			int currentSize = fds;
			for (int i = 0; i < currentSize; i++)
			{
				if (pollfds[i].revents == 0)
				{
					continue;
				}
				if (pollfds[i].revents != POLLIN)
				{
					if (pollfds[i].revents & POLLERR)
					{
						cerr << "Polling error on fd #" << i << endl;
						stopServer = true;
					}
					if (pollfds[i].revents & POLLHUP)
					{
						cout << "Hangup event on fd #" << i << endl;
						// Okay, but so what? We should have already disconnected and closed anyway.
					}
				}
				if (pollfds[i].fd == cmdsock)
				{
					cout << "New command-line session!" << endl;
					int sock = accept(cmdsock, NULL, NULL);
					if (sock < 0)
					{
						if (errno != EWOULDBLOCK)
						{
							cerr << "Cannot accept incoming connection?" << endl;
							stopServer = true;
						}
					}
					else if (cmdlineClientSession.IsConnected())
					{
						cerr << "Command-line console session refused: Someone else is connected" << endl;
						close(sock);
					}
					else if (fds >= totalConcurrentSessions)
					{
						cerr << "Command-line console session refused: No more room for additional TCP sessions" << endl;
						close(sock);
					}
					else
					{
						cmdlineClientSession.ConnectionEstablished(sock);
						pollfds[fds].fd = sock;
						pollfds[fds].events = POLLIN;
						fds++;
					}
				}
				else if (pollfds[i].fd == tcptranssock)
				{
					cout << "New echo session!" << endl;
					int sock = accept(tcptranssock, NULL, NULL);
					if (sock < 0)
					{
						if (errno != EWOULDBLOCK)
						{
							cerr << "Cannot accept incoming connection?" << endl;
							stopServer = true;
						}
					}
					else if (fds >= totalConcurrentSessions)
					{
						cerr << "No more room for additional TCP sessions" << endl;
						close(sock);
					}
					else
					{
						pollfds[fds].fd = sock;
						pollfds[fds].events = POLLIN;
						fds++;
					}
				}
				else if (pollfds[i].fd == udptranssock)
				{
					udpClientSession.MessageReceived(udptranssock);
				}
				else	// an existing socket
				{
					int n;
					if (pollfds[i].fd == cmdlineClientSession.Socket())
					{
						n = cmdlineClientSession.MessageReceived(pollfds[i].fd);
					}
					else
					{
						n = tcpClientSession.MessageReceived(pollfds[i].fd);
					}
					if (n <= 0)	// either there was an error on the session, or it was routinely closed
					{
						cout << "Closing session..." << endl;
						close(pollfds[i].fd);

						// This would leave a hole in our pollfds array, so let's 'defrag' here

						for (; i < fds; i++)
						{
							if (i < (totalConcurrentSessions - 1))	// don't run off the end of the array
							{
								pollfds[i].fd = pollfds[i+1].fd;
							}
						}
						fds--;
					}
				}
			}
		}
	} while (!stopServer);

	/*
	 * Clean up and bail out
	 */

	cout << "All operations completed. Exiting." << endl;

	for (int i = 0; i < fds; i++)
	{
		shutdown(pollfds[i].fd, SHUT_RDWR);
		close(pollfds[i].fd);
	}

	return 0;
}

// end of xm2m-server.cpp
