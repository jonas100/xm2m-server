# xm2m-server - an eXtensible Machine-2-Machine transaction simulator

Created on: Dec 14, 2019

Author: jsomers

## Overview

xm2m-server (and its forthcoming partner program, xm2m-client) simulate machine-to-machine (M2M)
communications. Together, the tools repeatedly establish a TCP session and perform a transaction in a single
round trip, typically in one single transmitted TCP request packet and one single received TCP reply.
The mission for the tools is to test underlying communications infrastructure, whether wireline or wireless.

xm2m-server provides functionality similar to an echo server on TCP or UDP port 7, although with far greater
flexibility and sophistication.

The motivations for the xm2m project include:
- Highlighting the differences that arise when communicating over high-speed links (such as direct Ethernet connections) 
vs low-speed or high-latency links (narrowband IoT LTE connections)
- Flexible testing, such as stress testing connections (flooding a channel with transactions) or profiling a
total network system (estimating up-time and availability over long periods of time)
- Illustrating some coding techniques for portability across platforms and effectively managing resources in embedded systems

Motivations behind building xm2m-server instead of just using a generic echo server include:
- Public Internet echo servers are becoming less common as they represent a security risk
- Abuse capture: if someone attempts to abuse the server, this can be detected
and recorded for future blacklisting
- Rate throttling: limit 'hogging' the server by any single entity
- Reconciliation: If an xm2m-client reports that a transaction attempt failed, then client information can be compared against
the data collected by xm2m-server. If the request made it to xm2m-server and was recorded there, the failure was probably in the return path. If not,
the error could have happened during the request phase, or could simply have been due to server unavailability (congestion,
server down, etc).

## Design criteria

1. Runs as a userspace daemon - operable entirely from a command line
2. Single running thread (simplifies code by eliminating need for semaphores, mutexes, and other contention mechanisms)
2. Readily ports to any Linux platform, but could be any processor family (so be cognizant of endianness and network addressing order)
3. Accepts concurrent connections from multiple clients at same or different IP addresses
4. Accepts either TCP or UDP connections concurrently
5. Accepts connections on specifiable ports (does not require port 7)
6. Allows monitoring of recent traffic and tracking of basic connection statistics
7. Not localizable
8. Ideally used with xm2m-client, but can be tested with any telnet-like client (even on same box)

Both client and server should share common source files relating to the payloads they exchange, as well as any other reusable code.
Note that xm2m-client's portability requirements are much broader, so shared code is more restricted in its portability considerations
(e.g. avoid using STL in common code).

Much of xm2m-server's resource utilization (# of concurrent TCP sessions, # of transactions to record in history FIFO)
is configurable via its command line.

## Futures

- Both client and server should be entirely configurable by command-line args, but by writing new subclasses, other
configuration means should also be possible (for example, a config stored in a file, or in internal flash memory).
- The server offers 'snap-on' capability thru its command console - so (for example) a separate Qt program could be launched at any time
and connect to xm2m-server (locally or remotely) to provide a friendlier GUI for monitoring activity.
- The server should have a primitive authentication capability to validate that transaction requests actually came from 
an xm2m-client. If any other client connects, their communication attempts could be logged to an exception bucket. New
validation and authentication subclasses could be written in the future to strengthen this feature. 
- Rudimentary authentication to permit access to the command line should be added.

## Building

xm2m-server is an Eclipse CDT project. It should be sufficient to clone the source tree, switch to your cloned xm2m-server/Debug tree,
and run make to produce an executable.

## Usage

xm2m-server runs well with no arguments using sensible defaults. xm2m-server --help will list all available options.

Once xm2m-server is running, open a separate terminal window (on the same or a different machine) and telnet or netcat to the xm2m-server on either
TCP or UDP port 9900 (by default). Anything you type will be echoed back to you, converted to uppercase. As described above, this behavior is highly
configurable via future subclassing. Multiple transaction sessions are permitted.

You can also telnet to TCP port 1900 (again, by default) to access the management console. Only one connection at a time is permitted to this port; 
attemps to connect concurrently will be silently dropped. (This isn't really done to be useful; it might actually be desirable to alow multiple concurrent
consoles. It's mainly done just to show how to limit behavior in this way.)

## Compatibility

xm2m-server has been tested with the following operating systems:
* Mac OS X 10.13.6 (High Sierra) Darwin Kernel Version 17.7.0 x86_64
* Raspberry Pi, Raspbian 3.6.11+ arm6L
* Ubuntu 16.04 Linux 4.4.0-170-generic x86_64