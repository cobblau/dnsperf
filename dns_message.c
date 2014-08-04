#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dns_message.h>

static int dns_name_compress(char *str, char *name, int *offset);
/*
 * check to see of `name's trailing string contained in `str'.
 * Returns:
 *   -2: `name' can't be compressed.
 *   other: `name' can be compressed and it's compress start at `other'.
 */

dns_message_t *
dns_message_alloc()
{
    int             i;
    dns_message_t  *m;

    if ((m = (dns_message_t *) malloc(sizeof(dns_message_t))) == NULL) {
        return NULL;
    }

    m->id = 0;
    m->flags = 0;
    m->opcode = 0;
    m->rcode = 0;

    for (i = 0; i < DNS_SECTION_MAX; i++) {
        m->counts[i] = 0;
    }

    for (i = 0; i < DNS_SECTION_MAX; i++) {
        LIST_INIT(m->sections[i]);
    }

    return m;
}

int
dns_message_free(dns_message_t *msg)
{
    int         i;
    dns_name_t *name, *next;

    for (i = 0; i < DNS_SECTION_MAX; i++) {
        name = LIST_HEAD(msg->sections[i]);
        for ( ; name; name = next) {
            next = LIST_NEXT(name, link);
            dns_name_free(name);
        }
    }

    return 0;
}


dns_name_t *
dns_name_alloc()
{
    dns_name_t  *n;

    if ((n = (dns_name_t *) malloc(sizeof(dns_name_t))) == NULL) {
        return NULL;
    }

    n->name = NULL;
    n->type = 0;
    n->class = 0;
    n->ttl = 0;
    n->rdlength = 0;
    n->rdata = 0;
    LINK_INIT(n, link);

    return n;
}


int
dns_name_free(dns_name_t *n)
{
    free(n->name);
    free(n->rdata);

    free(n);

    return 0;
}

int
dns_message_rendered_len(dns_message_t *m)
{
    int         i, j, size, offset, clen;
    int         compress_len;
    char        compress_table[64][DNS_MAX_NAME_LEN];
    dns_name_t *n;


    compress_len = 0;

    /* Header length */
    size = DNS_MESSAGE_HEADER_LEN;

    /* render question section */
    if (m->counts[DNS_SECTION_QUESTION] > 0) {

        /* only pack the first question */
        n = LIST_HEAD(m->sections[i]);

        memcpy(compress_table[compress_len++], n, strlen(n->name));
        size = size + strlen(n->name) + 2;

        size = size + 2 + 2;  /* qtype(2) + class(2) */
    }

    /* render other sections */
    for (i = DNS_SECTION_ANSWER; i < DNS_SECTION_MAX; i++) {

        if (m->counts[i] == 0) {
            continue;
        }

        for (n = LIST_HEAD(m->sections[i]); n != NULL; n = LIST_NEXT(n, link)) {

            for (j = 0; j < compress_len; j++) {

                clen = dns_name_compress(compress_table[j], n->name, &offset);
                if (clen == -2) {
                    /* if name can't be compressed */
                    memcpy(compress_table[compress_len++], n, strlen(n->name));
                    size = size + strlen(n->name) + 2;
                    break;
                } else {
                    /* name can be compressed, `clen' is the start position of the
                       compressed part */
                    size = size + clen + 1 + 2;
                }
            }

            size = size + 2 + 2 + 4 + 2;

            /* do not pack a and aaaa */
            if (n->type == DNS_RR_A || n->type == DNS_RR_AAAA) {
                size = size + n->rdlength;
                break;
            }

            for (j = 0; j < compress_len; j++) {

                clen = dns_name_compress(compress_table[j], n->name, &offset);
                if (clen == -2) {
                    /* if name can't be compressed */
                    memcpy(compress_table[compress_len++], n, strlen(n->name));
                    size = size + strlen(n->name) + 2;
                    break;
                } else {
                    /* name can be compressed, `clen' is the start position of the
                       compressed part */
                    size = size + clen + 1 + 2;
                }
            }

        }
    }

    return size;
}

int
dns_message_render(dns_message_t *msg, char *buf, int len)
{

}


int
dns_message_parse(dns_message_t *msg, char *buf, int len)
{

}
