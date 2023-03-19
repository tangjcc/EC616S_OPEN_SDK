/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcippacket.h
 Description:    - rohc common header file
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/
#ifndef ROHC_IP_PACKET_H
#define ROHC_IP_PACKET_H

#include "rohcbuff.h"


#define ROHC_IPV4_PROTOCOL    4
#define ROHC_IPV6_PROTOCOL    6
#define ROHC_IP_PROTO_ICMP    1
#define ROHC_IP_PROTO_IGMP    2
#define ROHC_IP_PROTO_UDP     17
#define ROHC_IP_PROTO_UDPLITE 136
#define ROHC_IP_PROTO_TCP     6

#define ROHC_IP_HDR_GET_VERSION(pkt)   ((*(UINT8*)(pkt)) >> 4)


BOOL RohcParseIpPacket(RohcInputBuff *input, RohcIpPacket *output);

BOOL RohcCheckNormalIpPacket(UINT8 *firstOctet);


#endif

