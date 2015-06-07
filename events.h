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

#define MAX_EVENT_SOCKS  10000

/* info about one given fd */
struct fddata {
    int events;
    struct {
        void  *arg;
    } cb[2];
};

typedef struct dns_perf_event_ops_s {
    int (*send)(void *arg);
    int (*recv)(void *arg);
} dns_perf_event_ops_t;


typedef struct dns_perf_eventsys_s {
    const char *name;
    int   (*init)(void);
    int   (*dispatch)(long timeout);
    int   (*set_fd)(int fd, int mod, void *p);
    int   (*clear_fd)(int fd, int mod);
    int   (*destroy)(void);
    void *(*get_obj_by_fd)(int fd, int mod);
    int   (*is_fdset)(int fd, int mod);
} dns_perf_eventsys_t;

#endif
