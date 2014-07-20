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
    return 0;
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
