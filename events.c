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

#include <events.h>

#ifdef HAVE_EPOLL
#include <sys/epoll.h>
#endif

#ifdef HAVE_KQUEUE
#include <sys/event.h>
#endif


dns_perf_eventsys_t *dns_perf_eventsys = NULL;

#ifdef HAVE_EPOLL

typedef struct dns_perf_epoll_data_s {
    int                 fd;         /* epoll fd */
    struct fddata      *fdtab;      /* fd data in epoll */
    struct epoll_event *events;
    int                 fd_size;
} dns_perf_epoll_data_t;

static dns_perf_epoll_data_t *ep = NULL;

static int dns_perf_epoll_init(void)
{
    struct rlimit       limit;

    if ((ep = (dns_perf_epoll_data_t *)malloc(sizeof(dns_perf_epoll_data_t))) == NULL) {
        return -1;
    }

    /* create epoll */
    if ((ep->fd = epoll_create(MAX_EVENT_SOCKS)) < 0) {
        fprintf(stderr, "Error call epoll_create");
        goto fail_fd;
    }

    /* set fd ulimits */
    limit.rlim_cur = limit.rlim_max = MAX_EVENT_SOCKS;
    if (setrlimit(RLIMIT_NOFILE, &limit) == -1){
        fprintf(stderr, "Error: set ulimits fd to %d failure. reason:%s\n",
                MAX_EVENT_SOCKS, strerror(errno));
        goto fail_limit;
    }

    /* allocate epoll events */
    ep->fd_size = MAX_EVENT_SOCKS;
    if ((ep->events = (struct epoll_event *) calloc(ep->fd_size,
                                                    sizeof(struct epoll_event))) == NULL)
    {
        fprintf(stderr, "Error: alloc epoll event failure.");
        goto fail_limit;
    }

    /* create fdtable */
    if ((ep->fdtab = (struct fddata *) calloc(ep->fd_size,
                                              sizeof(struct fddata))) == NULL)
    {
        fprintf(stderr, "Error: alloc fddata failure.");
        goto fail_fdtab;
    }

    return 0;

fail_fdtab:
    free(ep->events);
fail_limit:
    close(ep->fd);
fail_fd:
    free(ep);
    ep = NULL;
    return -1;
}


static int dns_perf_epoll_destroy(void)
{
    if (ep) {
        free(ep->fdtab);
        free(ep->events);

        if (ep->fd) {
            close(ep->fd);
        }

        free(ep);

        ep = NULL;
    }

    return 0;
}


static int dns_perf_epoll_clear_fd(int fd, int mod)
{
    int                opcode;
    struct epoll_event ev;


    if (mod == MOD_RD) {
        ep->fdtab[fd].events &= ~EPOLLIN;
    } else if (mod == MOD_WR) {
        ep->fdtab[fd].events &= ~EPOLLOUT;
    }
    ev.events = ep->fdtab[fd].events;

    if (ep->fdtab[fd].events == 0) {
        opcode = EPOLL_CTL_DEL;
    } else {
        opcode = EPOLL_CTL_MOD;
    }

    if (epoll_ctl(ep->fd, opcode, fd, &ev) < 0) {
        fprintf(stderr, "Error epoll delete fd %d failure.", fd);
        return -1;
    }

    ep->fdtab[fd].cb[mod].arg = NULL;

    return 0;
}

static void *dns_perf_epoll_get_obj_by_fd(int fd, int mod)
{
    return ep->fdtab[fd].cb[mod].arg;
}


static int dns_perf_epoll_do_wait(long timeout)
{
    int i, fd;
    int nevents;
    dns_perf_event_ops_t *op;

    nevents = epoll_wait(ep->fd, ep->events, ep->fd_size, timeout);

    if (nevents < 0) {
        fprintf(stderr, "epoll_wait error:%s", strerror(errno));
        return -1;
    }

    for (i = 0; i < nevents; i++) {
        fd = ep->events[i].data.fd;

        if (ep->events[i].events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) {
            op = (dns_perf_event_ops_t *) ep->fdtab[fd].cb[MOD_WR].arg;
            dns_perf_epoll_clear_fd(fd, MOD_WR);
            op->send((void *) op);
        }

        if (ep->events[i].events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
            op = (dns_perf_event_ops_t *) ep->fdtab[fd].cb[MOD_RD].arg;
            dns_perf_epoll_clear_fd(fd, MOD_RD);
            op->recv((void *) op);
        }
    }

    return 0;
}

static int dns_perf_epoll_set_fd(int fd, int mod, void *arg)
{
    int                opcode;
    struct epoll_event ev;


    if (fd > ep->fd_size) {
        // TODO: expand
    }

    if (ep->fdtab[fd].events == 0) {
        opcode = EPOLL_CTL_ADD;
    } else {
        opcode = EPOLL_CTL_MOD;
    }

    if (mod == MOD_RD) {
        ep->fdtab[fd].events = EPOLLIN;
    } else if(mod == MOD_WR) {
        ep->fdtab[fd].events = EPOLLOUT;
    } else {
        return -1;
    }

    ev.data.fd = fd;
    ev.events = ep->fdtab[fd].events;

    if (epoll_ctl(ep->fd, opcode, fd, &ev) < 0) {
        fprintf(stderr, "Error epoll add fd %d error. reason:%s\n", fd,
                strerror(errno));
        return -1;
    }

    ep->fdtab[fd].cb[mod].arg = arg;

    return 0;
}

static int dns_perf_epoll_is_fdset(int fd, int mod)
{
    if(((mod == MOD_RD) && ((ep->fdtab[fd].events & EPOLLIN) == EPOLLIN)) ||
       ((mod == MOD_WR) && ((ep->fdtab[fd].events & EPOLLOUT) == EPOLLOUT)))
    {
        return 1;
    }

    return 0;
}


static dns_perf_eventsys_t dns_perf_epoll_eventsys = {
    "epoll",
    dns_perf_epoll_init,
    dns_perf_epoll_do_wait,
    dns_perf_epoll_set_fd,
    dns_perf_epoll_clear_fd,
    dns_perf_epoll_destroy,
    dns_perf_epoll_get_obj_by_fd,
    dns_perf_epoll_is_fdset
};

#endif  /* HAVE_EPOLL */

#ifdef HAVE_KQUEUE

typedef struct dns_perf_kqueue_data_s {
    int            fd; /* kqueue fd */
    struct fddata *fdtab;
    struct kevent *monlist;  /* events we want to monitor */
    struct kevent *evtlist;
    int            fd_size;
    int            evtlist_size;
} dns_perf_kqueue_data_t;

static dns_perf_kqueue_data_t *kq = NULL;

static int dns_perf_kqueue_init()
{

    if ((kq = (dns_perf_kqueue_data_t *) malloc(sizeof(dns_perf_kqueue_data_t)))
        == NULL)
    {
        fprintf(stderr, "Error memory low");
        goto fail_kd;
    }

    if ((kq->fd = kqueue()) == -1) {
        fprintf(stderr, "Error create kqueue");
        goto fail_kqueue;
    }

    kq->fd_size = MAX_EVENT_SOCKS;
    if ((kq->fdtab = (struct fddata*) calloc(kq->fd_size,
                                             sizeof(struct fddata))) == NULL)
    {
        fprintf(stderr, "Error memory low");
        goto fail_fdlist;
    }

    if ((kq->monlist = (struct kevent*) calloc(kq->fd_size,
                                               sizeof(struct kevent))) == NULL)
    {
        fprintf(stderr, "Error memory low");
        goto fail_monlist;
    }

    kq->evtlist_size = kq->fd_size;
    if ((kq->evtlist = (struct kevent*) calloc(kq->evtlist_size,
                                               sizeof(struct kevent))) == NULL)
    {
        fprintf(stderr, "Error memory low");
        goto fail_evtlist;
    }

    return 0;

 fail_evtlist:
    free(kq->monlist);
 fail_monlist:
    free(kq->fdtab);
 fail_fdlist:
    close(kq->fd);
    kq->fd = -1;
 fail_kqueue:
    free(kq);
    kq = NULL;
 fail_kd:
    return -1;
}

static int dns_perf_kqueue_destroy()
{
    free(kq->fdtab);
    free(kq->evtlist);
    close(kq->fd);
    free(kq);

    return 0;
}

static void *dns_perf_kqueue_get_obj_by_fd(int fd, int mod)
{
    return kq->fdtab[fd].cb[mod].arg;
}


static int dns_perf_kqueue_clear_fd(int fd, int mod)
{
    int  filter;


    if (mod == MOD_RD) {
        filter = EVFILT_READ;
    } else if (mod == MOD_WR) {
        filter = EVFILT_READ;
    } else {
        return -1;
    }

    kq->fdtab[fd].events &= ~filter;

    EV_SET(&kq->monlist[fd], fd, filter, EV_DELETE, 0, 0, 0);

    kq->fdtab[fd].cb[mod].arg = NULL;

    return 0;
}

static int dns_perf_kqueue_do_kevent(long timeout)
{
    int             i, nevents, fd, filter;
    struct timespec ts, *tsp;
    dns_perf_event_ops_t *op;

    if (timeout == 0) {
        tsp = NULL;
    } else {
        ts.tv_sec = (time_t) timeout / 1000;
        ts.tv_nsec = (long) (timeout % 1000) * 1000000;
        tsp = &ts;
    }

    nevents = kevent(kq->fd, kq->monlist, kq->fd_size, kq->evtlist, kq->fd_size, tsp);

    if (nevents < 0) {
        fprintf(stderr, "kevent error:%s", strerror(errno));
        return -1;
    }

    for (i = 0; i < nevents; i++) {
        if (kq->evtlist[i].flags & EV_ERROR) {
            fprintf(stderr, "EV_ERROR");
            continue;
        }

        fd = kq->evtlist[i].ident;
        filter = kq->evtlist[i].filter;

        if (filter == EVFILT_READ) {
            op = (dns_perf_event_ops_t *) dns_perf_kqueue_get_obj_by_fd(fd, MOD_RD);
            dns_perf_kqueue_clear_fd(fd, MOD_RD);
            op->recv((void *) op);
        }

        if (filter == EVFILT_WRITE) {
            op = (dns_perf_event_ops_t *) dns_perf_kqueue_get_obj_by_fd(fd, MOD_WR);
            dns_perf_kqueue_clear_fd(fd, MOD_WR);
            op->send((void *) op);
        }

    }

    return 0;
}

static int dns_perf_kqueue_set_fd(int fd, int mod, void *arg)
{
    int             filter;


    if (fd > kq->fd_size) {
        //TODO: expand lists
    }

    if (mod == MOD_RD) {
        filter = EVFILT_READ;
    } else if(mod == MOD_WR) {
        filter = EVFILT_WRITE;
    } else {
        return -1;
    }

    /* Deleted. reason: see comments below
    if (kq->fdtab[fd].events != 0) {
        EV_SET(&kq->monlist[fd], fd, filter, EV_CLEAR, 0, 0, 0);
        kq->fdtab[fd].events = 0;
    }
    */

    kq->fdtab[fd].events = filter;

    /*
     * Re-adding an existing event will modify the parameters of the original event,
     * and not result in a duplicate entry.
     * Adding an event automatically enables it, unless overridden by the EV_DISABLE flag.
     */
    EV_SET(&kq->monlist[fd], fd, filter, EV_ADD, 0, 0, 0);

    kq->fdtab[fd].cb[mod].arg = arg;

    return 0;
}


static int dns_perf_kqueue_is_fdset(int fd, int mod)
{
    if(((mod == MOD_RD) && ((kq->fdtab[fd].events & EVFILT_READ) == EVFILT_READ)) ||
       ((mod == MOD_WR) && ((kq->fdtab[fd].events & EVFILT_WRITE) == EVFILT_WRITE)))
    {
        return 1;
    }

    return 0;
}


static dns_perf_eventsys_t dns_perf_kqueue_eventsys = {
    "epoll",
    dns_perf_kqueue_init,
    dns_perf_kqueue_do_kevent,
    dns_perf_kqueue_set_fd,
    dns_perf_kqueue_clear_fd,
    dns_perf_kqueue_destroy,
    dns_perf_kqueue_get_obj_by_fd,
    dns_perf_kqueue_is_fdset
};

#endif /* HAVE_KQUEUE */

int dns_perf_set_event_sys() {
    int set = -1;

#ifdef HAVE_EPOLL
    dns_perf_eventsys = &dns_perf_epoll_eventsys;
    set = 0;
#elif defined HAVE_KQUEUE
    dns_perf_eventsys = &dns_perf_kqueue_eventsys;
    set = 0;
#endif

    return set;
}
