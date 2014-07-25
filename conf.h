#ifndef _CONF_H_
#define _CONF_H_


#define DEFAULT_CONF_FILE "./dnsperf.conf"

#define is_digit(c)   ((c) >= '0' && (c) <= '9')
#define is_letter(c)  (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define is_blank(c)   ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

typedef int (*conf_get_f) (void *conf, unsigned int offset, char *s, int len);

typedef struct {
    char        *name;
    conf_get_f   handler;
    unsigned int offset;
} conf_command_t;

typedef struct {
    char         *name_server;
    unsigned int  port;
    unsigned int  timeout;
    unsigned int  max_query;
    unsigned int  concurrent_query;
    unsigned int  running_time;
    char         *protocol;
    char         *addr_family;
    unsigned      verbose;
} conf_t;

conf_t *conf_create();
int conf_parse_file(char *conf_file_name, conf_t **conf);
int conf_destroy(conf_t *cf);

#endif
