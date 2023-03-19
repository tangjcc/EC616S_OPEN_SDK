
/* coap_config.h
 *
 * Copyright (C) 2012,2014 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

#ifndef __COAP_CONFIG_H__
#define __COAP_CONFIG_H__

// Defined in SConscript's CPPDEFINES
#ifdef WITH_POSIX
//#include <sys/socket.h>
#include "lwip/opt.h"
#include "lwip/def.h"

#include <lwip/sockets.h>
//#include "freertos_sockets.h"
#if defined(__GNUC__)
#include "sys/types.h"
#elif defined(__CC_ARM)
#include "armcc/types.h"
#endif
#include "time.h"

/* coap_config.h.  Generated from coap_config.h.in by configure.  */
/* coap_config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */
//#if 1
/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the <assert.h> header file. */
#if defined(__CC_ARM)
#define HAVE_ASSERT_H 0
//#define assert(x)
#elif defined(__GNUC__)
#define HAVE_ASSERT_H 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if the system has libcunit */
/* #undef HAVE_LIBCUNIT */

/* Define if the system has libgnutls28 */
/* #undef HAVE_LIBGNUTLS */

/* Define if the system has libtinydtls */
//#define HAVE_LIBTINYDTLS 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `malloc' function. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define if the system has libssl1.1 */
/* #undef HAVE_OPENSSL */

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
//#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strnlen' function. */
//#define HAVE_STRNLEN 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the <syslog.h> header file. */
//#define HAVE_SYSLOG_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
//#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1


/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 0

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/unistd.h> header file. */
#define HAVE_SYS_UNISTD_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

#define COAP_RESOURCES_NOHASH 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "libcoap-developers@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libcoap"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libcoap 4.2.0rc4"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libcoap"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://libcoap.net/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "4.2.0rc4"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

#define IP_MULTICAST_TTL   5
#define IP_MULTICAST_IF    6
#define IP_MULTICAST_LOOP  7


#define IP_PKTINFO   IP_MULTICAST_IF
#define IPV6_PKTINFO IPV6_V6ONLY

#define CUSTOM_COAP_NETWORK_ENDPOINT
#define CUSTOM_COAP_NETWORK_SEND
#define CUSTOM_COAP_NETWORK_READ

#endif

#define HAVE_STDIO_H


#endif /* _CONFIG_H_ */
