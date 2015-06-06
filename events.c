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

static struct fdtab        *g_fdtab = NULL;     /* array of all the file descriptors */
static dns_perf_eventsys_t *dns_perf_eventsys = NULL;

#ifdef HAVE_EPOLL

static int                  g_epoll_fd;
static struct epoll_event  *g_epoll_events;

static int dns_perf_epoll_init(void)
{
    int                 fd;
    struct epoll_event *ee;
    struct rlimit       limit;

    /* create epoll */
    if ((fd = epoll_create(MAX_EPOLL_SOCKS)) < 0) {
        fprintf(stderr, "Error call epoll_create");
        goto fail_fd;
    }

    /* set fd ulimits */
    limit.rlim_cur = limit.rlim_max = MAX_EPOLL_SOCKS;
    if (setrlimit(RLIMIT_NOFILE, &limit) == -1){
        fprintf(stderr, "Error: set ulimits fd to %d failure. reason:%s\n",
                MAX_EPOLL_SOCKS, strerror(errno));
    }

    /* allocate epoll events */
    ee = (struct epoll_event *) calloc(MAX_EPOLL_SOCKS, sizeof(struct epoll_event));
    if(ee == NULL){
        fprintf(stderr, "Error: alloc epoll event failure.");
        goto fail_ee;
    }

    /* create fdtable */
    g_fdtab = (struct fdtab *) calloc(MAX_EPOLL_SOCKS, sizeof(struct fdtab));
    if(g_fdtab == NULL){
        fprintf(stderr, "Error: alloc fdtab failure.");
        goto fail_fdtab;
    }

    g_epoll_fd = fd;
    g_epoll_events = ee;

    return 0;

fail_fdtab:
    free(ee);
fail_ee:
    close(fd);
fail_fd:
    return -1;
}


static void dns_perf_epoll_destroy(void)
{
    free(g_epoll_events);
    g_epoll_events = NULL;

    free(g_fdtab);
    g_fdtab = NULL;

    if (g_epoll_fd >= 0) {
        close(g_epoll_fd);
        g_epoll_fd = -1;
    }
}

/* do epoll wait */
static int dns_perf_do_epoll(long timeout)
{
    return epoll_wait(g_epoll_fd, g_epoll_events, MAX_EPOLL_SOCKS, timeout);
}

static int dns_perf_epoll_set_fd(int fd, int mod, void *arg)
{
    int                opcode;
    struct epoll_event ev;


    if (g_fdtab[fd].events == 0) {
        opcode = EPOLL_CTL_ADD;
    } else {
        opcode = EPOLL_CTL_MOD;
    }

    if (mod == MOD_RD) {
        g_fdtab[fd].events = EPOLLIN;
    } else if(mod == MOD_WR) {
        g_fdtab[fd].events = EPOLLOUT;
    } else {
        return -1;
    }

    ev.data.fd = fd;
    ev.events = g_fdtab[fd].events;

    if (epoll_ctl(g_epoll_fd, opcode, fd, &ev) < 0) {
        fprintf(stderr, "Error epoll add fd %d error. reason:%s\n", fd,
                strerror(errno));
        return -1;
    }

    g_fdtab[fd].cb[mod].arg = arg;

    return 0;
}

static int dns_perf_epoll_clear_fd(int fd, int mod)
{
    int                opcode;
    struct epoll_event ev;


    if (mod == MOD_RD) {
        g_fdtab[fd].events &= ~EPOLLIN;
    } else if (mod == MOD_WR) {
        g_fdtab[fd].events &= ~EPOLLOUT;
    }
    ev.events = g_fdtab[fd].events;

    if (g_fdtab[fd].events == 0) {
        opcode = EPOLL_CTL_DEL;
    } else {
        opcode = EPOLL_CTL_MOD;
    }

    if (epoll_ctl(g_epoll_fd, opcode, fd, &ev) < 0) {
        fprintf(stderr, "Error epoll delete fd %d failure.", fd);
        return -1;
    }

    g_fdtab[fd].cb[mod].arg = NULL;

    return 0;
}

static void *dns_perf_epoll_get_obj_by_fd(int fd, int mod)
{
    return g_fdtab[fd].cb[mod].arg;
}

static int dns_perf_epoll_is_fdset(int fd, int mod)
{
    if(((mod == MOD_RD) && ((g_fdtab[fd].events & EPOLLIN) == EPOLLIN)) ||
       ((mod == MOD_WR) && ((g_fdtab[fd].events & EPOLLOUT) == EPOLLOUT)))
    {
        return 1;
    }

    return 0;
}


static dns_perf_eventsys_t dns_perf_epoll_eventsys = {
    "epoll",
    dns_perf_epoll_init,
    dns_perf_do_epoll,
    dns_perf_epoll_set_fd,
    dns_perf_epoll_clear_fd,
    dns_perf_epoll_destroy,
    dns_perf_epoll_get_obj_by_fd,
    dns_perf_epoll_is_fdset
};

#endif  /* HAVE_EPOLL */

int dns_perf_set_event_sys() {
    int set = -1;

#ifdef HAVE_EPOLL
    dns_perf_eventsys = &dns_perf_epoll_eventsys;
    set = 0;
#elif defined HAVE_KQUEUE
    dns_perf_eventsys = $dns_perf_kqueue_eventsys;
    set = 0;
#endif

    return -1;
}

#define dns_perf_eventsys_init()                 dns_perf_eventsys->init()
#define dns_perf_eventsys_wait(t)                dns_perf_eventsys->dispatch(t)
#define dns_perf_eventsys_destroy()              dns_perf_eventsys->destroy()
#define dns_perf_eventsys_is_fdset(fd)           dns_perf_eventsys->is_fdset(fd)
#define dns_perf_eventsys_clear_fd(fd, mod)      dns_perf_eventsys->clear_fd(fd, mod)
#define dns_perf_eventsys_set_fd(fd, mod, obj)   dns_perf_eventsys->set_fd(fd, mod, obj)
#define dns_perf_eventsys_get_obj_by_fd(fd, mod) dns_perf_eventsys->get_obj_by_fd(fd, mod)
