/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcapi.c
 Description:    - rohc related api
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/
#include "pssys.h"
#include "debug_log.h"

#include "rohcprofile.h"
#include "rohcchannel.h"
#include "rohccompress.h"
#include "rohcdecompress.h"
#include "rohcfeedback.h"
#include "rohcapi.h"



/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/
UINT32 RohcGetCapability(void)
{
    return RohcGetProfileCapability();
}


/******************************************************************************
 * RohcCreateCompChannel
 * Description: create rohc compress channel
 * input: UINT32 rohcProfiles;UINT16 maxCid;UINT32 mrru
 * output:void *
 * Comment: 
******************************************************************************/

void* RohcCreateCompChannel(UINT32 rohcProfiles, UINT16 maxCid, UINT32 mrru)
{
    ROHC_UNUSED_ARG(mrru);
    
    return (void *)RohcCreateCompressChannel(rohcProfiles, maxCid);
}


/******************************************************************************
 * RohcCreateDecompChannel
 * Description: create rohc decompress channel
 * input: UINT32 rohcProfiles;UINT16 maxCid;UINT32 mrru;UINT8 decompressMode
 * output:void *
 * Comment: 
******************************************************************************/

void* RohcCreateDecompChannel(UINT32 rohcProfiles, UINT16 maxCid, UINT32 mrru, UINT8 decompressMode)
{
    ROHC_UNUSED_ARG(mrru);
    ROHC_UNUSED_ARG(decompressMode);

    return (void *)RohcCreateDecompressChannel(rohcProfiles, maxCid, ROHC_MODE_R);
}


/******************************************************************************
 * RohcDestroyCompChannel
 * Description: destroy rohc compress channel
 * input: void *compChannel
 * output:
 * Comment: 
******************************************************************************/

void RohcDestroyCompChannel(void *compChannel)
{
    RohcDestroyCompressChannel((RohcCompressChannel *)compChannel);
}

/******************************************************************************
 * RohcDestroyDecompChannel
 * Description: destroy rohc decompress channel
 * input: void *decompChannel
 * output:
 * Comment: 
******************************************************************************/

void RohcDestroyDecompChannel(void *decompChannel)
{
    RohcDestroyDecompressChannel((RohcDecompressChannel *)decompChannel);
}

/******************************************************************************
 * RohcResetCompChannel
 * Description: reset rohc compress channel
 * input: void *compChannel
 * output:
 * Comment: 
******************************************************************************/

void RohcResetCompChannel(void *compChannel)
{
    RohcResetCompressChannel((RohcCompressChannel *)compChannel);
}

/******************************************************************************
 * RohcResetDecompChannel
 * Description: reset rohc decompress channel
 * input: void *decompChannel
 * output:
 * Comment: 
******************************************************************************/

void RohcResetDecompChannel(void *decompChannel)
{
    RohcResetDecompressChannel((RohcDecompressChannel *)decompChannel);
}


/******************************************************************************
 * RohcCompressPkt
 * Description: rohc compress raw IP pkt
 * input: 
 * output:
 * Comment: 
******************************************************************************/
BOOL RohcCompressPkt(void *compCahnnel, RohcBuff *ipInput, RohcBuff *compHdrOutput, RohcBuff *payload)
{
    RohcInputBuff input;
    RohcCompressOutputHeaderBuff output;
    
    //input parameter check
    GosDebugBegin(compCahnnel != PNULL && ipInput != PNULL && compHdrOutput != PNULL && payload != PNULL, ipInput, compHdrOutput, payload);
    return FALSE;
    GosDebugEnd(); 

    //init input structure
    input.dataLen = ipInput->dataLen;
    input.buffLen = ipInput->buffLen;
    input.pData = ipInput->pData;

    //init compress output header buff structure
    output.pCompressHdrBuff = compHdrOutput->pData;
    output.buffMaxLen = compHdrOutput->buffLen;

    //do compress
    if(RohcCompressIpPacket(compCahnnel, &input, &output) == TRUE)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressPkt_1, P_INFO, 0, "RohcCompressPkt success");
        compHdrOutput->dataLen = output.compressHdrLen;
        payload->pData = output.pPayload;
        payload->dataLen = output.payloadLen;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressPkt_2, P_ERROR, 0, "RohcCompressPkt fail");
        return FALSE;
    }

    return TRUE;
    
}


/******************************************************************************
 * RohcDecompressPkt
 * Description: rohc decompress raw IP pkt
 * input: 
 * output:
 * Comment: 
******************************************************************************/

BOOL RohcDecompressPkt(void *decompCahnnel, RohcBuff *rohcInput, RohcBuff *decompHdrOutput, RohcBuff *payload, RohcBuff *feedback)
{

    RohcInputBuff input;
    RohcDecompressOutputHeaderBuff output;
    RohcFeedbackBuff buffFeedback;

    //input parameter check   
    GosDebugBegin(decompCahnnel != PNULL && rohcInput != PNULL && decompHdrOutput != PNULL, decompCahnnel, rohcInput, decompHdrOutput);
    return FALSE;
    GosDebugEnd();
    
    GosDebugBegin(payload != PNULL && feedback != PNULL, payload, feedback, 0);
    return FALSE;
    GosDebugEnd();

    //init input rohc pkt structure
    input.dataLen = rohcInput->dataLen;
    input.buffLen = rohcInput->buffLen;
    input.pData = rohcInput->pData;

    //init output decompress hdr structure
    output.buffMaxLen = decompHdrOutput->buffLen;
    output.pDecompressHdrBuff = decompHdrOutput->pData;

    //init output feedback structure
    buffFeedback.buffMaxLen = feedback->buffLen;
    buffFeedback.pFeedbackBuff = feedback->pData;

    //do decompress
    if(RohcDecompressRohcPacket(decompCahnnel, &input, &output, &buffFeedback) == TRUE )
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDecompressPkt_1, P_INFO, 0, "RohcDecompressPkt success");
        decompHdrOutput->dataLen = output.decompressHdrLen;
        payload->pData = output.pPayload;
        payload->dataLen = output.payloadLen;
        feedback->dataLen = buffFeedback.feedbackLen;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDecompressPkt_2, P_ERROR, 0, "RohcDecompressPkt fail");
        return FALSE;
    }

    return TRUE;

    
}
BOOL RohcRcvFeedback(void *compCahnnel, RohcBuff *feedback)
{

    RohcInputBuff input;

    //input parameter check
    GosDebugBegin(compCahnnel != PNULL && feedback != PNULL, compCahnnel, feedback, 0);
    return FALSE;
    GosDebugEnd();

    input.pData = feedback->pData;
    input.dataLen = feedback->dataLen;
    input.buffLen = feedback->buffLen;

    if(RohcProcessFeedback(compCahnnel, &input) == TRUE )
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcRcvFeedback_1, P_INFO, 0, "RohcDecompressPkt success");
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcRcvFeedback_2, P_ERROR, 0, "RohcDecompressPkt fail");
        return FALSE;
    }

    return TRUE;

}





