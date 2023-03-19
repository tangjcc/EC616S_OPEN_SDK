/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcippacket.c
 Description:    - rohc ip packet related api
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/
#include "pssys.h"
#include "debug_log.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip6.h"
#include "rohcippacket.h"


/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

BOOL RohcParseIpPacket(RohcInputBuff *input, RohcIpPacket *output)
{
    struct ip_hdr *ip4Hdr;
    struct ip6_hdr *ip6Hdr;
    
     //input parameter check
    GosDebugBegin(input != PNULL && output != PNULL, input, output, 0);
    return FALSE;
    GosDebugEnd();        
        
    output->ipProtocol = 0;
    output->length = input->dataLen;
    output->nextProtocol = 0;
    output->pData = input->pData;
    output->pNextProtocol = PNULL;        

    if(ROHC_IP_HDR_GET_VERSION(input->pData) == ROHC_IPV4_PROTOCOL)
    {
        ip4Hdr = (struct ip_hdr *)input->pData;
        
        GosDebugBegin(ip4Hdr != PNULL, ip4Hdr, 0, 0);
        return FALSE;
        GosDebugEnd();        
        
        output->ipProtocol = ROHC_IPV4_PROTOCOL;
        output->nextProtocol = IPH_PROTO(ip4Hdr);
        output->pNextProtocol = input->pData - IPH_LEN(ip4Hdr);        
    }
    else if(ROHC_IP_HDR_GET_VERSION(input->pData) == ROHC_IPV6_PROTOCOL)
    {
        ip6Hdr = (struct ip6_hdr *)input->pData;
        
        GosDebugBegin(ip6Hdr != PNULL, ip6Hdr, 0, 0);
        return FALSE;
        GosDebugEnd();

        output->ipProtocol = ROHC_IPV6_PROTOCOL;
        output->nextProtocol = IP6H_NEXTH(ip6Hdr);
        output->pNextProtocol = input->pData - IP6_HLEN;     
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcParseIpPacket_1, P_ERROR, 0, "RohcParseIpPacket invalid ip packet");
        return FALSE;
    }

return TRUE;
    
}

BOOL RohcCheckNormalIpPacket(UINT8 *firstOctet)
{
    UINT8 protocol;

        //input parameter check
    GosDebugBegin(firstOctet != PNULL, firstOctet, 0, 0);
    return FALSE;
    GosDebugEnd();

    protocol = ROHC_IP_HDR_GET_VERSION(firstOctet);

    if(protocol == ROHC_IPV4_PROTOCOL || protocol == ROHC_IPV6_PROTOCOL)
    {
        return TRUE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCheckNormalIpPacket_1, P_WARNING, 1, "RohcCheckNormalIpPacket protocol %u is not ip packet", protocol);
        return FALSE;        
    }

}


