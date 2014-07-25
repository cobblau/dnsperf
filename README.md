dnsperf
======
A DNS performance tool.

### Introduction
Dnsperf is a single-process dns load testing and benchmarking utility. It was designed to measure the performance of your
DNS Server or Local DNS Server by send a configured number of queries.
Performance measures include elapsed time, its transation rate, its concurrrency and the percentage of successful queries.
These measures are reported at the end of each testing.

### Usage
Dnsperf supports the following command line options:

**-s**
&nbsp;&nbsp;&nbsp;&nbsp;Specifies the DNS server's IP address. The default IP is `127.0.0.1`  
**-p**
&nbsp;&nbsp;&nbsp;&nbsp;Specifies the DNS server's port. The default Port is `53`  
**-d**
&nbsp;&nbsp;&nbsp;&nbsp;Specifies the input data file. Input data file contains <domain, query_type> pairs.  
**-t**
&nbsp;&nbsp;&nbsp;&nbsp;Specifies the timeout for query completion in millisecond. The default timeout is `3000ms`.  
**-Q**
&nbsp;&nbsp;&nbsp;&nbsp;Specifies the max number of queries to be send. The default number is `1000`.  
**-c**
&nbsp;&nbsp;&nbsp;&nbsp;Specifies the number of concurrent queries. The default number is `100`. Dnsperf will randonly pick <domain, type> from data file as a query.  
**-l**
&nbsp;&nbsp;&nbsp;&nbsp;Specifies how long to run tests in seconds. The default number is infinite.  
**-e**
&nbsp;&nbsp;&nbsp;&nbsp;This will sets the real client IP in query string following the rules defined in edns-client-subnet.  

**-i**
&nbsp;&nbsp;&nbsp;&nbsp;Specifies interval of queries in seconds. The default number is zero. This option is not supported currently.  
**-P**
&nbsp;&nbsp;&nbsp;&nbsp;Specifies the transport layer protocol to send DNS queries, `udp` or `tcp`. As we known, DNS queries can be send either by UDP or TCP, though UDP is the suggested protocol. The default is `udp`. `tcp` is not supported currently, and it will coming soon.  
**-f**
&nbsp;&nbsp;&nbsp;&nbsp;Specify address family of DNS transport, `inet` or `inet6`. The default is `inet`. `inet6` is not supported currently.  
**-v**
&nbsp;&nbsp;&nbsp;&nbsp;Verbose: report the RCODE of each response on stdout.  
**-h**
&nbsp;&nbsp;&nbsp;&nbsp;Print the usage of dnsperf.  

### Data file format
An example of data file format is shown in `a.out` in project directory.
In the file, the line begin with `#` is recgonized as comment and ommited. Each useful line contains two columns. The first column is the `domain name` to be queried, and the second column is the `query type`. The `query type` currently supported includes:`A`,`NS`,`MD`,`MF`,`CNAME`,`SOA`,`MB`,`MG`,`MR`,`NULL`,`WKS`,`PTR`,`HINFO`,`MINFO`,`MX`,`TXT`,`AAAA`,`SRV`,`NAPTR`,`A6`,`ASFR`,`MAILB`,`MAILA`,`ANY`.

### Performance Statistics
Performance statistics will displayed on your `stdin` after testing. I believe you are intelligent enough to understand the outputs.

### Author
Cobblau, <keycobing@gmail.com>
