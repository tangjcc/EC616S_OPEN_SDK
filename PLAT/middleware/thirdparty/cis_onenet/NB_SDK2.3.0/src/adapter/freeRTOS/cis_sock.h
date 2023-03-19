#ifndef _NETWORK_IO_HEADER_
#define _NETWORK_IO_HEADER_


//#include <stdint.h>
#include <cis_def.h>

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "arch.h"


/*#   define ENETUNREACH WSAENETUNREACH*/
#   define net_errno (errno)
int net_Init(void);
void net_Close( int fd );
int net_Socket (int family, int socktype, int proto);


#endif /* _BASE_NETWORK_IO_HEADER_ */

