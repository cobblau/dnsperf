/*
 * This file if part of dnsperf.
 *
 * Copyright (C) 2014 Cobblau
 *
 * dnsperf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dnsperf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <sock.h>

int dns_perf_open_udp_socket(char *host, unsigned int port, int family)
{
    int fd;
    struct sockaddr_in  addr;
    int bufsize, val;

    if ((fd = socket(family, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Error create udp socket\n");
        return -1;
    }

    memset((void *)&addr, 0, sizeof(addr));
    addr.sin_family = family;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
        fprintf(stderr, "Error connect udp socket\n");
        return -1;
    }

    bufsize = 1024 * DEFAULT_BUF_SIZE;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &bufsize,
                   sizeof(bufsize)) < 0 )
    {
        fprintf(stderr, "Warning:  setsockbuf(SO_RCVBUF) failed\n");
    }

    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &bufsize,
                   sizeof(bufsize)) < 0)
    {
        fprintf(stderr, "Warning:  setsockbuf(SO_SNDBUF) failed\n");
    }

	val = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, val | O_NONBLOCK);

    return fd;
}


int dns_perf_open_tcp_socket(char *host, unsigned int port, int family)
{
    int fd;
    struct sockaddr_in  addr;
    int bufsize, val;

    if ((fd = socket(family, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Error create tcp socket\n");
        return -1;
    }

    memset((void *)&addr, 0, sizeof(addr));
    addr.sin_family = family;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    bufsize = 1024 * DEFAULT_BUF_SIZE;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &bufsize,
                   sizeof(bufsize)) < 0 )
    {
        fprintf(stderr, "Warning:  setsockbuf(SO_RCVBUF) failed\n");
    }

    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &bufsize,
                   sizeof(bufsize)) < 0)
    {
        fprintf(stderr, "Warning:  setsockbuf(SO_SNDBUF) failed\n");
    }

	val = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, val | O_NONBLOCK);

    return fd;
}

int dns_perf_socket_state(int fd)
{
	int       status = 0;
	socklen_t slen;

	/* Check file descriptor */
	slen = sizeof (status);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *) &status, &slen) == -1) {
        return -1;
    }

    return 0;
}
