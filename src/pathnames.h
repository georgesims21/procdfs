#ifndef PROCSYS_PATHNAMES_H
#define PROCSYS_PATHNAMES_H

#define PATHARRLEN 21

// As defined in: https://www.kernel.org/doc/Documentation/filesystems/proc.txt
/* Table 1-8: IPv6 info in /proc/net
 * ..............................................................................
 */
#define _PATH_PROCNET_UDP6		"/net/udp6"
#define _PATH_PROCNET_TCP6		"/net/tcp6"
#define _PATH_PROCNET_RAW6		"/net/raw6"
#define _PATH_PROCNET_IGMP6		"/net/igmp6"
//#define _PATH_PROCNET_IFINET6		"/net/if_inet6"
#define _PATH_PROCNET_ROUTE6		"/net/ipv6_route"
#define _PATH_PROCNET_RT6_STATS		"/net/rt6_stats"
#define _PATH_PROCNET_SOCKSTAT6		"/net/sockstat6"
#define _PATH_PROCNET_SNMP6		"/net/snmp6"

/* Table 1-9: Network info in /proc/net
 * ..............................................................................
 */
#define _PATH_PROCNET_ARP		"/net/arp"
#define _PATH_PROCNET_DEV		"/net/dev"
#define _PATH_PROCNET_DEV_MCAST		"/net/dev_mcast"
//#define _PATH_PROCNET_DEV_STAT		"/net/dev_stat"
//#define _PATH_PROCNET_IP_FWCHAINS "/net/ip_fwchains"
//#define _PATH_PROCNET_IP_FWNAMES "/net/ip_fwnames"
//#define _PATH_PROCNET_IP_MASQ "/net/ip_masq"
//#define _PATH_PROCNET_IP_MASQUERADE "/net/ip_masquerade"
#define _PATH_PROCNET_NETSTAT "/net/netstat"
#define _PATH_PROCNET_RAW		"/net/raw"
#define _PATH_PROCNET_ROUTE		"/net/route"
//#define _PATH_PROCNET_RPC		"/net/rpc"
#define _PATH_PROCNET_RTCACHE		"/net/rt_cache"
#define _PATH_PROCNET_SNMP		"/net/snmp"
#define _PATH_PROCNET_SOCKSTAT		"/net/sockstat"
//#define _PATH_PROCNET_TCP		"/net/tcp"
#define _PATH_PROCNET_UDP		"/net/udp"
//#define _PATH_PROCNET_UNIX		"/net/unix"
#define _PATH_PROCNET_WIRELESS		"/net/wireless"
#define _PATH_PROCNET_IGMP		"/net/igmp"
#define _PATH_PROCNET_PSCHED		"/net/psched"
//#define _PATH_PROCNET_NETLINK		"/net/netlink"
//#define _PATH_PROCNET_IP_MR_VIFS		"/net/ip_mr_vifs"
//#define _PATH_PROCNET_IP_MR_CACHE		"/net/ip_mr_cache"

const char filenames[PATHARRLEN][MAXPATH] = {
    "udp6",
    "tcp6",
    "raw6",
    "igmp6",
//    "if_inet6",
    "ipv6_route",
    "rt6_stats",
    "sockstat6",
    "snmp6",
    "arp",
    "dev",
    "dev_mcast",
//    "dev_stat",
//    "ip_fwchains",
//    "ip_fwnames",
//    "ip_masq",
//    "ip_masquerade",
    "netstat",
    "raw",
    "route",
//    "rpc",
    "rt_cache",
    "snmp",
    "sockstat",
//    "tcp",
    "udp",
//    "unix",
    "wireless",
    "igmp",
    "psched",
//    "netlink",
//    "ip_mr_vifs",
//    "ip_mr_cache",
};

const char paths[PATHARRLEN][MAXPATH] = {
    _PATH_PROCNET_UDP6,
    _PATH_PROCNET_TCP6,
    _PATH_PROCNET_RAW6,
    _PATH_PROCNET_IGMP6,
//    _PATH_PROCNET_IFINET6,
    _PATH_PROCNET_ROUTE6,
    _PATH_PROCNET_RT6_STATS,
    _PATH_PROCNET_SOCKSTAT6,
    _PATH_PROCNET_SNMP6,
    _PATH_PROCNET_ARP,
    _PATH_PROCNET_DEV,
    _PATH_PROCNET_DEV_MCAST,
//    _PATH_PROCNET_DEV_STAT,
//    _PATH_PROCNET_IP_FWCHAINS,
//    _PATH_PROCNET_IP_FWNAMES,
//    _PATH_PROCNET_IP_MASQ,
//    _PATH_PROCNET_IP_MASQUERADE,
    _PATH_PROCNET_NETSTAT,
    _PATH_PROCNET_RAW,
    _PATH_PROCNET_ROUTE,
//    _PATH_PROCNET_RPC,
//    _PATH_PROCNET_RPC,
    _PATH_PROCNET_RTCACHE,
    _PATH_PROCNET_SNMP,
    _PATH_PROCNET_SOCKSTAT,
//    _PATH_PROCNET_TCP,
    _PATH_PROCNET_UDP,
//    _PATH_PROCNET_UNIX,
    _PATH_PROCNET_WIRELESS,
    _PATH_PROCNET_IGMP,
    _PATH_PROCNET_PSCHED,
//    _PATH_PROCNET_NETLINK
//    _PATH_PROCNET_IP_MR_VIFS,
//    _PATH_PROCNET_IP_MR_CACHE
};

#endif //PROCSYS_PATHNAMES_H
