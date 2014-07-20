#ifndef _EVENTS_H
#define _EVENTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MOD_RD   0
#define MOD_WR   1

#define MAX_EPOLL_SOCKS  10000

/* info about one given fd */
struct fdtab {
    unsigned char events;
    struct {
        void  *arg;
    } cb[2];
};

int  dns_perf_epoll_init(void);
void dns_perf_epoll_destroy(void);
int  dns_perf_epoll_set_fd(int fd, int dir, void *p);
int  dns_perf_epoll_clear_fd(int fd, int dir);
void *dns_perf_get_arg_by_fd(int fd, int dir);
int  dns_perf_is_fdset(int fd, int dir);
int  dns_perf_do_epoll(long timeout);

#endif
