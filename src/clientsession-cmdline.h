/*
 * clientsession-cmdline.h
 *
 * Created on: Dec 17, 2019
 * Author: jsomers
 *
 * This is a specialized subclass of ClientSession dedicated to a shell-like command interpreter
 * for remotely viewing and managing the xm2m-server.
 *
 * Fundamentally, ClientSession is stateless, and for the time being CommandLineClientSession
 * is stateless too; however, it's easy to envision a day when multiple concurrent instances
 * of CommandLineClientSession exist, and state information would be needed for each.
 */

#ifndef CLIENTSESSION_CMDLINE_H_
#define CLIENTSESSION_CMDLINE_H_

#include "clientsession.h"

class CommandLineClientSession : public ClientSession
{
public:
	CommandLineClientSession(const char * description);
	~CommandLineClientSession();

	void ConnectionEstablished(int socket);
	void ConnectionTerminated();
	bool IsConnected();

	int Socket() { return socket; }

	int MessageReceived(int socket);

protected:
	bool connected;
	int socket;

private:
};

#endif /* CLIENTSESSION_CMDLINE_H_ */

// end of clientsession-cmdline.h
