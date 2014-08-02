#include <dns_param.h>
#include <list.h>


typedef struct dns_name_s  dns_name_t;
typedef LIST(dns_name_t)   dns_namelist_t;

struct dns_name_s {
    char            *name;
    dns_rrtype_t     type;
    dns_class_t      class;
    unsigned int     ttl;
    unsigned short   rdlength;
    char            *rdata;
    LINK(dns_name_t) link;
};


typedef struct {
    unsigned short id;
    unsigned short flags;
    dns_qcode_t    opcode;
    dns_rcode_t    rcode;
    unsigned short counts[DNS_SECTION_MAX];
    dns_namelist_t sections[DNS_SECTION_MAX];
} dns_message_t;


dns_message_t *dns_message_alloc();
int dns_message_free(dns_message_t *msg);

dns_name_t *dns_name_alloc();
int dns_name_free(dns_name_t *name);

int dns_message_rendered_len(dns_message_t *msg);
/*
 * Returns the buffer length which will be used to store packed msg.
 */

int dns_message_render(dns_message_t *msg, char *buf, int len);
/*
 * Pack `msg' into `buf'. `buf's length is specified by `len'.
 */

int dns_message_parse(dns_message_t *msg, char *buf, int len);
