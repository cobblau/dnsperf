#ifndef _SOCK_H
#define _SOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_BUF_SIZE  8

int dns_perf_open_udp_socket(char *host, unsigned int port, int family);
int dns_perf_open_tcp_socket(char *host, unsigned int port, int family);

int dns_perf_socket_state(int fd);
#endif
