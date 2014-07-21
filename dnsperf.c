/*
 * A DNS Performance tool.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

#include <events.h>
#include <sock.h>
/*
 * Defines.
 */

typedef struct timeval timeval_t;

#define TRUE  1
#define FALSE 0

#define DEFAULT_SERVER    "127.0.0.1"
#define DEFAULT_PORT      "53"
#define DEFAULT_TIMEOUT   "3000"      /* ms */
#define DEFAULT_QUERY_NUM "1000"
#define DEFAULT_C_QUERY_NUM "100"

#define MAX_DOMAIN_LEN     255


/* query states */
#define F_UNUSED        0  /* unused */
#define F_CONNECTING    1  /* connect() in progress */
#define F_SENDING       2  /* writing */
#define F_READING       4  /* reading */
#define F_DONE          8  /* all done */

typedef struct data_s {
    unsigned int  qtype;
    unsigned int  len;         /* domain's len */
    char          domain[MAX_DOMAIN_LEN];
} data_t;

typedef struct query_s {
    data_t       *data;

    int           id;
    int           fd;          /* socket fd */
    unsigned int  state;
    timeval_t     sands;
} query_t;



/*
 * Global vars.
 *
 */
char         *g_name_server;
unsigned int  g_name_server_port;
char         *g_data_file_name;
unsigned int  g_timeout;
unsigned int  g_perf_time;
unsigned int  g_query_number;
unsigned int  g_concurrent_query;
unsigned int  g_interval;
int           g_net_family = AF_INET;
int           g_print_rcode_num;
int           g_report_rcode;

timeval_t     g_query_start;
timeval_t     g_query_end;

/* Stores <domain, qtype> read from data `g_data_file_handler' */
data_t       *g_data_array;
int           g_data_array_len;
query_t      *g_query_array;   /* len = g_concurrent_query */

/* epoll vars */
int                  g_epoll_fd;
struct epoll_event  *g_epoll_events;


/* statistics */
unsigned int  g_send_number;
unsigned int  g_recv_number;
unsigned int  g_success_number;  /* 0 */
unsigned int  g_formerr_number;  /* 1 */
unsigned int  g_serverr_number;  /* 2 */
unsigned int  g_nxdomain_number; /* 3 */
unsigned int  g_notimp_number;   /* 4 */
unsigned int  g_refuse_number;   /* 5 */
unsigned int  g_other_number;    /* other rcode */


int           g_stop;  /* 1: running   0: stop */


/*
 * Functions.
 */
void dns_perf_show_info()
{
    printf("\nDNS Performance Testing Tool\n\n");
}

void dns_perf_show_usage()
{
    fprintf(stderr,"\n"
            "Usage: dnsperf [-d datafile] [-s server_addr] [-p port] [-q num_queries]\n"
            "               [-t timeout] [-l limit] [-f family] [-T qps] [-c] [-v] [-h]\n\n"
            "  -d specifies the input data file (default: stdin)\n"
            "  -s sets the dns server's address (default: %s)\n"
            "  -p sets the dns server's port (default: %s)\n"
            "  -t specifies the timeout for query completion in millisecond (default: %s)\n"
            "  -Q specifies the maximum number of queries to be send (default: %s)\n"
            "  -c specifies the number of concurrent queries (default: %s)\n"
            "     dns_perf will randomly pick <domain, type> from data file \n"
            "  -l specifies how long to run tests in seconds (no default)\n"
            "  -i specifies interval of queries in seconds (default: 0)\n"
            "  -f specify address family of DNS transport, inet or inet6 (default: inet)\n"
            "  -v verbose: report the RCODE of each response on stdout\n"
            "  -h print this usage\n"
            "\n",
            DEFAULT_SERVER, DEFAULT_PORT, DEFAULT_TIMEOUT, DEFAULT_QUERY_NUM,
            DEFAULT_C_QUERY_NUM);
}

/*
 * Set the nameserver's address to be queried.
 */
int dns_perf_set_str(char **dst, char *src)
{
    if ((*dst != NULL) && (src != NULL)) {
        if (strcmp(src, *dst) == 0) {
            return 0;
        }
    }

    if (src == NULL || src[0] == '\0') {
        return -1;
    }

	free(*dst);

	if ((*dst = malloc(strlen(src) + 1)) == NULL) {
		fprintf(stderr, "Error allocating memory for server name: %s\n", src);
		return -1;
	}

    memset(*dst, '\0', strlen(src) + 1);
    memcpy(*dst, src, strlen(src));

	return 0;
}

int dns_perf_set_uint(unsigned int *dst, char *src)
{
	unsigned int val;

    val = atol(src);
    if (val <= 0 && val > 65535) {
        return -1;
    }

    *dst = val;

	return 0;
}

void sig_int_handler(int sig)
{
    g_stop = 1;
}

timeval_t dns_perf_timer_sub(timeval_t a, timeval_t b)
{
    timeval_t ret;

    memset(&ret, 0, sizeof(timeval_t));

    ret.tv_usec = a.tv_usec - b.tv_usec;
    ret.tv_sec = a.tv_sec - b.tv_sec;

    if (ret.tv_usec < 0) {
        ret.tv_usec += 1000000;
        ret.tv_sec--;
    }

    return ret;
}

int dns_perf_timer_cmp(timeval_t a, timeval_t b)
{
    if (a.tv_sec > b.tv_sec)
        return 1;
    if (a.tv_sec < b.tv_sec)
        return -1;
    if (a.tv_usec > b.tv_usec)
        return 1;
    if (a.tv_usec < b.tv_usec)
        return -1;
    return 0;
}


timeval_t dns_perf_timer_add_long(timeval_t a, long b)
{
    timeval_t ret;

    memset(&ret, 0, sizeof(timeval_t));

    ret.tv_usec = a.tv_usec + b % 1000000;
    ret.tv_sec = a.tv_sec + b / 1000000;

    if (ret.tv_usec >= 1000000) {
        ret.tv_sec++;
        ret.tv_usec -= 1000000;
    }

    return ret;
}


int dns_perf_valid_qtype(char *qtype)
{
    static char *qtypes[] = {"A", "NS", "MD", "MF", "CNAME", "SOA", "MB", "MG",
        "MR", "NULL", "WKS", "PTR", "HINFO", "MINFO", "MX", "TXT",
        "AAAA", "SRV", "NAPTR", "A6", "AXFR", "MAILB", "MAILA", "*", "ANY"};

    static int qtype_codes[] =  {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16,	28, 33, 35, 38, 252, 253, 254, 255, 255};

    int   qtype_len = sizeof(qtypes) / sizeof(qtypes[0]);
    int   i;

    for (i = 0; i < qtype_len; i++) {
        if (strcasecmp(qtypes[i], qtype) == 0) {
            return qtype_codes[i];
        }
    }

    return -1;
}


int dns_perf_parse_args(int argc, char **argv)
{
    int queryset, perfset;;
    int c;

    while((c = getopt(argc, argv, "d:s:p:t:l:Q:q:i:f:T:c:vh")) != -1) {

        switch (c) {
        case 'd':
            if (dns_perf_set_str(&g_data_file_name, optarg) == -1) {
                fprintf(stderr, "Error setting datafile %s\n", optarg);
                return -1;
            }
            break;

        case 's':
            if (dns_perf_set_str(&g_name_server, optarg) == -1) {
                fprintf(stderr, "Error setting name_server %s\n", optarg);
                return -1;
            }
            break;

        case 'p':
            if (dns_perf_set_uint(&g_name_server_port, optarg) == -1) {
                fprintf(stderr, "Error setting name_server's port %s\n", optarg);
                return -1;
            }
            break;

        case 't':
            if (dns_perf_set_uint(&g_timeout, optarg) == -1) {
                fprintf(stderr, "Error setting timeout %s\n", optarg);
                return -1;
            }
            break;

        case 'l':
            if (dns_perf_set_uint(&g_perf_time, optarg) == -1) {
                fprintf(stderr, "Error setting query time %s\n", optarg);
                return -1;
            }
            perfset = TRUE;
            break;

        case 'Q':
            if (dns_perf_set_uint(&g_query_number, optarg) == -1) {
                fprintf(stderr, "Error setting query number %s\n", optarg);
                return -1;
            }
            queryset = TRUE;
            break;

        case 'c':
            if (dns_perf_set_uint(&g_concurrent_query, optarg) == -1) {
                fprintf(stderr, "Error setting concurrent query num %s\n", optarg);
                return -1;
            }
            break;

        case 'i':
            if (dns_perf_set_uint(&g_interval, optarg) == -1) {
                fprintf(stderr, "Error setting interval number %s\n", optarg);
                return -1;
            }
            break;

        case 'f':
            if (strcmp(optarg, "inet") == 0) {
                g_net_family = AF_INET;
            } else if (strcmp(optarg, "inet6") == 0) {
                g_net_family = AF_INET6;
            } else {
                fprintf(stderr, "Invalid address family: %s\n", optarg);
                return -1;
            }
            break;

        case 'v':
            g_report_rcode = TRUE;
            break;

        case 'h':
            return -1;

        default:
            fprintf(stderr, "Invalid option: %s\n", optarg);
            return -1;
        }
    }

    if (queryset == TRUE && perfset == TRUE) {
        fprintf(stderr, "-Q and -l is exclusive, please set only one\n");
        return -1;
    }

    if (g_perf_time != 0) {
        g_query_number = 100000000;
    }

    return 0;
}


/*
 * dns_perf_data_array_init:
 *     fill 'g_query_array' with information read from 'g_data_file_handler'
 */
int dns_perf_data_array_init()
{
    FILE    *file;
    char     buf[1024], domain[255], qtype[10];
    int      len = 0, qtype_n;
    data_t  *d;

    if (g_data_file_name == NULL) {
        return -1;
    }


    if ((file = fopen(g_data_file_name, "r")) == NULL) {
        return -1;
    }

    /* Calculate how many useful lines */
    while(fgets(buf, 1024, file) != 0) {
        if (buf[0] == '#' || buf[0] == '\n') {
            continue;
        }

        len++;
    }

    if ((g_data_array = calloc(len, sizeof(data_t))) == NULL) {
        fprintf(stderr, "Malloc memory error");
        goto finish;
    }

    rewind(file);
    g_data_array_len = 0;
    while(fgets(buf, 1024, file) != 0) {
        if (buf[0] == '#' || buf[0] == '\n') {
            continue;
        }

        if (sscanf(buf, "%s %s", domain, qtype) == EOF) {
            fprintf(stderr, "Error string in data file:%s\n", buf);
            goto finish;
        }

        if (strlen(domain) > MAX_DOMAIN_LEN) {
            fprintf(stderr, "Error domain name too long:%s\n", domain);
            goto finish;
        }

        if ((qtype_n = dns_perf_valid_qtype(qtype)) == -1) {
            fprintf(stderr, "Error unknown qtype:%s\n", qtype);
            goto finish;
        }

        d = &g_data_array[g_data_array_len];
        d->len = strlen(domain);
        memcpy(d->domain, domain, d->len);
        d->qtype = qtype_n;

        g_data_array_len++;
    }

 finish:

    if (fclose(file) != 0) {
        free(g_data_array);
        return -1;
    }

    return 0;
}

/*
 * Desc:
 *     Package q to DNS message and send to remote name server.
 *
 */
int dns_perf_query_dispatch(query_t *q)
{
    static unsigned short query_id = 0;
	static u_char output[PACKETSZ + 1];
	int           out_len = PACKETSZ;
	int           ret;
	unsigned short int net_id;
	char              *id_ptr;
	HEADER            *hp = (HEADER *)output;
    timeval_t          tv;


	out_len = res_mkquery(QUERY, q->data->domain, C_IN, q->data->qtype, NULL, 0,
				 NULL, output, PACKETSZ);
	if (out_len == -1) {
		fprintf(stderr, "Failed to create query packet: %s %d\n", q->data->domain,
                q->data->qtype);
		return -1;
	}
	hp->rd = 1;

    query_id++;
    q->id = query_id;

    net_id = htons(query_id);
    id_ptr = (char *) &net_id;

	output[0] = id_ptr[0];
	output[1] = id_ptr[1];

    /* send query to remote name server */
    gettimeofday(&tv, NULL);
    q->sands = dns_perf_timer_add_long(tv, g_timeout * 1000);

    ret = send(q->fd, output, out_len, 0);
    if (ret < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            goto error;
        }

        q->state = F_SENDING;
        if (dns_perf_epoll_set_fd(q->fd, MOD_WR, q) == -1) {
            fprintf(stderr, "Error set write fd:%d\n", q->fd);
            goto error;
        }

    } else { /* already send */
        if (ret != out_len) {
            fprintf(stderr, "Error send request partially\n");
            goto error;
        }

        q->state = F_READING;
        if (dns_perf_epoll_set_fd(q->fd, MOD_RD, q) == -1) {
            fprintf(stderr, "Error set read fd:%d\n", q->fd);
            goto error;
        }
    }

    g_send_number++;
    return 0;

 error:
    close(q->fd);
    q->state = F_UNUSED;
    return 0;
}



int dns_perf_query_process_response(query_t *q, unsigned short id, unsigned short flag)
{
    /* 做一些统计工作 */
    if (q->id != id) {
        return -1;  /* TODO: can be happened */
    }

    g_recv_number++;

    switch(flag) {
    case 0:
        g_success_number++;
        break;

    case 1:
        g_formerr_number++;
        break;

    case 2:
        g_serverr_number++;
        break;

    case 3:
        g_nxdomain_number++;
        break;

    case 4:
        g_notimp_number++;
        break;

    case 5:
        g_refuse_number++;
        break;

    default:
        g_other_number++;
        break;
    }

    return 0;
}


int dns_perf_query_read(query_t *q)
{
	static u_char input[1024];
    int           ret;
    unsigned short  id;
    unsigned short  flags;


    ret = recv(q->fd, input, 1024, 0);

    if (ret < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            close(q->fd);
            q->state = F_UNUSED;
            return 0;
        }

        if (dns_perf_epoll_set_fd(q->fd, MOD_RD, q) == -1) {
            close(q->fd);
            q->state = F_UNUSED;
            return 0;
        }
    } else {
        close(q->fd);
        q->state = F_UNUSED;

        id = input[0] * 256 + input[1];
        flags = input[2] * 256 + input[3];

        dns_perf_query_process_response(q, id, flags & 0xF);
    }

    return 0;
}


inline int dns_perf_cancel_timeout_query()
{
    int       i;
    query_t  *query;
    timeval_t now, diff;

    /* Deal with timeout */
    gettimeofday(&now, NULL);
    for (i = 0; i < g_concurrent_query ; i++) {

        query = &g_query_array[i];

        if (query->state == F_UNUSED) {
            continue;
        }

        diff = dns_perf_timer_sub(now, query->sands);
        if (diff.tv_sec * 1000000 + diff.tv_usec >= 0) {
            /* delete timeouted queries */
            if (query->state == F_SENDING) {
                dns_perf_epoll_clear_fd(query->fd, MOD_WR);
            } else if (query->state == F_READING) {
                dns_perf_epoll_clear_fd(query->fd, MOD_RD);
            }

            close(query->fd);
            query->state = F_UNUSED;
        }
    }

    return 0;
}




/*
 * dns_perf_prepare:
 *     Do some preparation before quering.
 *   1. allocate query_array
 */
inline int dns_perf_prepare()
{
    query_t    *q;
    int         i, index;

    g_query_array = calloc(g_concurrent_query, sizeof(query_t));
    if (g_query_array == NULL) {
        fprintf(stderr, "Error memory low");
        return -1;
    }

    for (i = 0; i < g_concurrent_query; i++) {

        index = random() % g_data_array_len;

        q = &g_query_array[i];

        q->data = &g_data_array[index];
        q->id = -1;
        q->fd = -1;
        q->state = F_UNUSED;
    }

    return 0;
}


/*
 * Whip query_t to make it as busier as possible.
 *
 */
inline int dns_perf_whip_query()
{
    int i;
    query_t  *q;

    for (i = 0; i < g_concurrent_query; i++) {

        q = &g_query_array[i];

        if (q->state != F_UNUSED) {
            continue;
        }

        q->fd = dns_perf_open_udp_socket(g_name_server, g_name_server_port,
                                         g_net_family);
        if (q->fd == -1) {
            fprintf(stderr, "Error create udp socket failed\n");
            return -1;
        }

        q->state = F_CONNECTING;

        dns_perf_query_dispatch(q);
    }

    return 0;
}


inline int dns_perf_clear_query()
{
    int i;
    query_t  *q;

    for (i = 0; i < g_concurrent_query; i++) {

        q = &g_query_array[i];

        if (q->state != F_UNUSED) {
            close(q->fd);
        }

        q->state = F_UNUSED;
    }

    return 0;
}


inline void dns_perf_statistic()
{
    timeval_t     diff;
    unsigned int  msec;
    double        elapse, qps;


    diff = dns_perf_timer_sub(g_query_end, g_query_start);
    msec = diff.tv_sec * 1000 + diff.tv_usec / 1000;

    printf("\n[Status]DNS Query Performance Testing Finish\n");
    printf("[Result]Quries sent:\t\t%d\n", g_send_number);
    printf("[Result]Quries completed:\t%d\n", g_recv_number);
    printf("[Result]Complete percentage:\t%.2f%\n\n", g_recv_number * 100.0 / g_send_number);

    if (g_report_rcode) {
        printf("[Result]Rcode=Success:\t%d\n\n", g_success_number);
        printf("[Result]Rcode=FormatError:\t%d\n\n", g_formerr_number);
        printf("[Result]Rcode=ServerError:\t%d\n\n", g_serverr_number);
        printf("[Result]Rcode=NXDOMAIN:\t%d\n\n", g_nxdomain_number);
        printf("[Result]Rcode=NotImp:\t%d\n\n", g_notimp_number);
        printf("[Result]Rcode=Refuse:\t%d\n\n", g_refuse_number);
        printf("[Result]Rcode=Others:\t%d\n\n", g_other_number);
    }

    elapse = msec * 1.0 / 1000;
    printf("[Result]Elapsed time(s):\t%.5f\n\n", elapse);


    qps = g_send_number / elapse;
    printf("[Result]Queries Per Second:\t%.5f\n", qps);
}


/*
 * dns_perf_setup:
 *     Init data.
 */
int dns_perf_setup(int argc, char **argv)
{

    if (dns_perf_set_str(&g_name_server, DEFAULT_SERVER) == -1) {
        fprintf(stderr, "%s: Unable to set default name_server\n", argv[0]);
        return -1;
    }

    if (dns_perf_set_uint(&g_name_server_port, DEFAULT_PORT) == -1) {
        fprintf(stderr, "%s: Unable to set default name_server's port\n", argv[0]);
        return -1;
    }

    if (dns_perf_set_uint(&g_timeout, DEFAULT_TIMEOUT) == -1) {
        fprintf(stderr, "%s: Unable to set default timeout\n", argv[0]);
        return -1;
    }

    if (dns_perf_set_uint(&g_query_number, DEFAULT_QUERY_NUM) == -1) {
        fprintf(stderr, "%s: Unable to set default query number\n", argv[0]);
        return -1;
    }

    if (dns_perf_set_uint(&g_concurrent_query, DEFAULT_C_QUERY_NUM) == -1) {
        fprintf(stderr, "%s: Unable to set default concurrent query number\n", argv[0]);
        return -1;
    }

    if (dns_perf_parse_args(argc, argv) == -1) {
        dns_perf_show_usage();
        return -1;
    }

    if (dns_perf_data_array_init() == -1) {
        return -1;
    }

    if (dns_perf_epoll_init() == -1) {
        return -1;
    }

    return 0;
}


int main(int argc, char** argv)
{
    int     i, nevents;
    int     fd;
    query_t *query;
    timeval_t  now, age;


    dns_perf_show_info();
    signal(SIGINT, sig_int_handler);

    if (dns_perf_setup(argc, argv) == -1) {
        return -1;
    }

    printf("[Status] Processing query data\n");
    if (dns_perf_prepare() == -1) {
        return -1;
    }

    printf("[Status] Sending queries to %s:%d\n", g_name_server, g_name_server_port);
    gettimeofday(&g_query_start, NULL);

    /* how long can you live */
    age = dns_perf_timer_add_long(g_query_start, g_perf_time * 1000000);

    if (dns_perf_whip_query() == -1) {
        return -1;
    }

    while (g_stop == 0) {
        nevents = dns_perf_do_epoll(g_timeout);

        for (i = 0; i < nevents; i++) {
            fd = g_epoll_events[i].data.fd;

            if (g_epoll_events[i].events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) {
                query = dns_perf_get_arg_by_fd(fd, MOD_WR);
                dns_perf_epoll_clear_fd(fd, MOD_WR);

                dns_perf_query_read(query);
            }

            if (g_epoll_events[i].events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
                query = dns_perf_get_arg_by_fd(fd, MOD_RD);
                dns_perf_epoll_clear_fd(fd, MOD_RD);

                dns_perf_query_read(query);
            }
        }

        dns_perf_cancel_timeout_query();

        /* Is time up? */
        if (g_perf_time != 0) {
            gettimeofday(&now, NULL);
            if (dns_perf_timer_cmp(now, age) > 0) {
                printf("time up");
                break;
            }
        }

        /* Is query number overflowed? */
        if (g_send_number >= g_query_number) {
            break;
        }

        dns_perf_whip_query();
    }

    gettimeofday(&g_query_end, NULL);

    dns_perf_statistic();

    dns_perf_clear_query();

    free(g_data_array);
    free(g_query_array);
    free(g_name_server);
    free(g_data_file_name);

    dns_perf_epoll_destroy();

    return 0;
}
