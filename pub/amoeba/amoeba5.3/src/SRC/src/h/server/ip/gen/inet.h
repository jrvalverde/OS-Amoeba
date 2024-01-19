/*	@(#)inet.h	1.3	96/02/27 10:37:18 */
/*
server/ip/gen/inet.h
*/

#ifndef __SERVER__IP__GEN__INET_H__
#define __SERVER__IP__GEN__INET_H__

ipaddr_t inet_addr _ARGS(( const char *addr ));
ipaddr_t inet_network _ARGS(( const char *addr ));
char *inet_ntoa _ARGS(( ipaddr_t addr ));
int inet_aton _ARGS(( const char *cp, ipaddr_t *pin ));

#endif /* __SERVER__IP__GEN__INET_H__ */

/*
 * $PchHeader: /mount/hd2/minix/include/net/gen/RCS/inet.h,v 1.3 1994/12/14 21:45:12 philip Exp $
 */
