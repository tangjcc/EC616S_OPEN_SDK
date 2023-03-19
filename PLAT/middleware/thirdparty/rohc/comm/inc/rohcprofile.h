/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcprofile.h
 Description:    - rohc profile related header file
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/

#ifndef ROHC_PROFILE_H
#define ROHC_PROFILE_H

#include "rohccontext.h"
#include "rohcbuff.h"
#include "rohcippacket.h"
#include "rohcpacket.h"

/******************************************************************************
 *****************************************************************************
 * MACRO
 *****************************************************************************
******************************************************************************/
#define ROHC_PROFILE_UTILITY_SUPPORT_MAX 3
#define ROHC_PROFILE_NUMBER_MAX 15


/******************************************************************************
 *****************************************************************************
 * STRUCT/ENUM
 *****************************************************************************
******************************************************************************/

typedef enum
{
	ROHC_PROFILE_UNCOMPRESSED     = 0x0000,

	ROHCv1_PROFILE_RTP            = 0x0001,
	ROHCv1_PROFILE_UDP            = 0x0002,
	ROHCv1_PROFILE_ESP            = 0x0003,
	ROHCv1_PROFILE_IP             = 0x0004,
	ROHCv1_PROFILE_RTP_LLA        = 0x0005,
	ROHCv1_PROFILE_TCP            = 0x0006,
	ROHCv1_PROFILE_UDPLITE_RTP    = 0x0007,
	ROHCv1_PROFILE_UDPLITE        = 0x0008,

	ROHCv2_PROFILE_IP_UDP_RTP     = 0x0101,
	ROHCv2_PROFILE_IP_UDP         = 0x0102,
	ROHCv2_PROFILE_IP_ESP         = 0x0103,
	ROHCv2_PROFILE_IP             = 0x0104,
	ROHCv2_PROFILE_IP_UDPLITE_RTP = 0x0107,
	ROHCv2_PROFILE_IP_UDPLITE     = 0x0108,
} RohcProfileId;

typedef enum
{
    ROHC_PROFILE_PRIORITY_UNUSED    = 0,
    ROHC_PROFILE_PRIORITY_LOW       = 1,
    ROHC_PROFILE_PRIORITY_MEDIUM    = 2,
    ROHC_PROFILE_PRIORITY_HIGH      = 3,
}RohcProfilePriority;


typedef struct RohcProfileMappingCapability_Tag{
    UINT16 profileId;
    UINT16 capablility;
}RohcProfileMappingCapability;


typedef struct RohcCompressProfileUtility_Tag
{
    BOOL (*Create)(RohcCompressContext *context, RohcIpPacket *pIpPkt);
    void (*Destroy)(RohcCompressContext *context);
    BOOL (*CheckProile)(RohcIpPacket *pIpPkt);
    BOOL (*CheckContext)(RohcCompressContext *context, RohcIpPacket *pIpPkt);
    BOOL (*PktCompress)(RohcCompressContext *context, RohcIpPacket *pIpPkt, RohcCompressOutputHeaderBuff *pOutput);
    BOOL (*FeedbackProcess)(RohcCompressContext *context, RohcPacketBuff *pRohcPkt);
}RohcCompressProfileUtility;

typedef struct RohcDecompressProfileUtility_Tag
{
    BOOL (*Create)(RohcDecompressContext *context, RohcPacketBuff *pRohcPkt);
    void (*Destroy)(RohcDecompressContext *context);
    BOOL (*CheckProile)(RohcPacketBuff *pRohcPkt);
    BOOL (*CheckContext)(RohcDecompressContext *context, RohcPacketBuff *pRohcPkt);
    BOOL (*PktDecompress)(RohcDecompressContext *context, RohcPacketBuff *pRohcPkt, RohcDecompressOutputHeaderBuff *pOutput, RohcFeedbackBuff *feedback);
}RohcDecompressProfileUtility;


typedef struct RohcProfileCapability_Tag
{
    UINT32 profileId;
    UINT32 priority;
    RohcCompressProfileUtility *pProfiltCompressUtility;
    RohcDecompressProfileUtility *pProfiltDecompressUtility;
}RohcProfileCapability;


/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

UINT32 RohcGetProfileCapability(void);
RohcProfileCapability *RohcGetProfileUtilityByProfileId(UINT32 profileId);
INT32 RohcGetAdptCompressProfileByIpPacket(UINT32 capability, RohcIpPacket *packet);
INT32 RohcGetAdptDecompressProfileByIpPacket(UINT32 capability, RohcPacketBuff *packet);

#endif
