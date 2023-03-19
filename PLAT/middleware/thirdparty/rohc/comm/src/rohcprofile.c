/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcprofile.c
 Description:    - rohc profile related api
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/

#include "rohcprofile.h"


/******************************************************************************
 *****************************************************************************
 * external
 *****************************************************************************
******************************************************************************/
extern const RohcCompressProfileUtility rohcv1UncompressCompressUtility;
extern const RohcDecompressProfileUtility rohcv1UncompressDecompressUtility;


const RohcProfileCapability gProfileCapability[ROHC_PROFILE_UTILITY_SUPPORT_MAX] =
{
    {ROHC_PROFILE_UNCOMPRESSED, ROHC_PROFILE_PRIORITY_LOW,    (RohcCompressProfileUtility *)&rohcv1UncompressCompressUtility, (RohcDecompressProfileUtility *)&rohcv1UncompressDecompressUtility},
    {ROHCv1_PROFILE_UDP,        ROHC_PROFILE_PRIORITY_HIGH,   NULL, NULL},
    {ROHCv1_PROFILE_IP,         ROHC_PROFILE_PRIORITY_MEDIUM, NULL, NULL},
};

const RohcProfileMappingCapability gProfileMappingCapability[ROHC_PROFILE_NUMBER_MAX] =
{
    {ROHC_PROFILE_UNCOMPRESSED,     0},
    {ROHCv1_PROFILE_RTP,            1},
    {ROHCv1_PROFILE_UDP,            1<<1},
    {ROHCv1_PROFILE_ESP,            1<<2},
    {ROHCv1_PROFILE_IP,             1<<3},
    {ROHCv1_PROFILE_RTP_LLA,        1<<4},
    {ROHCv1_PROFILE_TCP,            1<<5},
    {ROHCv1_PROFILE_UDPLITE_RTP,    1<<9},
    {ROHCv1_PROFILE_UDPLITE,        1<<10},
    {ROHCv2_PROFILE_IP_UDP_RTP,     1<<6},
    {ROHCv2_PROFILE_IP_UDP,         1<<7},
    {ROHCv2_PROFILE_IP_ESP,         1<<8},
    {ROHCv2_PROFILE_IP,             1<<11},
    {ROHCv2_PROFILE_IP_UDPLITE_RTP, 1<<12},
    {ROHCv2_PROFILE_IP_UDPLITE,     1<<13},
};

/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

RohcProfileCapability *RohcGetProfileUtilityByProfileId(UINT32 profileId)
{
    UINT8 i;
    RohcProfileCapability *utility = PNULL;

    for(i = 0; i < ROHC_PROFILE_UTILITY_SUPPORT_MAX; i++)
    {
        if(gProfileCapability[i].profileId == profileId)
        {
            utility = (RohcProfileCapability *)&gProfileCapability[i];
        }
    }

    return utility;
}


UINT32 RohcGetProfileCapability(void)
{
    UINT32 capability = 0;
    UINT8 i;
    RohcProfileCapability *utility = PNULL;

    for(i = 0;i < ROHC_PROFILE_NUMBER_MAX; i ++)
    {
        utility = RohcGetProfileUtilityByProfileId(gProfileMappingCapability[i].profileId);
        if(utility)
        {
            if(utility->pProfiltCompressUtility != NULL && utility->pProfiltDecompressUtility != NULL)
            {
                capability |= gProfileMappingCapability[i].capablility;
            }
        }
    }

    return capability;

}


INT32 RohcGetAdptCompressProfileByIpPacket(UINT32 capability, RohcIpPacket *packet)
{
    UINT8 i;
    UINT32 profileId = 0;
    RohcProfileCapability *utility = PNULL;
    UINT32 priority = 0;
    INT32 result = -1;
    

    //input parameter check
    GosDebugBegin(packet != PNULL, packet, 0, 0);  
    return result; 
    GosDebugEnd();


    for(i = 0;i < ROHC_PROFILE_NUMBER_MAX; i ++)
    {
        profileId = gProfileMappingCapability[i].profileId;
        utility = RohcGetProfileUtilityByProfileId(profileId);
        if(utility)
        {
            if(utility->priority > priority)
            {
                if(utility->pProfiltCompressUtility->CheckProile(packet) == TRUE)
                {
                    result = profileId;
                    priority = utility->priority;
                }
            }
        }
    }

    return result;
}

INT32 RohcGetAdptDecompressProfileByIpPacket(UINT32 capability, RohcPacketBuff *packet)
{
    UINT8 i;
    UINT32 profileId = 0;
    RohcProfileCapability *utility = PNULL;
    UINT32 priority = 0;
    INT32 result = -1;
    

    //input parameter check
    GosDebugBegin(packet != PNULL, packet, 0, 0);  
    return result; 
    GosDebugEnd();

    for(i = 0;i < ROHC_PROFILE_NUMBER_MAX; i ++)
    {
        profileId = gProfileMappingCapability[i].profileId;
        utility = RohcGetProfileUtilityByProfileId(profileId);
        if(utility)
        {
            if(utility->priority > priority)
            {
                if(utility->pProfiltDecompressUtility->CheckProile(packet) == TRUE)
                {
                    result = profileId;
                    priority = utility->priority;
                }
            }
        }
    }

    return result;
}




