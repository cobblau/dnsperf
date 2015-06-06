#ifndef _EVENTS_H
#define _EVENTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
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

typedef struct dns_perf_eventsys_ops {
    const char *name;
    int (*init)(void);
    int (*dispatch)(long timeout);
    int (*set_fd)(int fd, int dir, void *p);
    int (*clear_fd)(int fd, int dir);
    int (*destroy)(void);
    int (*get_obj_by_fd)(int fd, int dir);
    int (*is_fdset)(int fd, int dir);
} dns_perf_eventsys_t;

#endif
