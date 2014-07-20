**dnsperf** - A DNS performance tool

## Introduction
Dnsperf is a single-process dns load testing and benchmarking utility. It was designed to measure the performance of your
DNS Server or Local DNS Server by send a configured number of queries.
Performance measures include elapsed time, its transation rate, its concurrrency and the percentage of successful queries.
These measures are reported at the end of each testing.

## Usage
Dnsperf supports the following command line options:
**-s**
    Sets the DNS server's IP address. The default IP is 127.0.0.1.

**-p**
    Sets the DNS server's port. The default Port is 53.

**-d**
    Specifies the input data file. Input data file contains <domain, query_type> pairs.

**-t**
    Specifies the timeout for query completion in millisecond. The default timeout is 3000ms.

**-Q**
    Specifies the max number of queries to be send. The default number is 1000.

**-c**
    Specifies the number of concurrent queries. The default number is 100. Dnsperf will randonly pick <domain, type> from data file as a query.

**-l**
    Specifies how long to run tests in seconds. The default number is infinite.

**-i**
    Specifies interval of queries in seconds. The default number is zero. This option is not supported currently.

**-P**
    Specifies the transport layer prototol to send DNS queries, udp or tcp. As we known, DNS queries can be send either by UDP or TCP, though UDP is the suggested protocol. The default is udp. `tcp` is not supported currently, and it will coming soon.

**-f**
    Specify address family of DNS transport, inet or inet6. The default is inet. inet6 is not supported currently.

**-v**
    Verbose: report the RCODE of each response on stdout.

**-h**
    Print the usage of dnsperf.


## Author
Cobblau, <keycobing@gmail.com>