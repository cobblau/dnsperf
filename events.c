
#include <events.h>


extern int                  g_epoll_fd;
extern struct epoll_event  *g_epoll_events;

static struct fdtab *g_fdtab = NULL;     /* array of all the file descriptors */

int dns_perf_epoll_init(void)
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


void dns_perf_epoll_destroy(void)
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
int dns_perf_do_epoll(long timeout)
{
    return epoll_wait(g_epoll_fd, g_epoll_events, MAX_EPOLL_SOCKS, timeout);
}

int dns_perf_epoll_set_fd(int fd, int mod, void *arg)
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

int dns_perf_epoll_clear_fd(int fd, int mod)
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

void *dns_perf_get_arg_by_fd(int fd, int mod)
{
    return g_fdtab[fd].cb[mod].arg;
}

int dns_perf_is_fdset(int fd, int mod)
{
    if(((mod == MOD_RD) && ((g_fdtab[fd].events & EPOLLIN) == EPOLLIN)) ||
       ((mod == MOD_WR) && ((g_fdtab[fd].events & EPOLLOUT) == EPOLLOUT)))
    {
        return 1;
    }

    return 0;
}
