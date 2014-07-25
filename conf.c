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


/*
 *
 * NOTE: This file is not used currently.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "conf.h"


static int conf_read_file(char *file_name, char *buf, int len);
static int conf_parse(conf_t *cf, char *buf, int len);
static conf_command_t *conf_find_command(conf_command_t *commands, char *p, int len);
static int conf_get_int(void *conf, unsigned int offset, char *s, int len);
static int conf_get_on_off(void *conf, unsigned int offset, char *s, int len);
static int conf_get_string(void *conf, unsigned int offset, char *s, int len);
static int conf_get_iarray(void *conf, unsigned int offset, char *s, int len);


static int my_atoi(char *p, int len);
static char *my_substring(char *pos, char *last, char **start, int *len);


static conf_command_t main_commands[] = {
    { "name_server",
      conf_get_string,
      offsetof(conf_t, name_server) },

    { "port",
      conf_get_int,
      offsetof(conf_t, port) },

    { "timeout",
      conf_get_int,
      offsetof(conf_t, timeout) },

    { "max_query",
      conf_get_int,
      offsetof(conf_t, max_query) },

    { "concurrent_query",
      conf_get_int,
      offsetof(conf_t, concurrent_query) },

    { "running_time",
      conf_get_int,
      offsetof(conf_t, running_time) },

    { "protocol",
      conf_get_string,
      offsetof(conf_t, protocol) },

    { "address_family",
      conf_get_string,
      offsetof(conf_t, addr_family) },

    { "verbose",
      conf_get_on_off,
      offsetof(conf_t, verbose) },

    { "", NULL, 0 }
};

int
conf_parse_file(char *conf_file_name, conf_t **conf)
{
    int       len;
    char      buf[8192] = {0};
    conf_t   *cf;

    if ((cf = malloc(sizeof(conf_t))) == NULL) {
        return -1;
    }

    len = conf_read_file(conf_file_name, buf, 8192);
    if (len == -1) {
        return -1;
    }

    if (conf_parse(cf, buf, len) == -1) {
        return -1;
    }

    *conf = cf;
    return 0;
}

static int
conf_read_file(char *file_name, char *buf, int len)
{
    int fd;
    int n, size;

    fd = open(file_name, O_RDONLY, 0);
    if (fd == -1) {
        return -1;
    }

    n = size = 0;
    for (;;) {
        n = read(fd, buf + size, len);

        if (n == 0) {
            break;
        }

        size += n;
        len -= n;

        if (len <= 0) {
            /* config file is too large */
            return -1;
        }
    }

    close(fd);
    return size;
}

static int
conf_parse(conf_t *cf, char *buf, int len)
{
    conf_command_t  *cmd;
    char *begin, *end;
    char *p, *p1, *p2, ch;
    int skip;

    p1 = p2 = NULL;
    cmd = NULL;
    begin = buf;
    end = buf + len;

    enum {
        directive_start = 0,
        directive_name,
        directive_arg
    } state;

    state = directive_start;
    skip = 0;

    for (p = begin; p < end; p++) {
        ch = *p;

        if (ch == '#') {
            skip = 1;
            continue;
        }

        if (skip) {
            if (ch == '\n') {
                skip = 0;
            }
            continue;
        }

        switch (state) {
        case directive_start:
            if (is_digit(ch) || is_letter(ch) || ch == '_') {
                p1 = p;
                state = directive_name;
                break;
            }

            if (!is_blank(ch)) {
                return -1;
            }

            break;

        case directive_name:

            if (is_blank(ch)) {
                p2 = p;

                cmd = conf_find_command(main_commands, p1, p2 - p1);
                if (cmd == NULL) {
                    fprintf(stderr, "CONF: not found conf command %s", p1);
                    return -1;
                }

                p1 = p;
                state = directive_arg;
                break;
            }

            if (!is_digit(ch) && !is_letter(ch) && ch != '_') {
                fprintf(stderr, "CONF: invalid character(%c) in command", ch);
                return -1;
            }

            break;

        case directive_arg:

            if (ch == ';') {
                p2  = p;

                while(is_blank(*p1)) p1++;

                if (cmd->handler(cf, cmd->offset, p1, p2 - p1) != 0) {
                    fprintf(stderr, "CONF: get value error!");
                    return -1;
                }

                state = directive_start;
            }

            break;

        }
    }

    if (state != directive_start) {
        fprintf(stderr, "CONF: command not complete");
        return -1;
    }

    return 0;
}

static conf_command_t *
conf_find_command(conf_command_t *commands, char *p, int len)
{
    int i;
    conf_command_t *cmd;

    for (i = 0; ; i++) {
        cmd = main_commands + i;
        if (cmd->handler == NULL) {
            break;
        }

        if (strncmp(cmd->name, p, len) == 0) {
            return cmd;
        }
    }

    return NULL;
}

static int
conf_get_int(void *conf, unsigned int offset, char *s, int len)
{
    char *p;
    int v;

    p = (char *) conf;

    v = my_atoi(s, len);

    *(int *) (p + offset) = v;

    return 0;
}

static int
conf_get_on_off(void *conf, unsigned int offset, char *s, int len)
{
    char *p;

    p = (char *) conf;

    if (strncasecmp(s, "off", 3) == 0) {
        *(int *) (p + offset) = 0;
        return 0;
    }

    if (strncasecmp(s, "on", 2) == 0) {
        *(int *) (p + offset) = 1;
        return 0;
    }

    return -1;
}

static int
conf_get_string(void *conf, unsigned int offset, char *s, int len)
{
    char *p, *v;


    p = (char *) conf;

    if (len == 0) {
        *(char **) (p + offset) = NULL;
        return 0;
    }

    if((v = calloc(1, sizeof(char) * (len + 1))) == NULL) {
        return -1;
    }
    memcpy(v, s, len);

    *(char **) (p + offset) = v;

    return 0;
}

static int
conf_get_iarray(void *conf, unsigned int offset, char *s, int len)
{
    char          *p, *c, *start;
    int            l, value;
    struct _list  *rcodes;

    if ((rcodes = alloc_list(NULL, NULL)) == NULL) {
        return -1;
    }

    for (p = s; p < s + len; ) {
        p = my_substring(p, s + len, &start, &l);

        if ((value = my_atoi(start, l)) == -1) {
            goto failure;
        }

        list_add2(rcodes, (void *) value);
    }

    c = (char *) conf;

    *(struct _list **) (c + offset) = rcodes;

    return 0;

 failure:
    free(rcodes);
    return -1;
}

static int
my_atoi(char *s, int n)
{
    int  value;

    if (n == 0) {
        return -1;
    }

    for (value = 0; n--; s++) {
        if (*s < '0' || *s > '9') {
            return -1;
        }

        value = value * 10 + (*s - '0');
    }

    return value < 0 ? -1 : value;
}

static char *
my_substring(char *pos, char *last, char **start, int *len)
{
    char *p;

    for (p = pos; p < last; p++) {
        if (*p == ' ' || *p == '\t' || *p == '\n') {
            continue;
        }
        break;
    }

    *start = p;

    for ( ; p < last; p++) {
        if (*p == ' ' || *p == '\t' || *p == '\n') {
            break;
        }
    }

    *len = p - *start;

    return p;
}


conf_t *
conf_create()
{
    conf_t *main_conf;

    if((main_conf = malloc(sizeof(conf_t))) == NULL) {
        return NULL;
    }

    main_conf->name_server = NULL;
    main_conf->port = 53;
    main_conf->timeout = 3000;
    main_conf->max_query = 1000;
    main_conf->concurrent_query = 100;
    main_conf->running_time = 1000000;
    main_conf->protocol = NULL;
    main_conf->addr_family = NULL;
    main_conf->verbose = 0;

    return main_conf;
}

int
conf_destroy(conf_t *cf)
{
    free(cf->name_server);
    free(cf->protocol);
    free(cf->addr_family);

    free(cf);

    return 0;
}
