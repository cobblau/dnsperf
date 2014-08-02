
#ifndef _DNS_PARAM_H_
#define _DNS_PARAM_H_


typedef enum {                  /* OP Code */
    DNS_OPCODE_UNKNOWN = -1,
    DNS_OPCODE_QUERY  = 0,
    DNS_OPCODE_IQUERY = 1,      /* inverse query. RFC3425 */
    DNS_OPCODE_STSTUS = 2,      /* DNS status query */
    DNS_OPCODE_NOTIFY = 4,
    DNS_OPCODE_UPDATE = 5
} dns_qcode_t;

typedef enum{                  /* Query Classes */
    DNS_CLASS_UNKNOWN = -1,    /* Unknown */
    DNS_CLASS_IN      = 1,     /* Internet */
    DNS_CLASS_CHAOS   = 3,     /* CHAOS (obsolete) */
    DNS_CLASS_HESIOD  = 4,     /* HESIOD (obsolete) */
    DNS_CLASS_NONE    = 254,   /* NONE (RFC 2136) */
    DNS_CLASS_ANY     = 255    /* ANY */
} dns_class_t;

typedef enum{                    /* Query Code */
    DNS_RCODE_UNKNOWN  = -1,
    DNS_RCODE_NOERROR  = 0,
    DNS_RCODE_FORMERR  = 1,
    DNS_RCODE_SERVFAIL = 2,
    DNS_RCODE_NXDOMAIN = 3,
    DNS_RCODE_NOTIMP   = 4,
    DNS_RCODE_REFUSED  = 5,
    /**
     * Rcode from RFC2136
     */
    DNS_RCODE_YXDOMAIN = 6,      /* name exists when it should not */
    DNS_RCODE_YXRRSET  = 7,      /* RR set exists when it should not*/
    DNS_RCODE_NXRRSET  = 8,      /* RR set that should exist does not */
    DNS_RCODE_NOTAUTH  = 9,      /* server not authoritive for zone */
    DNS_RCODE_NOTZONE  = 10,     /* name not contained in zone */

    DNS_RCODE_BADVERS  = 16,      /* bad opt version number. RFC2671 */
    DNS_RCODE_BADSIG   = 16,      /* TSIG signature failure. RFC2845 */
    DNS_RCODE_BADKEY   = 17,      /* key not recognized. RFC2845 */
    DNS_RCODE_BADTIME  = 18,      /* signature out-of-time window. RFC2845 */
    DNS_RCODE_BADMODE  = 19,      /* bad tkey mode. RFC2930 */
    DNS_RCODE_BADNAME  = 20,      /* duplicate key name. RFC2930 */
    DNS_RCODE_BADALG   = 21       /* algorithm not supported. RFC2930 */
} dns_rcode_t;


typedef enum {                    /* Query types */
    DNS_RR_UNKNOWN     = -1,      /* Unknown */

    DNS_RR_NONE        = 0,       /* None/invalid */
    DNS_RR_A           = 1,       /* Address */
    DNS_RR_NS          = 2,       /* Nameserver */
    DNS_RR_MD          = 3,       /* Mail dest */
    DNS_RR_MF          = 4,       /* Mail forwarder */
    DNS_RR_CNAME       = 5,       /* Canonical name */
    DNS_RR_SOA         = 6,       /* Start of authority */
    DNS_RR_MB          = 7,       /* Mailbox name */
    DNS_RR_MG          = 8,       /* Mail group */
    DNS_RR_MR          = 9,       /* Mail rename */
    DNS_RR_NULL        = 10,      /* Null */
    DNS_RR_WKS         = 11,      /* Well known service */
    DNS_RR_PTR         = 12,      /* IP -> fqdn mapping */
    DNS_RR_HINFO       = 13,      /* Host info */
    DNS_RR_MINFO       = 14,      /* Mailbox info */
    DNS_RR_MX          = 15,      /* Mail routing info */
    DNS_RR_TXT         = 16,      /* Text */
    DNS_RR_RP          = 17,      /* Responsible person */
    DNS_RR_AFSDB       = 18,      /* AFS cell database */
    DNS_RR_X25         = 19,      /* X_25 calling address */
    DNS_RR_ISDN        = 20,      /* ISDN calling address */
    DNS_RR_RT          = 21,      /* Router */
    DNS_RR_NSAP        = 22,      /* NSAP address */
    DNS_RR_NSAP_PTR    = 23,      /* Reverse NSAP lookup (depreciated) */
    DNS_RR_SIG         = 24,      /* Security signature */
    DNS_RR_KEY         = 25,      /* Security key */
    DNS_RR_PX          = 26,      /* X.400 mail mapping */
    DNS_RR_GPOS        = 27,      /* Geographical position (withdrawn) */
    DNS_RR_AAAA        = 28,      /* IPv6 Address */
    DNS_RR_LOC         = 29,      /* Location info */
    DNS_RR_NXT         = 30,      /* Next domain (security) */
    DNS_RR_EID         = 31,      /* Endpoint identifier */
    DNS_RR_NIMLOC      = 32,      /* Nimrod Locator */
    DNS_RR_SRV         = 33,      /* Server */
    DNS_RR_ATMA        = 34,      /* ATM Address */
    DNS_RR_NAPTR       = 35,      /* Naming Authority Pointer */
    DNS_RR_KX          = 36,      /* Key Exchange */
    DNS_RR_CERT        = 37,      /* Certification record */
    DNS_RR_A6          = 38,      /* IPv6 address (deprecates AAAA) */
    DNS_RR_DNAME       = 39,      /* Non-terminal DNAME (for IPv6) */
    DNS_RR_SINK        = 40,      /* Kitchen sink (experimentatl) */
    DNS_RR_OPT         = 41,      /* EDNS0 option (meta-RR) */
    DNS_RR_APL         = 42,      /* (RFC3123)*/
    DNS_RR_DS          = 43,      /* Delegation Signer */
    DNS_RR_SSHFP       = 44,      /* SSH Key Fingerprint */
    DNS_RR_IPSECKEY    = 45,      /* IPSECKEY */
    DNS_RR_RRIG        = 46,      /* RRSIG */
    DNS_RR_NSEC        = 47,      /* NSEC */
    DNS_RR_DNSKEY      = 48,      /* DNSKEY */
    DNS_RR_DHCID       = 49,      /* DNCID */
    DNS_RR_NSEC3       = 50,      /* NSEC3 */
    DNS_RR_NSEC3PARAM  = 51,      /* NSEC3PARAM */
    DNS_RR_TLSA        = 52,      /* TLSA */
    DNS_RR_HIP         = 55,      /* Host Identity Protocol */
    DNS_RR_TKEY        = 249,     /* Transaction Key */
    DNS_RR_TSIG        = 250,     /* Transaction signature */
    DNS_RR_IXFR        = 251,     /* Incremental zone transfer */
    DNS_RR_AXFR        = 252,     /* Zone transfer */
    DNS_RR_MAILB       = 253,     /* Transfer mailbox records */
    DNS_RR_MAILA       = 254,     /* Transfer mail agent records */
    DNS_RR_ANY         = 255,     /* A request for all records */
    DNS_RR_URI         = 256,
    DNS_RR_CAA         = 257,     /* Certification Authority Restriction */
    DNS_RR_TA          = 32768,   /* DNSSEC Trust Authorities */
    DNS_RR_DLV         = 32769    /* DNSSEC Lookaside Validation */
} dns_rrtype_t;

/* DNS EDNS0 Option Codes (OPT) */
typedef enum {
    DNS_OPTTYPE_LLQ           = 1,
    DNS_OPTTYPE_UL            = 2,
    DNS_OPTTYPE_NSID          = 3,
    DNS_OPTTYPE_DAU           = 4,
    DNS_OPTTYPE_DHU           = 5,
    DNS_OPTTYPE_N3U           = 6,
    DNS_OPTTYPE_CLIENT_SUBNET = 8
} dns_opttype_t;

#define DNS_MESSAGE_HEADER_LEN		12

/* DNS Header Flags */
#define DNS_HEADER_FLAG_QR		0x8000U     /* Query-Response bit */
#define DNS_HEADER_FLAG_AA		0x0400U     /* Authoritative Answer */
#define DNS_HEADER_FLAG_TC		0x0200U     /* Truncated Response */
#define DNS_HEADER_FLAG_RD		0x0100U     /* Recursion Desired */
#define DNS_HEADER_FLAG_RA		0x0080U     /* Recursion Allowed */
#define DNS_HEADER_FLAG_AD		0x0020U     /* Authentic Data. Used by DNSSEC */
#define DNS_HEADER_FLAG_CD		0x0010U     /* Checking Disabled. Used by DNSSEC */

/* EDNS Header Flags */
#define DNS_EDNS_MESSAGEFLAGS_DO 0x8000U    /* DNSSEC answer OK */

/* Section types */
#define DNS_SECTION_ANY         (-1)
#define DNS_SECTION_QUESTION     0
#define DNS_SECTION_ANSWER       1
#define DNS_SECTION_AUTHORITY    2
#define DNS_SECTION_ADDITIONAL   3
#define DNS_SECTION_MAX          4

#define DNS_PSEUDOSECTION_ANY   (-1)
#define DNS_PSEUDOSECTION_OPT    0
#define DNS_PSEUDOSECTION_TSIG   1
#define DNS_PSEUDOSECTION_SIG0   2
#define DNS_PSEUDOSECTION_MAX    3

#define DNS_NAME_COMPRESS_LABEL     0x00U
#define DNS_NAME_COMPRESS_POINTER   0xC0U

#define DNS_MAX_NAME_LEN        255
#define DNS_MAX_LABEL_LEN       63

#endif
