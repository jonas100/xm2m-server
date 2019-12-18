/*
 * reportwriter.cpp
 *
 * Created on: Dec 17, 2019
 * Author: jsomers
 *
 * See the header file for a (relatively) complete description.
 */


#include "resultsrepo.h"
#include "reportwriter.h"
#include <iomanip>		// for setw and setfill

extern int transactionPort;		// defined in xm2m-server.cpp - settable by command line args

ReportWriter::ReportWriter(ostream &of)
{
	outputFile = &of;
}

ReportWriter::~ReportWriter()
{

}

bool ReportWriter::Begin()
{
	time_t timer;
	struct tm* tm_info;
	char timeBuffer[25];

	/*
	 * Record the time at which the report was run.
	 */
	time(&timer);
	tm_info = localtime(&timer);
	strftime(timeBuffer, 25, "%Y-%m-%d %H:%M:%S", tm_info);

	// Now write relevant stats about this test run to the HTML file

	*outputFile << "<html>\n <head>\n  <title>xm2m-server session test results</title>\n </head>\n <body>\n";
	*outputFile <<"  <h1>xm2m-server session test results</h1>\n";
	*outputFile <<"  <h2>Report run on " << timeBuffer << "</h2>\n" ;
	*outputFile <<"  <p>Server build date: " <<  __DATE__ << "</p>\n";
	*outputFile <<"  <p>Server port: " << transactionPort << "</p>\n";
	*outputFile <<"  <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" halign=\"left\" valign=\"middle\">\n";
	*outputFile <<"  <tr><td>Transaction #</td><td>Transaction time</td><td>From IP address</td><td>From port</td><td>Inbound data</td><td>Reply data</td></tr>\n";

	return true;
}

bool ReportWriter::WriteRecord(TestRecord &tr)
{
	struct tm *tm_info;
	char startTime[25];

	tm_info = localtime(&(tr.startTime.tv_sec));
	strftime(startTime, 25, "%Y-%m-%d %H:%M:%S", tm_info);

	*outputFile <<"<tr><td>" << tr.transactionNumber
		<< "</td> <td>" << startTime << '.'
		<< setfill('0') << setw(3) << (int)(tr.startTime.tv_usec / 1000)
		<< setfill(' ') << setw(0) << "</td> <td>" << inet_ntoa(tr.ipAddress)
		<< "</td> <td>" << ntohs(tr.port)
		<< "</td> <td>" << tr.dataReceived
		<< "</td> <td>" << tr.dataSent
		<< "</td></tr>\n";

	return true;
}

bool ReportWriter::End()
{
	*outputFile <<"  </table>\n </body>\n</html>";
	// TODO: if not console, close it explicitly - unwise to rely on destructors for this
	return true;
}

// end of reportwriter.cpp
