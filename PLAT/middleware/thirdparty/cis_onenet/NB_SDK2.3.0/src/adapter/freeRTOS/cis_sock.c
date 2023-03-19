#include <cis_sock.h>
#include <stdlib.h>
#include <stdio.h>
#include <cis_log.h>

int net_Init(void)
{
    return 1;
}

void net_Uninit(void)
{
}

void net_Close( int fd )
{
    closesocket (fd);
}

int net_SetNonBlock(int socket, int enable)
{
    int ret = ioctlsocket(socket, FIONBIO, (unsigned long *)(&enable));
	if(ret == 0)
		return 1;
	else
		return 0;
}

int net_Socket (int family, int socktype, int proto)
{
	int fd;

	fd = socket (family, socktype, proto);
	if (fd == -1)
	{
		LOGE( "cannot create socket, error no: %d.\n", net_errno);
		return -1;
	}
	
	net_SetNonBlock( fd, 1);
    return fd;
}
