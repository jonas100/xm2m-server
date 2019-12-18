/*
 * clientsession.h
 *
 * Created on: Dec 16, 2019
 * Author: jsomers
 *
 * The ClientSession class provides an extremely simple echo-reply capability, with additional tracking
 * of transactions performed. No authentication is performed and no true query-reply is done in the base class.
 *
 * To provide *some* small glimmer that rudimentary processing is happening, this base class's replies
 * are uppercase conversions of the requests. Subclasses can do more and/or completely different
 * manipulations.
 *
 * The intent is to allow creation of more sophisticated subclasses of ClientSession with greater
 * specializations, such as:
 * - a rudimentary command-line processor (see clientsession-cmdline for a VERY rudimentary example)
 * - an ATM simulator with some type of mock cash-dispense authorizations
 * - validations to distinguish ordinary Telnet sessions from xm2m-client sessions
 * - rudimentary authentications, perhaps using SPA
 * Lots of other customizations are certainly possible.
 */

#ifndef CLIENTSESSION_H_
#define CLIENTSESSION_H_

#define RX_BUFFER_SIZE	250	// TODO: this would be a great candidate for a command-line parameter as well

class ClientSession
{
public:
	ClientSession(
		const char * description,
		bool useUDP
	);
	virtual ~ClientSession();

	virtual int MessageReceived(int socket);
	virtual int SendMessage(
		int socket,
		struct sockaddr *clientAddress,
		int addrLength,
		char * buffer,
		int bufferLength
	);

protected:
	char * description;	// as friendly and plaintext-y a description as the available intel will allow
	bool useUDP;

	char rxbuffer[RX_BUFFER_SIZE];

private:
};

#endif /* CLIENTSESSION_H_ */

// end of clientsession.h
