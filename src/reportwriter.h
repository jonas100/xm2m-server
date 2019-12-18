/*
 * reportwriter.h
 *
 * Created on: Dec 17, 2019
 * Author: jsomers
 *
 * ReportWriter is a base class used to traverse a ResultsRepository and format
 * its contents, either to disk or perhaps directly to a remote console operator.
 * This base class writes records into an HTML report. Derived classes could be built
 * to write in CSV, XML, JSON, plaintext, or any of a variety of other useful formats.
 */

#ifndef REPORTWRITER_H_
#define REPORTWRITER_H_

#include <iostream>
using namespace std;

#include "resultsrepo.h"	// for TestRecord definition

class ReportWriter
{
public:
	ReportWriter(ostream &of);
	virtual ~ReportWriter();

	virtual bool Begin();
	virtual bool WriteRecord(TestRecord &tr);
	virtual bool End();

protected:
	ostream *outputFile;

private:
};

#endif /* REPORTWRITER_H_ */
