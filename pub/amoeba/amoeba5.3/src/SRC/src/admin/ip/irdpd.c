/*	@(#)irdpd.c	1.1	96/02/27 10:14:28 */
/*	irdpd 1.5 - Internet router discovery protocol daemon.
 *							Author: Kees J. Bot
 *								28 May 1994
 * Activily solicit or passively look for routers.
 * Based heavily on its forerunners, the irdp_sol and rip2icmp daemons by
 * Philip Homburg.
 */
#define nil 0
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <sys/ioctl.h>
#ifndef AMOEBA
#include <sys/asynchio.h>
#include <net/hton.h>
#include <net/netlib.h>
#include <net/gen/in.h>
#include <net/gen/ip_hdr.h>
#include <net/gen/icmp.h>
#include <net/gen/icmp_hdr.h>
#include <net/gen/ip_io.h>
#include <net/gen/inet.h>
#include <net/gen/netdb.h>
#include <net/gen/oneCsum.h>
#include <net/gen/socket.h>
#include <net/gen/udp.h>
#include <net/gen/udp_hdr.h>
#include <net/gen/udp_io.h>
#else
#include <amoeba.h>
#undef   timeout	/* avoid name clash */
#include <ampolicy.h>
#include <cmdreg.h>
#include <stderr.h>
#include <thread.h>
#include <module/host.h>
#include <module/mutex.h>
#include <server/ip/hton.h>
#include <server/ip/types.h>
#include <server/ip/gen/in.h>
#include <server/ip/gen/ip_hdr.h>
#include <server/ip/gen/icmp.h>
#include <server/ip/gen/icmp_hdr.h>
#include <server/ip/gen/ip_io.h>
#include <server/ip/gen/inet.h>
#include <server/ip/gen/netdb.h>
#include <server/ip/gen/oneCsum.h>
#include <server/ip/gen/socket.h>
#include <server/ip/gen/udp.h>
#include <server/ip/gen/udp_hdr.h>
#include <server/ip/gen/udp_io.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#define RIP_STACK		16000
#define IRDP_STACK		16000
#endif

#define MAX_RETRIES		   10	/* # router solicitates. */
#define TIMEOUT			    4	/* Secs between solicitate retries. */
#define DEST_TO 	      (10*60)	/* 10 minutes */
#define NEW_ROUTE	       (5*60)	/* 5 minutes */
#define DANGER		       (1*60)	/* Nearing a advert timeout? */
#define DEAD_TO		     (24L*60*60) /* 24 hours */
#define DEAD_PREF	     0x80000000L /* from RFC 1236 */
#define MaxAdvertisementInterval  450	/* 7.5 minutes */
#define LifeTime		(4 * MaxAdvertisementInterval)
#define PRIO_OFF_DEF		-1024
#define RIP_VERSION		    1
#define RIP_REPLY		    2

/* A don't time out timeout. */
#define NEVER		((time_t) ((time_t) -1 < 0 ? LONG_MAX : ULONG_MAX))

#if !DEBUG
/* Unless doing serious debugging. */
#define gethostbyaddr(addr, len, type)	((struct hostent *) nil)
#endif

#if !__minix_vmd
/* Standard Minix needs to choose between router discovery and RIP info. */
int do_rdisc= 1;
int do_rip= 0;
#else
/* VMD Minix can do both at once. */
#define do_rdisc 1
#define do_rip 1
#endif

#ifndef AMOEBA
int rip_fd;			/* For incoming RIP packet. */
int irdp_fd;			/* Receive or transmit IRDP packets. */
#define lock_table()		/* single threaded, so not needed */
#define unlock_table()
#else
capability udp_cap, ip_cap;	/* The IP server itself */
capability rip_chan_cap;	/* For incoming RIP packet. */
capability irdp_chan_cap;	/* Receive or transmit IRDP packets. */
static mutex table_lock;
#define lock_table()		mu_lock(&table_lock)
#define unlock_table()		mu_unlock(&table_lock)
#endif

char *udp_device;		/* UDP device to use. */
char *ip_device;		/* IP device to use. */

int priority_offset;		/* Offset to make my routes less preferred. */

int bcast= 0;			/* Broadcast adverts to all. */
int silent= 0;			/* Don't react to solicitates. */
int debug= 0;

char rip_buf[8192];		/* Incoming RIP packet buffer. */
char irdp_buf[1024];		/* IRDP buffer. */

char *prog_name = "IRDPD";

typedef struct routeinfo
{
	u8_t		command;
	u8_t		version;
	u16_t		zero1;
	struct routedata {
		u16_t	family;
		u16_t	zero2;
		u32_t	ip_addr;
		u32_t	zero3, zero4;
		u32_t	metric;
	} data[1];
} routeinfo_t;

typedef struct table
{
	ipaddr_t	tab_gw;
	i32_t		tab_pref;
	time_t		tab_time;
} table_t;

table_t *table;			/* Collected table of routing info. */
size_t table_size;

ipaddr_t my_ip_addr;		/* IP address of the local host. */

time_t now, timeout, next_sol, next_advert, router_advert_valid;
int sol_retries= MAX_RETRIES;

#ifdef __STDC__
void report(const char *label)
#else
void report(label) char *label;
#endif
/* irdpd: /dev/hd0: Device went up in flames */
{
	fprintf(stderr, "irdpd: %s: %s\n", label, strerror(errno));
}

#ifdef __STDC__
void fatal(const char *label)
#else
void fatal(label) char *label;
#endif
/* irdpd: /dev/house: Taking this with it */
{
	report(label);
	exit(1);
}

#ifdef __STDC__
char *addr2name(ipaddr_t host)
#else
char *addr2name(host) ipaddr_t host;
#endif
/* Translate an IP address to a printable name. */
{
#if DEBUG
	struct hostent *hostent;

	hostent= gethostbyaddr((char *) &host, sizeof(host), AF_INET);
	return hostent == nil ? inet_ntoa(host) : hostent->h_name;
#else
	return inet_ntoa(host);
#endif
}

#ifdef AMOEBA
static mutex print_lock;
#define printf lock_printf

/*VARARGS1*/
#ifdef __STDC__
int lock_printf(const char *format, ...)
#else
int lock_printf(format, va_alist) char *format; va_dcl
#endif
{
        va_list ap;
        int retval;
 
	mu_lock(&print_lock);
#ifdef __STDC__
        va_start(ap, format);
#else
        va_start(ap);
#endif
        retval = vfprintf(stdout, format, ap);
 
        va_end(ap);
	mu_unlock(&print_lock);
        return retval;
}
#endif

#ifdef __STDC__
void print_table(void)
#else
void print_table()
#endif
/* Show the collected routing table. */
{
	int i;
	table_t *ptab;
	struct tm *tm;

	lock_table();
	for (i= 0, ptab= table; i < table_size; i++, ptab++) {
		if (ptab->tab_time < now - DEAD_TO) continue;

		tm= localtime(&ptab->tab_time);
		printf("%-40s %6ld %02d:%02d:%02d\n",
			addr2name(ptab->tab_gw),
			(long) ptab->tab_pref,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
	unlock_table();
}

#ifdef AMOEBA

int
tcp_write(cap, buf, size)
capability *cap;
char *buf;
int size;
{
    int nbytes;

    nbytes = tcpip_write(cap, buf, size);
    if (ERR_STATUS(nbytes)) {
	fprintf(stderr, "%s: tcpip_write failed: %s\n",
		prog_name, err_why(ERR_CONVERT(nbytes)));
	return -1;
    }
    if (nbytes != size) {
	fprintf(stderr, "%s: tcpip_write only wrote %d of %d\n",
		prog_name, nbytes, size);
    }
    return nbytes;
}

#endif


#ifdef __STDC__
void advertize(ipaddr_t host)
#else
void advertize(host) ipaddr_t host;
#endif
/* Send a router advert to a host. */
{
	char *buf, *data;
	ip_hdr_t *ip_hdr;
	icmp_hdr_t *icmp_hdr;
	int i;
	table_t *ptab;

	buf= malloc(sizeof(*ip_hdr) + offsetof(icmp_hdr_t, ih_dun.uhd_data)
			+ table_size * (sizeof(ipaddr_t) + sizeof(u32_t)));
	if (buf == nil) fatal("heap error");

	ip_hdr= (ip_hdr_t *) buf;
	icmp_hdr= (icmp_hdr_t *) (ip_hdr + 1);

	ip_hdr->ih_vers_ihl= 0x45;
	ip_hdr->ih_dst= host;

	icmp_hdr->ih_type= ICMP_TYPE_ROUTER_ADVER;
	icmp_hdr->ih_code= 0;
	icmp_hdr->ih_hun.ihh_ram.iram_na= 0;
	icmp_hdr->ih_hun.ihh_ram.iram_aes= 2;
	icmp_hdr->ih_hun.ihh_ram.iram_lt= htons(LifeTime);
	data= (char *) icmp_hdr->ih_dun.uhd_data;

	/* Collect gateway entries from the table. */
	lock_table();
	for (i= 0, ptab= table; i < table_size; i++, ptab++) {
		if (ptab->tab_time < now - DEAD_TO) continue;

		icmp_hdr->ih_hun.ihh_ram.iram_na++;
		if (ptab->tab_time < now - DEST_TO) ptab->tab_pref= DEAD_PREF;
		* (ipaddr_t *) data= ptab->tab_gw;
		data+= sizeof(ipaddr_t);
		* (i32_t *) data= htonl(ptab->tab_pref);
		data+= sizeof(i32_t);
	}
	unlock_table();
	icmp_hdr->ih_chksum= 0;
	icmp_hdr->ih_chksum= ~oneC_sum(0, (u16_t *) icmp_hdr,
					data - (char *) icmp_hdr);

	if (icmp_hdr->ih_hun.ihh_ram.iram_na > 0) {
		/* Send routing info. */

		if (debug) {
			printf("Routing table send to %s:\n", addr2name(host));
			print_table();
		}

#ifndef AMOEBA
		if (write(irdp_fd, buf, data - buf) < 0) fatal(ip_device);
#else
		if (tcp_write(&irdp_chan_cap, buf, data - buf) != data - buf)
			fatal("tcpip_write to irdp channel failed");
#endif
	}
	free(buf);
}

#ifdef __STDC__
void time_functions(void)
#else
void time_functions()
#endif
/* Perform time dependend functions: router solicitate, router advert. */
{
#if !__minix_vmd && !defined(AMOEBA)
	if (sol_retries == 0) {
		/* No more solicitates, switch to RIP. */
		do_rdisc= 0;
		do_rip= 1;
	}
#endif
	if (sol_retries > 0 && now >= next_sol) {
		/* Broadcast a router solicitate to find a router. */
		char buf[sizeof(ip_hdr_t) + 8];
		ip_hdr_t *ip_hdr;
		icmp_hdr_t *icmp_hdr;

		ip_hdr= (ip_hdr_t *) buf;
		icmp_hdr= (icmp_hdr_t *) (ip_hdr + 1);

		ip_hdr->ih_vers_ihl= 0x45;
		ip_hdr->ih_dst= HTONL(0xFFFFFFFFL);

		icmp_hdr->ih_type= ICMP_TYPE_ROUTE_SOL;
		icmp_hdr->ih_code= 0;
		icmp_hdr->ih_chksum= 0;
		icmp_hdr->ih_hun.ihh_unused= 0;
		icmp_hdr->ih_chksum= ~oneC_sum(0, (u16_t *)icmp_hdr, 8);

		if (debug) printf("Broadcasting router solicitate\n");

#ifndef AMOEBA
		if (write(irdp_fd, buf, sizeof(buf)) < 0)
			fatal("sending router solicitate failed");
#else
		if (tcp_write(&irdp_chan_cap, buf, sizeof(buf)) != sizeof(buf))
			fatal("tcpip_write to irdp channel failed");
#endif

		/* Schedule the next packet. */
		next_sol= now + TIMEOUT;

		sol_retries--;
	}

	if (table_size > 0 && now >= next_advert) {
		/* Advertize routes to the local host (normally), or
		 * broadcast them (to keep bad hosts up.)
		 */

		advertize(bcast ? HTONL(0xFFFFFFFFL) : HTONL(0x7F000001L));
		next_advert= now + MaxAdvertisementInterval;
	}

	/* Schedule the next wakeup call. */
	timeout= NEVER;
	if (sol_retries > 0 && next_sol < timeout) {
		timeout= next_sol;
	}
	if (table_size > 0 && next_advert < timeout) {
		timeout= next_advert;
	}
}

#ifdef __STDC__
void add_gateway(ipaddr_t host, i32_t pref)
#else
void add_gateway(host, pref)
ipaddr_t host;
i32_t    pref;
#endif
/* Add a router with given address and preference to the routing table. */
{
	table_t *oldest, *ptab;
	int i;

	lock_table();

	/* Look for the host, or select the oldest entry. */
	oldest= nil;
	for (i= 0, ptab= table; i < table_size; i++, ptab++) {
		if (ptab->tab_gw == host) break;

		if (oldest == nil || ptab->tab_time < oldest->tab_time)
			oldest= ptab;
	}

	/* Don't evict the oldest if it is still valid. */
	if (oldest != nil && oldest->tab_time >= now - DEST_TO) oldest= nil;

	/* Expand the table? */
	if (i == table_size && oldest == nil) {
		table_size++;
		table= realloc(table, table_size * sizeof(*table));
		if (table == nil) fatal("heap error");
		oldest= &table[table_size - 1];
	}

	if (oldest != nil) {
		ptab= oldest;
		ptab->tab_gw= host;
		ptab->tab_pref= DEAD_PREF;
	}

	/* Replace an entry if the new one looks more promising. */
	if (pref >= ptab->tab_pref || ptab->tab_time <= now - NEW_ROUTE) {
		ptab->tab_pref= pref;
		ptab->tab_time= now;
	}

	unlock_table();
}

#ifdef __STDC__
void rip_incoming(ssize_t n)
#else
void rip_incoming(n) ssize_t n;
#endif
/* Use a RIP packet to add to the router table.  (RIP packets are really for
 * between routers, but often it is the only information around.)
 */
{
	udp_io_hdr_t *udp_io_hdr;
	u32_t default_dist;
	i32_t pref;
	routeinfo_t *routeinfo;
	struct routedata *data, *end;

	/* We don't care about RIP packets when there are router adverts. */
	if (now + MaxAdvertisementInterval < router_advert_valid) return;

	udp_io_hdr= (udp_io_hdr_t *) rip_buf;
	if (udp_io_hdr->uih_data_len != n - sizeof(*udp_io_hdr)) {
		if (debug) printf("Bad sized route packet (discarded)\n");
		return;
	}
	routeinfo= (routeinfo_t *) (rip_buf + sizeof(*udp_io_hdr)
			+ udp_io_hdr->uih_ip_opt_len);

	if (routeinfo->version != RIP_VERSION
				|| routeinfo->command != RIP_REPLY) {
		if (debug) {
			printf("Route packet command %d, version %d ignored\n",
				routeinfo->command, routeinfo->version);
		}
		return;
	}

	/* Look for a default route, the route to the gateway. */
	end= (struct routedata *) (rip_buf + n);
	default_dist= (u32_t) -1;
	for (data= routeinfo->data; data < end; data++) {
		if (ntohs(data->family) != AF_INET || data->ip_addr != 0)
			continue;
		default_dist= ntohl(data->metric);
		if (default_dist >= 256) {
			if (debug) printf("Strange metric %d\n", default_dist);
		}
	}
	pref= default_dist >= 256 ? 1 : 512 - default_dist;
	pref+= priority_offset;

	/* Add the gateway to the table with the calculated preference. */
	add_gateway(udp_io_hdr->uih_src_addr, pref);

	if (debug) {
		printf("Routing table after RIP packet from %s:\n",
			addr2name(udp_io_hdr->uih_src_addr));
		print_table();
	}
}

#ifdef __STDC__
void irdp_incoming(ssize_t n)
#else
void irdp_incoming(n) ssize_t n;
#endif
/* Look for router solicitates and router advertisements.  The solicitates
 * are probably from other irdpd daemons, we answer them if we do not expect
 * a real router to answer.  The advertisements cause this daemon to shut up.
 */
{
	ip_hdr_t *ip_hdr;
	icmp_hdr_t *icmp_hdr;
	int ip_hdr_len;
	char *data;
	int i;
	int router;
	ipaddr_t addr;
	i32_t pref;
	time_t valid;

	ip_hdr= (ip_hdr_t *) irdp_buf;
	ip_hdr_len= (ip_hdr->ih_vers_ihl & IH_IHL_MASK) << 2;
	if (n < ip_hdr_len + 8) {
		if (debug) printf("Bad sized ICMP (discarded)\n");
		return;
	}

	icmp_hdr= (icmp_hdr_t *)(irdp_buf + ip_hdr_len);

	/* Did I send it myself? */
	if (ip_hdr->ih_src == my_ip_addr) return;
	if ((htonl(ip_hdr->ih_src) & 0xFF000000L) == 0x7F000000L) return;

#if __minix_vmd
	if (icmp_hdr->ih_type == ICMP_TYPE_ROUTE_SOL) {
		/* Some other host is looking for a router.  Send a table
		 * if there is no smart router around, we are not still
		 * solicitating ourselves, and we are not told to be silent.
		 */
		if (sol_retries == 0 && now > router_advert_valid && !silent)
			advertize(ip_hdr->ih_src);
		return;
	}
#endif
	if (icmp_hdr->ih_type != ICMP_TYPE_ROUTER_ADVER) return;

	/* Incoming router advertisement, the kind of packet the TCP/IP task
	 * is very happy with.  No need to solicitate further.
	 */
	sol_retries= 0;

	/* Add router info to our table.  Also see if the packet came from
	 * a router, and not another irdpd.  If it is from a router then we
	 * go silent for the lifetime of the ICMP.
	 */
	router= 0;
	data= (char *) icmp_hdr->ih_dun.uhd_data;
	for (i= 0; i < icmp_hdr->ih_hun.ihh_ram.iram_na; i++) {
		addr= * (ipaddr_t *) data;
		data+= sizeof(ipaddr_t);
		pref= htonl(* (i32_t *) data);
		data+= sizeof(i32_t);

		if (addr == ip_hdr->ih_src) {
			/* The sender is in the routing table! */
			router= 1;
#if !__minix_vmd && !defined(AMOEBA)
			/* Under standard Minix we assume that routers live
			 * forever.
			 */
			if (debug) {
				printf(
				"Advertizing router %s available, so exit!\n",
					addr2name(ip_hdr->ih_src));
			}
			exit(0);
#endif
		}
		add_gateway(addr, pref);
	}

	valid= now + ntohs(icmp_hdr->ih_hun.ihh_ram.iram_lt);
	if (router) router_advert_valid= valid;

	/* Restart advertizing close to the timeout of the advert.  (No more
	 * irdpd adverts if the router stays alive.)
	 */
	if (router || next_advert > valid - DANGER)
		next_advert= valid - DANGER;

	if (debug) {
		printf("Routing table after advert received from %s:\n",
			addr2name(ip_hdr->ih_src));
		print_table();
		if (router) {
			struct tm *tm= localtime(&router_advert_valid);
			printf(
			"This router advert is valid until %02d:%02d:%02d\n",
				tm->tm_hour, tm->tm_min, tm->tm_sec);
		}
	}
}

#ifdef AMOEBA

/*ARGSUSED*/
void
rip_thread(param, psize)
char *param;
int psize;
{
	errstat nbytes;
	udp_io_hdr_t *udp_io_hdr;
	int actlen;
	u32_t default_dist;
	i32_t pref;
	routeinfo_t *routeinfo;
	struct routedata *data, *end;

	for (;;)
	{
		nbytes= udp_read_msg (&rip_chan_cap, (char *)rip_buf, 
				      sizeof(rip_buf), &actlen, 0);
		if (ERR_STATUS(nbytes))	{
			fprintf(stderr, "%s: error reading RIP channel (%s)\n",
				prog_name, err_why(ERR_CONVERT(nbytes)));
			exit(1);
		}
		if (actlen != nbytes)
		{
			printf("%s: RIP packet too large (len= %d, got %d)\n", 
				prog_name, actlen, nbytes);
			continue;
		}
		now= time(nil);
		rip_incoming(actlen);
	}
}

/*ARGSUSED*/
void
irdp_thread(param, psize)
char *param;
int psize;
{
	errstat nbytes;
	udp_io_hdr_t *udp_io_hdr;
	int actlen;
	u32_t default_dist;
	i32_t pref;
	routeinfo_t *routeinfo;
	struct routedata *data, *end;

	for (;;)
	{
		nbytes= tcpip_read (&irdp_chan_cap, (char *)irdp_buf, 
				    sizeof(irdp_buf));
		if (ERR_STATUS(nbytes))	{
			fprintf(stderr, "%s: error reading RIP channel (%s)\n",
				prog_name, err_why(ERR_CONVERT(nbytes)));
			exit(1);
		}
		now= time(nil);
		irdp_incoming(nbytes);
	}
}

#endif /* AMOEBA */

#ifdef __STDC__
void usage(void)
#else
void usage()
#endif
{
	fprintf(stderr,
"Usage: irdpd [-bds] [-U udp-device] [-I ip-device] [-o priority-offset]\n");
	exit(1);
}

#ifdef __STDC__
int main(int argc, char **argv)
#else
int main(argc, argv)
int    argc;
char **argv;
#endif
{
	int i;
	struct servent *service;
	udpport_t route_port;
	nwio_udpopt_t udpopt;
	nwio_ipconf_t ipconf;
	nwio_ipopt_t ipopt;
#ifndef AMOEBA
	asynchio_t asyn;
	struct timeval tv;
#else
	errstat error;
#endif
	char *offset_arg, *offset_end;
	long arg;

#ifdef AMOEBA
	mu_init(&print_lock);
	mu_init(&table_lock);
#endif
	prog_name = argv[0];
	udp_device = ip_device = offset_arg = nil;

	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		char *p= argv[i] + 1;

		if (p[0] == '-' && p[1] == 0) { i++; break; }

		while (*p != 0) {
			switch (*p++) {
			case 'U':
				if (udp_device != nil) usage();
				if (*p == 0) {
					if (++i == argc) usage();
					p= argv[i];
				}
				udp_device= p;
				p= "";
				break;
			case 'I':
				if (ip_device != nil) usage();
				if (*p == 0) {
					if (++i == argc) usage();
					p= argv[i];
				}
				ip_device= p;
				p= "";
				break;
			case 'o':
				if (offset_arg != nil) usage();
				if (*p == 0) {
					if (++i == argc) usage();
					p= argv[i];
				}
				offset_arg= p;
				p= "";
				break;
			case 'b':
				bcast= 1;
				break;
			case 's':
				silent= 1;
				break;
			case 'd':
				debug= 1;
				break;
			default:
				usage();
			}
		}
	}
	if (i != argc) usage();

#ifndef AMOEBA
	if (udp_device == nil && (udp_device= getenv("UDP_DEVICE")) == nil)
		udp_device= UDP_DEVICE;

	if (ip_device == nil && (ip_device= getenv("IP_DEVICE")) == nil)
		ip_device= IP_DEVICE;
#else
	if (udp_device == nil && (udp_device= getenv("UDP_SERVER")) == nil)
		udp_device= UDP_SVR_NAME;

	if (ip_device == nil && (ip_device= getenv("IP_SERVER")) == nil)
		ip_device= IP_SVR_NAME;
#endif

	if (offset_arg == nil) {
		priority_offset= PRIO_OFF_DEF;
	} else {
		arg= strtol(offset_arg, &offset_end, 0);
		if (*offset_end != 0 || (priority_offset= arg) != arg) usage();
	}

	if ((service= getservbyname("route", "udp")) == nil) {
		fprintf(stderr,
	"irdpd: unable to look up the port number for the 'route' service\n");
		exit(1);
	}

	route_port= (udpport_t) service->s_port;

#ifndef AMOEBA
	if ((rip_fd= open(udp_device, O_RDWR)) < 0) fatal(udp_device);
#else
	error= ip_host_lookup(udp_device, "udp", &udp_cap);
        if (error != STD_OK) {
		fprintf(stderr, "%s: cannot lookup UDP cap %s (%s)\n",
			prog_name, udp_device, err_why(ERR_CONVERT(error)));
		exit(1);
        }

        error= tcpip_open(&udp_cap, &rip_chan_cap);
        if (error != STD_OK) {
                fprintf(stderr, "%s: cannot open UDP channel at %s (%s)\n",
                        prog_name, udp_device, err_why(ERR_CONVERT(error)));
                exit(1);
        }
 
        error= tcpip_keepalive_cap(&rip_chan_cap);
        if (error != STD_OK) {
                fprintf(stderr, "%s: cannot keep UDP channel alive (%s)\n",
                        prog_name, err_why(ERR_CONVERT(error)));
                exit(1);
        }
#endif

	udpopt.nwuo_flags= NWUO_COPY | NWUO_LP_SET | NWUO_DI_LOC
		| NWUO_EN_BROAD | NWUO_RP_SET | NWUO_RA_ANY | NWUO_RWDATALL
		| NWUO_DI_IPOPT;
	udpopt.nwuo_locport= route_port;
	udpopt.nwuo_remport= route_port;
#ifndef AMOEBA
	if (ioctl(rip_fd, NWIOSUDPOPT, &udpopt) < 0)
		fatal("setting UDP options failed");
#else
	error= udp_ioc_setopt(&rip_chan_cap, &udpopt);
        if (error != STD_OK) {
                fprintf(stderr, "%s: unable to set UDP options (%s)\n",
                        prog_name, err_why(ERR_CONVERT(error)));
                exit(1);
        }
#endif

#ifndef AMOEBA
	if ((irdp_fd= open(ip_device, O_RDWR)) < 0) fatal(ip_device);

	if (ioctl(irdp_fd, NWIOGIPCONF, &ipconf) < 0)
		fatal("can't get IP configuration");
#else
	error= ip_host_lookup(ip_device, "ip", &ip_cap);
        if (error != STD_OK) {
		fprintf(stderr, "%s: cannot lookup IP cap %s (%s)\n",
			prog_name, ip_device, err_why(ERR_CONVERT(error)));
		exit(1);
        }

        error= tcpip_open(&ip_cap, &irdp_chan_cap);
        if (error != STD_OK)
        {
                fprintf(stderr, "%s: cannot open IP channel at %s (%s)\n",
                        prog_name, ip_device, err_why(ERR_CONVERT(error)));
                exit(1);
        }
 
        error= tcpip_keepalive_cap(&irdp_chan_cap);
        if (error != STD_OK)
        {
                fprintf(stderr, "%s: tcpip_keepalive_cap failed: %s\n",
                        prog_name, err_why(ERR_CONVERT(error)));
                exit(1);
        }
 
        error= ip_ioc_getconf(&irdp_chan_cap, &ipconf);
        if (error != STD_OK)
        {
                fprintf(stderr, "%s: unable to ip_ioc_getconf: %s\n",
                        prog_name, err_why(ERR_CONVERT(error)));
                exit(1);
        }
#endif

	my_ip_addr= ipconf.nwic_ipaddr;

	ipopt.nwio_flags= NWIO_COPY | NWIO_EN_LOC | NWIO_EN_BROAD
			| NWIO_REMANY | NWIO_PROTOSPEC
			| NWIO_HDR_O_SPEC | NWIO_RWDATALL;
	ipopt.nwio_tos= 0;
	ipopt.nwio_ttl= 255;
	ipopt.nwio_df= 0;
	ipopt.nwio_hdropt.iho_opt_siz= 0;
	ipopt.nwio_rem= HTONL(0xFFFFFFFFL);
	ipopt.nwio_proto= IPPROTO_ICMP;

#ifndef AMOEBA
	if (ioctl(irdp_fd, NWIOSIPOPT, &ipopt) < 0)
		fatal("can't configure ICMP channel");
#else
        error= ip_ioc_setopt(&irdp_chan_cap, &ipopt);
        if (error != STD_OK) {
                fprintf(stderr, "%s: cannot set ICMP channel options (%s)\n",
                        prog_name, err_why(ERR_CONVERT(error)));
                exit(1);
        }
#endif

	/* Start time related things. */
	now= time(nil);
	timeout= 0;

#ifndef AMOEBA
	asyn_init(&asyn);

	while (1) {
		ssize_t r;

		if (do_rip) {
			/* Try a RIP read. */
			r= asyn_read(&asyn, rip_fd, rip_buf, sizeof(rip_buf));
			if (r < 0) {
				if (errno == EIO) fatal(udp_device);
			} else {
				now= time(nil);
				rip_incoming(r);
			}
		}

		if (do_rdisc) {
			/* Try an IRDP read. */
			r= asyn_read(&asyn, irdp_fd, irdp_buf,
							sizeof(irdp_buf));
			if (r < 0) {
				if (errno == EIO) fatal(ip_device);
			} else {
				now= time(nil);
				irdp_incoming(r);
			}
		}
		fflush(stdout);

		/* Wait for a RIP or IRDP packet or a timeout. */
		tv.tv_sec= timeout;
		tv.tv_usec= 0;
		if (asyn_wait(&asyn, 0, timeout == NEVER ? nil : &tv) < 0) {
			/* Timeout? */
			if (errno != EINTR && errno != EAGAIN)
				fatal("asyn_wait()");
			now= time(nil);
			time_functions();
		}
	}
#else /* AMOEBA */
	
	/* start threads to handle incoming RIP and ICMP packets */
	if (!thread_newthread(rip_thread, RIP_STACK, NULL, 0)) {
                fprintf(stderr, "%s: unable to create thread\n", prog_name);
                exit(1);
        }
        if (!thread_newthread(irdp_thread, IRDP_STACK, NULL, 0)) {
                fprintf(stderr, "%s: unable to create thread\n", prog_name);
                exit(1);
        }

	for (;;) {
	    int waittime;

	    /* default to 60 seconds unless the waittime is reasonable */
	    if (timeout == NEVER || timeout == 0) {
		waittime = 60;
	    } else {
		waittime = timeout - now;
		if (waittime <= 0 || waittime > 10000) {
		    waittime = 60;
		}
	    }

	    /* printf("%s: timeout 0x%lx now 0x%lx waittime %d\n",
	              prog_name, timeout, now, waittime); */

	    sleep(waittime);
	    now= time(nil);
	    time_functions();
	}
#endif
}