/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcpacket.c
 Description:    - rohc pakcet related api
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/
#include "rohccontext.h"
#include "osasys.h"
#include "debug_log.h"


/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/



//RFC5795 5.3.2
UINT8 RohcDecodeSdvValue(UINT8 *data, UINT16 len, UINT32 *value, UINT8 *bits)
{
    UINT8 bytes = 0;

    //input parameter check
    GosDebugBegin(data != PNULL && bits != PNULL && value != PNULL, data, value, bits);
    return bytes;
    GosDebugEnd();

    if(len < 1)
    {
        return 0;
    }

    if(((*data) & 0x80) == 0)
    {
        bytes = 1;
        *value = (*data) & 0x7F;
        *bits = ROHC_SDV_VALUE_BITS_1;
        
    }
    else if(((*data) & 0xC0) == 0x80)
    {
        if( len < 2)
        {
            return 0;            
        }

        bytes = 2;
        *value = (((*data) & 0x3F) << 8) | (*data);
        *bits = ROHC_SDV_VALUE_BITS_2;
    }
    else if(((*data) & 0xE0) == 0xC0)
    {
        if(len < 3)
        {
            return 0;
        }

        bytes = 3;
        *value = (((*data) & 0x1F) << 16) | ((*(data +1)) << 8) | (*(data +2));
        *bits = ROHC_SDV_VALUE_BITS_3;
    }
    else if(((*data) & 0xE0) == 0XE0)
    {
        if(len < 4)
        {
            return 0;
        }

        bytes = 4;
        *value = (((*data) & 0x1F) << 24) | ((*(data +1)) << 16) | ((*(data +2)) << 8) | (*(data +3));
        *bits = ROHC_SDV_VALUE_BITS_4;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDecodeSdvValue_1, P_WARNING, 1, "RohcDecodeSdvValue invalid data %u", *data);
    }

    return bytes;
}

UINT8 RohcEncodeSdvValue(UINT32 value, UINT8 maxBytes, UINT8 *buff)
{
    UINT8 bytes = 0;

    //input parameter check
    GosDebugBegin(buff != PNULL && maxBytes <= 4, maxBytes, buff, 0);
    return bytes;
    GosDebugEnd();
    
    //calculate value bits
    if(value <= ((1 << ROHC_SDV_VALUE_BITS_1) -1))
    {
        bytes = 1;
    }
    else if(value <= ((1 << ROHC_SDV_VALUE_BITS_2) -1))
    {
        bytes = 2;
    }
    else if(value <= ((1 << ROHC_SDV_VALUE_BITS_3) -1))
    {
        bytes = 3;
    }
    else if(value <= ((1 << ROHC_SDV_VALUE_BITS_4) -1))
    {
        bytes = 4;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcEncodeSdvValue_1, P_WARNING, 1, "RohcEncodeSdvValue invalid value %u", value);
        return bytes;
    }

    if(maxBytes < bytes)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcEncodeSdvValue_2, P_WARNING, 2, "RohcEncodeSdvValue need bytes %u, but maxbytes %u", bytes, maxBytes);
        return 0;
    }

    switch(bytes)
    {
        case 1:
        {
            buff[0] = value & 0x7F;
            break;
        }
        case 2:
        {
            buff[0] = (((value >> 8) & 0x3F) | (2 << 6)) & 0xFF;
            buff[1] = value & 0xFF;
            break;
        }
        case 3:
        {
            buff[0] = (((value >> 16) & 0x1F) | (6 << 5)) & 0xFF;
            buff[1] = (value >> 8) & 0xFF;
            buff[2] = value & 0xFF;
            break;
        }
        case 4:
        {
            buff[0] = (((value >> 24) & 0x1F) | (7 << 5)) & 0xFF;
            buff[1] = (value >> 16) & 0xFF;
            buff[2] = (value >> 8) & 0xFF;
            buff[3] = value & 0xFF;
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcEncodeSdvValue_3, P_WARNING, 1, "RohcEncodeSdvValue invalid bytes %u", bytes);
            return bytes;            
        }
    }

    return bytes;
}


RohcPacketType RohcGetPacketTypeFromTypeByte(UINT8 *typeByte)
{
            //input parameter check
    GosDebugBegin(typeByte != PNULL, typeByte, 0, 0);
    return ROHC_PACKET_TYPE_INVALID;
    GosDebugEnd();

    //check padding
    if(ROHC_CHECK_PADDING_TYPE(*typeByte))
    {
        return ROHC_PACKET_TYPE_PADDING;
    }
    else if(ROHC_CHECK_SEGEMNT_TYPE(*typeByte))
    {
        return ROHC_PACKET_TYPE_SEGMENT;
    }else if(ROHC_CHECK_ADD_CID_TYPE(*typeByte))
    {
        return ROHC_PACKET_TYPE_ADDCID;
    }else if(ROHC_CHECK_FEEDBACK_TYPE(*typeByte))
    {
        return ROHC_PACKET_TYPE_FEEDBACK;
    }else if(ROHC_CHECK_IR_DYN_TYPE(*typeByte))
    {
        return ROHC_PACKET_TYPE_IR_DYN;
    }else if(ROHC_CHECK_IR_TYPE(*typeByte))
    {
        return ROHC_PACKET_TYPE_IR;
    }
    else if(RohcCheckNormalIpPacket(typeByte))
    {
        return ROHC_PACKET_TYPE_NORMAL;
    }
    else
    {
        return ROHC_PACKET_TYPE_INVALID;
    }
}

void RohcSetPacketTypeCode(UINT8 *typeCode, UINT8 type)
{
            //input parameter check
    GosDebugBegin(typeCode != PNULL, typeCode, 0, 0);
    return;
    GosDebugEnd();

    switch(type)
    {
        case ROHC_PACKET_TYPE_PADDING:
        {
            *typeCode = 0xE0;
            break;
        }
        case ROHC_PACKET_TYPE_SEGMENT:
        {
            *typeCode = 0xFE;
            break;
        }
        case ROHC_PACKET_TYPE_ADDCID:
        {
            *typeCode = 0xE0;
            break;
        }
        case ROHC_PACKET_TYPE_FEEDBACK:
        {
            *typeCode = 0xF0;
            break;
        } 
        case ROHC_PACKET_TYPE_IR_DYN:
        {
            *typeCode = 0xF8;
            break;
        }
        case ROHC_PACKET_TYPE_IR:
        {
            *typeCode = 0xFC;
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcSetPacketTypeCode_1, P_ERROR, 1, "RohcSetPacketTypeCode note support %u", type);
            break;
        }
    }
}

void RohcSetProfileCode(UINT8 *profileCode, UINT8 profileId)
{
    //input parameter check
    GosDebugBegin(profileCode != PNULL, profileCode, 0, 0);
    return;
    GosDebugEnd();

    *profileCode = profileId;
}

BOOL RohcParseFeedbackPkt(RohcInputBuff *input, RohcPacketBuff *output)
{
    UINT8 sizeData;
    UINT8 sizeHeader;
    UINT8 *typeCode;
    UINT8 *cidInfo;
    UINT32 cid;
    RohcFeedbackPacket *feedback;

    //input parameter check
    GosDebugBegin(input != PNULL && output != PNULL, input, output, 0);
    return FALSE; 
    GosDebugEnd();

    feedback = &(output->private.infoFeedback);
    typeCode = input->pData;

    //get size
    if(ROHC_GET_SIZE_FROM_TYE(*typeCode) != 0)
    {
        sizeData = ROHC_GET_SIZE_FROM_TYE(*typeCode);
        sizeHeader = 1;
        cidInfo = typeCode + 1;      
    }
    else
    {
        sizeData = *((UINT8 *)(typeCode +1));
        sizeHeader = 2;
        cidInfo = typeCode + 2;
    }

    //check size
    if(sizeHeader + sizeData != input->dataLen)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcParseFeedbackPkt_1, P_ERROR, 3, "RohcParseFeedbackPkt input size %u is not equal feedback header size %u and feedback data size %u"
            input->dataLen, sizeHeader, sizeData);
        return FALSE;
    }

    //get cid info
    //small cid
    if(RohcGetPacketTypeFromTypeByte(cidInfo) == ROHC_PACKET_TYPE_ADDCID)
    {
        cid = ROHC_GET_CID_FROM_ADD_CID(*cidInfo);
        OsaCheck(cid != 0, cid, 0, 0);
        feedback->lenFeedbackData = sizeData - 1;
        feedback->pFeedbackData = cidInfo + 1;
        feedback->cid = cid & 0x000000FF;        
    }
    else //large cid
    {
        UINT8 largeCidBytes;
        UINT8 valueBits;
        largeCidBytes = RohcDecodeSdvValue(cidInfo, 2, &cid, &valueBits);
        if(largeCidBytes == 1)
        {
            feedback->lenFeedbackData = sizeData - 1;
            feedback->pFeedbackData = cidInfo + 1;
            feedback->cid = cid & 0x000000FF;            
        }
        else if(largeCidBytes == 2)
        {
            feedback->lenFeedbackData = sizeData - 2;
            feedback->pFeedbackData = cidInfo + 2;
            feedback->cid = cid & 0x000000FF;        
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcParseFeedbackPkt_2, P_ERROR, 0, "RohcParseFeedbackPkt parse cid fail");
            return FALSE;            
        }
    }

    output->type = ROHC_PACKET_TYPE_FEEDBACK;
    output->uncompPayload = PNULL;

    return TRUE;

    
}

BOOL RohcParseGeneralHeaderPkt(RohcInputBuff *input, RohcPacketBuff *output, UINT16 maxCid)
{
    UINT8 *typeCode;
    UINT8 *pHeaderBody;
    UINT32 cid;
    UINT8 type;

    //input parameter check
    GosDebugBegin(input != PNULL && output != PNULL, input, output, 0);
    return FALSE; 
    GosDebugEnd();

    //small cid
    if(ROHC_CHECK_SMALL_CID(maxCid))
    {
        if(RohcGetPacketTypeFromTypeByte(input->pData) == ROHC_PACKET_TYPE_ADDCID)
        {

            cid = ROHC_GET_CID_FROM_ADD_CID(*(input->pData));
            typeCode = input->pData + 1;
            pHeaderBody = input->pData + 2;
        }
        else
        {
            cid = ROHC_DEFAULT_CID;
            typeCode = input->pData;
            pHeaderBody = input->pData + 1;
        }
    }    
    else if(ROHC_CHECK_LARGE_CID(maxCid))
    {
        UINT8 largeCidBytes;
        UINT8 valueBits;
        largeCidBytes = RohcDecodeSdvValue(input->pData +1 , 2, &cid, &valueBits);
        if(largeCidBytes == 1)
        {
            pHeaderBody =  input->pData +2;           
        }
        else if(largeCidBytes == 2)
        {
            pHeaderBody =  input->pData +3;         
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcParseFeedbackPkt_2, P_ERROR, 0, "RohcParseFeedbackPkt parse cid fail");
            return FALSE;            
        }
        
        typeCode = input->pData;
    }
    else
    {
            ECOMM_TRACE(UNILOG_ROHC, RohcParseFeedbackPkt_3, P_ERROR, 1, "RohcParseFeedbackPkt parse fail, invalid max cid %u", maxCid);
            return FALSE;        
    }

    //parse header type
    type = RohcGetPacketTypeFromTypeByte(typeCode);

    switch(type)
    {
        case ROHC_PACKET_TYPE_IR:
        {
            RohcIRPacket *irInfo;
            irInfo = &(output->private.infoIR);
            irInfo->xInfo = ((*typeCode) & 0x01);
            irInfo->profileId = *pHeaderBody;
            irInfo->CRC = *(pHeaderBody + 1);
            irInfo->infoProfilespeccific = pHeaderBody + 2;
            irInfo->pRawWithoutPadding = input->pData;
            irInfo->lenRawWithoutPadding = input->dataLen;
            irInfo->cid = cid & 0x0000FFFF;
            output->type = ROHC_PACKET_TYPE_IR;

            break;
        }
        case ROHC_PACKET_TYPE_IR_DYN:
        {
            RohcIRDYNPacket *irdynInfo;
            irdynInfo = &(output->private.infoIRDYN);
            irdynInfo->profileId = *pHeaderBody;
            irdynInfo->CRC = *(pHeaderBody + 1);
            irdynInfo->infoProfilespeccific = pHeaderBody + 2;
            irdynInfo->pRawWithoutPadding = input->pData;
            irdynInfo->cid = cid & 0x0000FFFF;
            output->type = ROHC_PACKET_TYPE_IR_DYN;

            break;
        }
        case ROHC_PACKET_TYPE_NORMAL:
        {
            RohcNormalPacket *normalInfo;
            normalInfo = &(output->private.infoNormal);
            normalInfo->firstOctet = *typeCode;
            normalInfo->restOctect = pHeaderBody;
            normalInfo->cid = cid & 0x0000FFFF;
            output->type = ROHC_PACKET_TYPE_NORMAL;

            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcParseFeedbackPkt_4, P_ERROR, 0, "RohcParseFeedbackPkt not support now");
            output->uncompPayload = PNULL;
            return FALSE;              
        }
    }
    
    output->uncompPayload = PNULL;
    
    return TRUE;
    
}

BOOL RohcParseRohcPkt(RohcInputBuff *input, RohcPacketBuff *output, UINT16 maxCid)
{
    UINT8 *currData;
    UINT16 remainLen;
    RohcInputBuff inputWithoutPadding;

    //input parameter check
    GosDebugBegin(input != PNULL && output != PNULL, input, output, 0);
    return FALSE; 
    GosDebugEnd();

    currData = input->pData;
    remainLen = input->dataLen;
    output->pRawPkt = input->pData;
    output->lenRawPkt = input->dataLen;

    //skip padding data
    while(RohcGetPacketTypeFromTypeByte(currData) == ROHC_PACKET_TYPE_PADDING)
    {
        currData = currData + 1;
        remainLen --;
    }

    inputWithoutPadding.dataLen = remainLen;
    inputWithoutPadding.pData = currData;

    //check whether feedback packet
    if(RohcGetPacketTypeFromTypeByte(currData) == ROHC_PACKET_TYPE_FEEDBACK)
    {  
        //parse rohc feedback pkt
        if(RohcParseFeedbackPkt(&inputWithoutPadding, output) == TRUE)
        {
            return TRUE;
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcParseRohcPkt_1, P_ERROR, 0, "RohcParseRohcPkt parse feedback pkt fail");
            return FALSE;
        }
        
    }

    //parse header
    if(RohcParseGeneralHeaderPkt(&inputWithoutPadding, output, maxCid) == FALSE)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcParseRohcPkt_2, P_ERROR, 0, "RohcParseRohcPkt parse gernal header fail");
        return FALSE;        
    }

    return TRUE;
}

BOOL RohcCreateIrHeaderWithoutCrc(UINT8 profileId, UINT16 cid, RohcOutputBuff *output, UINT8 **pCrc)
{
    UINT8 *pType;
    UINT8 *pProfile;
    UINT8 *pCRC;
   
    //input parameter check
    GosDebugBegin(output != PNULL, output, 0 , 0);
    return FALSE; 
    GosDebugEnd();

    output->dataLen = 0;
    
    if(ROHC_CHECK_SMALL_CID(cid))
    {
        if(cid == ROHC_DEFAULT_CID)
        {
            if(output->buffLen < 3)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateIrHeaderWithoutCrc_1, P_ERROR, 1, "RohcCreateIrHeaderWithoutCrc output buff len invalid %u", output->buffLen);
                return FALSE;
            }
            pType = &(output->pData[0]);
            pProfile = &(output->pData[1]);
            pCRC = &(output->pData[2]);
            output->dataLen = 3;        
        }
        else
        {
            if(output->buffLen < 4)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateIrHeaderWithoutCrc_2, P_ERROR, 1, "RohcCreateIrHeaderWithoutCrc output buff len invalid %u", output->buffLen);
                return FALSE;
            }
            output->pData[0] = cid | 0xE0; //add ADD-CID for small cid
            pType = &(output->pData[1]);
            pProfile = &(output->pData[2]);
            pCRC = &(output->pData[3]);
            output->dataLen = 4;
        }
    }
    else if(ROHC_CHECK_LARGE_CID(cid))
    {
        UINT8 cidByes = 0;
        UINT8 cidBuff[2];
        cidByes = RohcEncodeSdvValue(cid, ROHC_LARGE_CID_MAX_BYTES, cidBuff);
        if(cidByes == 1)
        {
            if(output->buffLen < 4)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateIrHeaderWithoutCrc_3, P_ERROR, 1, "RohcCreateIrHeaderWithoutCrc output buff len invalid %u", output->buffLen);
                return FALSE;
            }        
            output->pData[1] = cidBuff[0]; //add cid info
            pType = &(output->pData[0]);
            pProfile = &(output->pData[2]);
            pCRC = &(output->pData[3]);
            output->dataLen = 4;            
        }
        else if(cidByes == 2)
        {
            if(output->buffLen < 5)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateIrHeaderWithoutCrc_4, P_ERROR, 1, "RohcCreateIrHeaderWithoutCrc output buff len invalid %u", output->buffLen);
                return FALSE;
            }         
            output->pData[1] = cidBuff[0]; //add cid info
            output->pData[2] = cidBuff[1]; //add cid info
            pType = &(output->pData[0]);
            pProfile = &(output->pData[3]);
            pCRC = &(output->pData[4]);
            output->dataLen = 5;             
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcCreateIrHeaderWithoutCrc_5, P_ERROR, 1, "RohcCreateIrHeaderWithoutCrc invalid cid %u", cid);
            return FALSE;            
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCreateIrHeaderWithoutCrc_6, P_ERROR, 1, "RohcCreateIrHeaderWithoutCrc invalid cid %u", cid);
        return FALSE;        
    }

    GosDebugBegin(pType != PNULL && pProfile!= PNULL && pCRC != PNULL, pType, pProfile , pCRC);    
    return FALSE; 
    GosDebugEnd();

    //set Type code
    RohcSetPacketTypeCode(pType, ROHC_PACKET_TYPE_IR);

    //set profile ID
    RohcSetProfileCode(pProfile, profileId);

    *pCrc = pCRC;

    //calculate CRC-8 value
//    crcResult = RohcGernCrc8(output->pData, output->dataLen -1);

//    pCRC = crcResult;

    return TRUE;
    
}

BOOL RohcCreateNormalHeader(UINT8 ipFirst, UINT16 cid, RohcOutputBuff *output)
{

    UINT8 *pType;

   //input parameter check
    GosDebugBegin(output != PNULL, output, 0, 0);    
    return FALSE; 
    GosDebugEnd();

    output->dataLen = 0;
    
    if(ROHC_CHECK_SMALL_CID(cid))
    {
        if(cid == ROHC_DEFAULT_CID)
        {
            if(output->buffLen < 1)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateNormalHeader_1, P_ERROR, 1, "RohcCreateNormalHeader output buff len invalid %u", output->buffLen);
                return FALSE;
            }
            pType = &(output->pData[0]);
            output->dataLen = 1;        
        }
        else
        {
            if(output->buffLen < 2)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateNormalHeader_2, P_ERROR, 1, "RohcCreateNormalHeader output buff len invalid %u", output->buffLen);
                return FALSE;
            }
            output->pData[0] = cid | 0xE0; //add ADD-CID for small cid
            pType = &(output->pData[1]);
            output->dataLen = 2;
        }
    }
    else if(ROHC_CHECK_LARGE_CID(cid))
    {
        UINT8 cidByes = 0;
        UINT8 cidBuff[2];
        cidByes = RohcEncodeSdvValue(cid, ROHC_LARGE_CID_MAX_BYTES, cidBuff);
        if(cidByes == 1)
        {
            if(output->buffLen < 2)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateNormalHeader_3, P_ERROR, 1, "RohcCreateNormalHeader output buff len invalid %u", output->buffLen);
                return FALSE;
            }        
            output->pData[1] = cidBuff[0]; //add cid info
            pType = &(output->pData[0]);
            output->dataLen = 2;            
        }
        else if(cidByes == 2)
        {
            if(output->buffLen < 3)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateNormalHeader_4, P_ERROR, 1, "RohcCreateNormalHeader output buff len invalid %u", output->buffLen);
                return FALSE;
            }        
            output->pData[1] = cidBuff[0]; //add cid info
            output->pData[2] = cidBuff[1]; //add cid info
            pType = &(output->pData[0]);
            output->dataLen = 3;             
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcCreateNormalHeader_5, P_ERROR, 1, "RohcCreateNormalHeader invalid cid %u", cid);
            return FALSE;            
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCreateNormalHeader_6, P_ERROR, 1, "RohcCreateNormalHeader invalid cid %u", cid);
        return FALSE;        
    }

    GosDebugBegin(pType != PNULL, pType, 0, 0);    
    return FALSE; 
    GosDebugEnd();

    //set first octect of IP packet
    *pType = ipFirst;

    return TRUE;
    
}

BOOL RohcCreateFeedback(UINT16 cid, RohcFeebackData *feedbackData, RohcOutputBuff *output)
{

   //input parameter check
    GosDebugBegin(output != PNULL && feedbackData != PNULL, output, feedbackData, 0);    
    return FALSE; 
    GosDebugEnd();

    output->dataLen = 0;

    //firt calculate cid bytes
     if(ROHC_CHECK_SMALL_CID(cid))
    {
        if(cid == ROHC_DEFAULT_CID)
        {
            if(feedbackData->dataLen <= 0x07)
            {
                GosDebugBegin(output->buffLen >= (feedbackData->dataLen + 2), output->buffLen, 0, 0);   
                return FALSE; 
                GosDebugEnd();
            
                //set feedback type
                RohcSetPacketTypeCode(&output->pData[0], ROHC_PACKET_TYPE_FEEDBACK);
                //set size code
                output->pData[0] |= feedbackData->dataLen;
            
                memcpy(&(output->pData[1]), feedbackData->pData, feedbackData->dataLen);
                output->dataLen = feedbackData->dataLen + 1;
            }
            else
            {
                GosDebugBegin(output->buffLen >= (feedbackData->dataLen + 3), output->buffLen, 0, 0);   
                return FALSE; 
                GosDebugEnd();
            
                //set feedback type
                RohcSetPacketTypeCode(&output->pData[0], ROHC_PACKET_TYPE_FEEDBACK);
                //set size code
                output->pData[1] = feedbackData->dataLen;
            
                memcpy(&(output->pData[2]), feedbackData->pData, feedbackData->dataLen);
                output->dataLen = feedbackData->dataLen + 2;
            }            
        }
        else
        {
            if(feedbackData->dataLen + 1 <= 0x07)
            {
                GosDebugBegin(output->buffLen >= (feedbackData->dataLen + 2), output->buffLen, 0, 0);   
                return FALSE; 
                GosDebugEnd();
            
                //set feedback type
                RohcSetPacketTypeCode(&output->pData[0], ROHC_PACKET_TYPE_FEEDBACK);
                //set size code
                output->pData[0] |= (feedbackData->dataLen + 1);
                //set cid info           
                output->pData[1] = cid | 0xE0; //add ADD-CID for small cid
            
                memcpy(&(output->pData[2]), feedbackData->pData, feedbackData->dataLen);
                output->dataLen = feedbackData->dataLen + 2;
            }
            else
            {
                GosDebugBegin(output->buffLen >= (feedbackData->dataLen + 3), output->buffLen, 0, 0);   
                return FALSE; 
                GosDebugEnd();
            
                //set feedback type
                RohcSetPacketTypeCode(&output->pData[0], ROHC_PACKET_TYPE_FEEDBACK);
                //set size code
                output->pData[1] = (feedbackData->dataLen + 1);
                //set cid info           
                output->pData[2] = cid | 0xE0; //add ADD-CID for small cid
            
                memcpy(&(output->pData[3]), feedbackData->pData, feedbackData->dataLen);
                output->dataLen = feedbackData->dataLen + 3;
            }
        }
       
    }
    else if(ROHC_CHECK_LARGE_CID(cid))
    {
        UINT8 cidByes = 0;
        UINT8 cidBuff[2];
        cidByes = RohcEncodeSdvValue(cid, ROHC_LARGE_CID_MAX_BYTES, cidBuff);
        if(cidByes == 1 || cidByes == 2)
        {
            if(feedbackData->dataLen + cidByes <= 0x07)
            {
                GosDebugBegin(output->buffLen >= (feedbackData->dataLen + cidByes + 1), output->buffLen, 0, 0);  
                return FALSE; 
                GosDebugEnd();
            
                //set feedback type
                RohcSetPacketTypeCode(&output->pData[0], ROHC_PACKET_TYPE_FEEDBACK);
                //set size code
                output->pData[0] |= (feedbackData->dataLen + cidByes);
                //set cid info
                if(cidByes == 1)
                {
                    output->pData[1] = cidBuff[0];
                    memcpy(&(output->pData[2]), feedbackData->pData, feedbackData->dataLen);
                    output->dataLen = feedbackData->dataLen + 2;                    
                }
                else
                {
                    output->pData[1] = cidBuff[0];
                    output->pData[2] = cidBuff[1];
                    memcpy(&(output->pData[3]), feedbackData->pData, feedbackData->dataLen);
                    output->dataLen = feedbackData->dataLen + 3;                     
                }           

            }
            else
            {
                GosDebugBegin(output->buffLen >= (feedbackData->dataLen + 3), output->buffLen, 0, 0);  
                return FALSE; 
                GosDebugEnd();
                
                //set feedback type
                RohcSetPacketTypeCode(&output->pData[0], ROHC_PACKET_TYPE_FEEDBACK);
                //set size code
                output->pData[1] = (feedbackData->dataLen + cidByes);
                //set cid info           
                //set cid info
                if(cidByes == 1)
                {
                    output->pData[2] = cidBuff[0];
                    memcpy(&(output->pData[3]), feedbackData->pData, feedbackData->dataLen);
                    output->dataLen = feedbackData->dataLen + 3;                    
                }
                else
                {
                    output->pData[2] = cidBuff[0];
                    output->pData[3] = cidBuff[1];
                    memcpy(&(output->pData[4]), feedbackData->pData, feedbackData->dataLen);
                    output->dataLen = feedbackData->dataLen + 4;                     
                }

            }          
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcCreateFeedback_1, P_ERROR, 1, "RohcCreateFeedback invalid cid %u", cid);
            return FALSE;            
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCreateFeedback_2, P_ERROR, 1, "RohcCreateFeedback invalid cid %u", cid);
        return FALSE;        
    }

    return TRUE;
}



