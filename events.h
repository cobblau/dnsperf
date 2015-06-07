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


extern dns_perf_eventsys_t *dns_perf_eventsys;

int dns_perf_set_event_sys();
#define dns_perf_eventsys_init()                 dns_perf_eventsys->init()
#define dns_perf_eventsys_destroy()              dns_perf_eventsys->destroy()
#define dns_perf_eventsys_dispatch(t)            dns_perf_eventsys->dispatch(t)
#define dns_perf_eventsys_is_fdset(fd)           dns_perf_eventsys->is_fdset(fd)
#define dns_perf_eventsys_clear_fd(fd, mod)      dns_perf_eventsys->clear_fd(fd, mod)
#define dns_perf_eventsys_set_fd(fd, mod, obj)   dns_perf_eventsys->set_fd(fd, mod, obj)
#define dns_perf_eventsys_get_obj_by_fd(fd, mod) dns_perf_eventsys->get_obj_by_fd(fd, mod)

#endif
