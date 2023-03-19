#include "../cis_api.h"
#include "../cis_internals.h"
#include "std_object.h"

typedef struct st_security_instance
{
	struct st_security_instance *	next;        // matches lwm2m_list_t::next
	cis_listid_t					instanceId;  // matches lwm2m_list_t::id
	char *							host;        // ip address;
#if CIS_ENABLE_DTLS
	char *							identify;
	char *							secret;
	char *							public_key;
#endif
	bool                            isBootstrap;
	uint16_t						shortID;
	uint32_t						clientHoldOffTime;
	uint8_t							securityMode;
#if CIS_ENABLE_DTLS
#if CIS_ENABLE_PSK
	uint8_t							id_len;
	uint8_t							psk_len;
#endif
#endif
} security_instance_t;


static uint8_t prv_get_value(st_context_t * contextP,
	                         st_data_t * dataP,
                             security_instance_t * targetP)
{
    switch (dataP->id)
    {
    case RES_SECURITY_URI_ID:
        data_encode_string(targetP->host, dataP);
        return COAP_205_CONTENT;

    case RES_SECURITY_BOOTSTRAP_ID:
        data_encode_bool(targetP->isBootstrap, dataP);
        return COAP_205_CONTENT;

    case RES_SECURITY_SECURITY_ID:
        data_encode_int(targetP->securityMode, dataP);
        return COAP_205_CONTENT;
    case RES_SECURITY_SHORT_SERVER_ID:
        data_encode_int(targetP->shortID, dataP);
        return COAP_205_CONTENT;

    case RES_SECURITY_HOLD_OFF_ID:
        data_encode_int(targetP->clientHoldOffTime, dataP);
        return COAP_205_CONTENT;
    default:
        return COAP_404_NOT_FOUND;
    }
}


static security_instance_t * prv_security_find(st_context_t * contextP,cis_iid_t instanceId)
{
    security_instance_t * targetP;
    targetP = (security_instance_t *)cis_list_find(std_object_get_securitys(contextP), instanceId);
    if (NULL != targetP)
    {
        return targetP;
    }

    return NULL;
}



cis_coapret_t std_security_read(st_context_t * contextP,cis_iid_t instanceId,
                                 int * numDataP,
                                 st_data_t ** dataArrayP,
                                 st_object_t * objectP)
{
    security_instance_t * targetP;
    uint8_t result;
    int i;

    targetP = prv_security_find(contextP,instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    // is the server asking for the full instance ?
    if (*numDataP == 0)
    {
		uint16_t resList[] = { RES_SECURITY_URI_ID,
							   RES_SECURITY_BOOTSTRAP_ID,
							   RES_SECURITY_SHORT_SERVER_ID,
							   RES_SECURITY_HOLD_OFF_ID };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        result = prv_get_value(contextP,(*dataArrayP) + i, targetP);
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

cis_coapret_t std_security_write(st_context_t * contextP,
	cis_iid_t instanceId,
	int numData,
	st_data_t * dataArray,
	st_object_t * objectP)
{
	security_instance_t * targetP;
	int i;
	uint8_t result = COAP_204_CHANGED;

	targetP = prv_security_find(contextP, instanceId);
	if (NULL == targetP)
	{
		return COAP_404_NOT_FOUND;
	}

	i = 0;
	do {
		switch (dataArray[i].id)
		{
		case RES_SECURITY_URI_ID:
			if (targetP->host != NULL) cis_free(targetP->host);
			targetP->host = (char *)cis_malloc(dataArray[i].asBuffer.length + 1);
			if (targetP->host != NULL)
			{
				cis_memset(targetP->host, 0, dataArray[i].asBuffer.length + 1);
				
#if CIS_ENABLE_DTLS 
			   if(TRUE== contextP->isDTLS)
			   {
				 cis_utils_stringCopy(targetP->host, dataArray[i].asBuffer.length - 8 - URI_TAILER_HOST_LEN, (char*)dataArray[i].asBuffer.buffer + 8);
				 cis_memset(contextP->hostRestore, 0, 16);
			   	 cis_memcpy(contextP->hostRestore, targetP->host,  dataArray[i].asBuffer.length - 8 - URI_TAILER_HOST_LEN);
			   }
			   else
			   {
				 cis_utils_stringCopy(targetP->host, dataArray[i].asBuffer.length - 7 - URI_TAILER_HOST_LEN, (char*)dataArray[i].asBuffer.buffer + 7);
				 cis_memset(contextP->hostRestore, 0, 16);
				 cis_memcpy(contextP->hostRestore, targetP->host,  dataArray[i].asBuffer.length - 7 - URI_TAILER_HOST_LEN);
		   	     ECOMM_STRING(UNILOG_ONENET,std_security_write,P_SIG,"hostRestor=%s",(uint8_t*)contextP->hostRestore);
			   }
#else
				cis_utils_stringCopy(targetP->host, dataArray[i].asBuffer.length - URI_HEADER_HOST_LEN - URI_TAILER_HOST_LEN, (char*)dataArray[i].asBuffer.buffer + URI_HEADER_HOST_LEN);
				cis_memset(contextP->hostRestore, 0, dataArray[i].asBuffer.length - URI_HEADER_HOST_LEN - URI_TAILER_HOST_LEN + 1);
				cis_memcpy(contextP->hostRestore, targetP->host,  dataArray[i].asBuffer.length - URI_HEADER_HOST_LEN - URI_TAILER_HOST_LEN);
				ECOMM_STRING(UNILOG_ONENET,std_security_write_1,P_SIG,"hostRestor=%s",(uint8_t*)contextP->hostRestore);

#endif

				result = COAP_204_CHANGED;
			}
			else
			{
				result = COAP_500_INTERNAL_SERVER_ERROR;
			}
			break;

		case RES_SECURITY_SECURITY_ID:
		{
			int64_t value;

			if (1 == data_decode_int(dataArray + i, &value))
			{
				if (value >= 0 && value <= 3)
				{
					targetP->securityMode = (uint8_t)value;
					result = COAP_204_CHANGED;
				}
				else
				{
					result = COAP_406_NOT_ACCEPTABLE;
				}
			}
			else
			{
				result = COAP_400_BAD_REQUEST;
			}
		}
		break;
		case RES_SECURITY_BOOTSTRAP_ID:
			if (1 == data_decode_bool(dataArray + i, &(targetP->isBootstrap)))
			{
				result = COAP_204_CHANGED;
			}
			else
			{
				result = COAP_400_BAD_REQUEST;
			}
			break;

		case RES_SECURITY_SHORT_SERVER_ID:
		{
			int64_t value;

			if (1 == data_decode_int(dataArray + i, &value))
			{
				if (value >= 0 && value <= 0xFFFF)
				{
					targetP->shortID = (uint16_t)value;
					result = COAP_204_CHANGED;
				}
				else
				{
					result = COAP_406_NOT_ACCEPTABLE;
				}
			}
			else
			{
				result = COAP_400_BAD_REQUEST;
			}
		}
		break;

		case RES_SECURITY_HOLD_OFF_ID:
		{
			int64_t value;

			if (1 == data_decode_int(dataArray + i, &value))
			{
				if (value >= 0 && value <= 0xFFFF)
				{
					targetP->clientHoldOffTime = (uint32_t)value;
					result = COAP_204_CHANGED;
				}
				else
				{
					result = COAP_406_NOT_ACCEPTABLE;
				}
			}
			else
			{
				result = COAP_400_BAD_REQUEST;
			}
			break;
		}
#if CIS_ENABLE_DTLS
		case RES_SECURITY_IDENTIFY:
			if (targetP->identify != NULL) cis_free(targetP->identify);
			targetP->identify = (char *)cis_malloc(dataArray[i].asBuffer.length + 1);
			if (targetP->identify != NULL)
			{
				cis_memset(targetP->identify, 0, dataArray[i].asBuffer.length + 1);
				cis_memcpy(targetP->identify, (char*)dataArray[i].asBuffer.buffer, dataArray[i].asBuffer.length);
				targetP->id_len = dataArray[i].asBuffer.length;
				if(contextP->identify != NULL) cis_free(contextP->identify);
				contextP->identify = (char *)cis_malloc(targetP->id_len + 1);
				cis_memset(contextP->identify, 0, targetP->id_len + 1);
				cis_memcpy(contextP->identify, targetP->identify, targetP->id_len);
				result = COAP_204_CHANGED;
    			ECOMM_STRING(UNILOG_ONENET, std_security_write_3, P_INFO, "targetP->identify=%s",(uint8_t*)targetP->identify);
			}
			else
			{
				result = COAP_500_INTERNAL_SERVER_ERROR;
			}
			break;
		case RES_SECURITY_SECRET:
			if (targetP->secret != NULL) cis_free(targetP->secret);
			targetP->secret = (char *)cis_malloc(dataArray[i].asBuffer.length + 1);
			if (targetP->secret != NULL)
			{
				cis_memset(targetP->secret, 0, dataArray[i].asBuffer.length + 1);
				cis_memmove(targetP->secret, (char*)dataArray[i].asBuffer.buffer, dataArray[i].asBuffer.length);
				targetP->psk_len = dataArray[i].asBuffer.length;
				if(contextP->secret != NULL) cis_free(contextP->secret);
				contextP->secret = (char *)cis_malloc(targetP->psk_len + 1);
				cis_memset(contextP->secret, 0, targetP->psk_len + 1);
				cis_memcpy(contextP->secret, targetP->secret, targetP->psk_len);
				result = COAP_204_CHANGED;
	    		ECOMM_STRING(UNILOG_ONENET, std_security_write_4, P_INFO, "targetP->secret=%s",(uint8_t*)targetP->secret);
			}
			else
			{
				result = COAP_500_INTERNAL_SERVER_ERROR;
			}
			break;
		case RES_SECURITY_PUBLIC_KEY:
			if (targetP->public_key != NULL) cis_free(targetP->public_key);
			targetP->public_key = (char *)cis_malloc(dataArray[i].asBuffer.length + 1);
			if (targetP->public_key != NULL)
			{
				cis_memset(targetP->public_key, 0, dataArray[i].asBuffer.length + 1);
				cis_memcpy(targetP->public_key, (char*)dataArray[i].asBuffer.buffer, dataArray[i].asBuffer.length);
				result = COAP_204_CHANGED;
			}
			else
			{
				result = COAP_500_INTERNAL_SERVER_ERROR;
			}
			break;
#endif
		default:
			return COAP_404_NOT_FOUND;
		}
		i++;
	} while (i < numData && result == COAP_204_CHANGED);

	return result;
}


cis_coapret_t std_security_discover(st_context_t * contextP,
								    uint16_t instanceId,
								    int * numDataP,
							        st_data_t ** dataArrayP,
								    st_object_t * objectP)
{
	return COAP_404_NOT_FOUND;
}

bool std_security_create(st_context_t * contextP,
	int instanceId,
	const char* serverHost,
	bool isBootstrap,
	st_object_t * securityObj)
{
	security_instance_t * instSecurity = NULL;
	security_instance_t * targetP = NULL;
	cis_instcount_t instBytes = 0;
	cis_instcount_t instCount = 0;
	cis_iid_t instIndex;
	if (NULL == securityObj)
	{
		return false;
	}

	// Manually create a hard-code instance
	targetP = (security_instance_t *)cis_malloc(sizeof(security_instance_t));
	if (NULL == targetP)
	{
		return false;
	}

	cis_memset(targetP, 0, sizeof(security_instance_t));
	targetP->instanceId = (uint16_t)instanceId;
	targetP->host = (char*)cis_malloc(utils_strlen(serverHost) + 1);
	if (targetP->host == NULL)
	{
		cis_free(targetP);
		return false;
	}
	cis_memset(targetP->host, 0, utils_strlen(serverHost) + 1);
	cis_utils_stringCopy(targetP->host, utils_strlen(serverHost), serverHost);
#if CIS_ENABLE_DTLS
#if CIS_ENABLE_DM
	if (TRUE == isBootstrap && TRUE != contextP->isDM && TRUE == contextP->isDTLS)
#else
	if (TRUE == isBootstrap && TRUE == contextP->isDTLS)
#endif
	{
#if CIS_ENABLE_PSK
		//read psk file and get the encrypt
		targetP->identify = (char*)cis_malloc(NBSYS_IMEI_MAXLENGTH + 2);
		if (targetP->identify == NULL)
		{
			cis_free(targetP->host);
			cis_free(targetP);
			return false;
		}
		cis_memset(targetP->identify, 0, NBSYS_IMEI_MAXLENGTH + 2);
		cissys_getIMEI(targetP->identify, NBSYS_IMEI_MAXLENGTH);
		targetP->id_len = strlen(targetP->identify);
        
        if(NULL==contextP->PSK)
		{
			cis_free(targetP->host);
			cis_free(targetP->identify);
			cis_free(targetP);
			return false;
		}
	
        targetP->psk_len = strlen(contextP->PSK);		
		targetP->secret = (char *)cis_malloc(targetP->psk_len+1);
		if (targetP->secret == NULL)
		{
			cis_free(targetP->host);
			cis_free(targetP->identify);
			cis_free(targetP);
			return false;
		}
		cis_memcpy(targetP->secret,contextP->PSK,targetP->psk_len+1);
		//targetP->psk_len = cissys_getPSK(targetP->secret, NBSYS_PSK_MAXLENGTH);
#endif
	}
#endif

	targetP->isBootstrap = isBootstrap;
	targetP->shortID = 0;
	targetP->clientHoldOffTime = 10;

	instSecurity = (security_instance_t *)std_object_put_securitys(contextP, (cis_list_t*)targetP);

	instCount = (cis_instcount_t)CIS_LIST_COUNT(instSecurity);
	if (instCount == 0)
	{
#if CIS_ENABLE_DTLS
		if (targetP->identify != NULL)
			cis_free(targetP->identify);
		if (targetP->secret != NULL)
			cis_free(targetP->secret);
#endif
		cis_free(targetP->host);
		cis_free(targetP);
		return false;
	}

	/*first security object instance
	 *don't malloc instance bitmap ptr*/
	if (instCount == 1)
	{
		return true;
	}

	securityObj->instBitmapCount = instCount;
	instBytes = (instCount - 1) / 8 + 1;
	if (securityObj->instBitmapBytes < instBytes) {
		if (securityObj->instBitmapBytes != 0 && securityObj->instBitmapPtr != NULL)
		{
			cis_free(securityObj->instBitmapPtr);
		}
		securityObj->instBitmapPtr = (uint8_t*)cis_malloc(instBytes);
		securityObj->instBitmapBytes = instBytes;
	}
	cis_memset(securityObj->instBitmapPtr, 0, instBytes);

	for (instIndex = 0; instIndex < instCount; instIndex++)
	{
		uint16_t instBytePos = (uint16_t)instSecurity->instanceId / 8;
		uint16_t instByteOffset = 7 - (instSecurity->instanceId % 8);
		securityObj->instBitmapPtr[instBytePos] += 0x01 << instByteOffset;
		instSecurity = instSecurity->next;
	}
	return true;
}



void std_security_clean(st_context_t * contextP)
{
	security_instance_t * deleteInst;
	security_instance_t * securityInstance = NULL;
	securityInstance = (security_instance_t *)(contextP->instSecurity);

	while (securityInstance != NULL)
	{
		deleteInst = securityInstance;
		securityInstance = securityInstance->next;

		std_object_remove_securitys(contextP, (cis_list_t*)deleteInst);
		if (NULL != deleteInst->host)
		{
			cis_free(deleteInst->host);
		}
#if CIS_ENABLE_DTLS
		if (NULL != deleteInst->identify)
		{
			cis_free(deleteInst->identify);
		}
		if (NULL != deleteInst->secret)
		{
			cis_free(deleteInst->secret);
		}
		if (NULL != deleteInst->public_key)
		{
			cis_free(deleteInst->public_key);
		}
#endif
		cis_free(deleteInst);
	}
}



char * std_security_get_host(st_context_t * contextP,cis_iid_t InstanceId)
{
    security_instance_t * targetP = prv_security_find(contextP,InstanceId);

    if (NULL != targetP)
    {
        return utils_strdup(targetP->host);
    }

    return NULL;
}

#if CIS_ENABLE_DTLS
#include "dtls.h" //EC add dtls path avoid compile error
#include "dtls_debug.h" //EC add dtls path avoid compile error
#if CIS_ENABLE_PSK
int get_psk_info(dtls_context_t *ctx,
	const session_t *session,
	dtls_credentials_type_t type,
	const unsigned char *id, size_t id_len,
	unsigned char *result, size_t result_length) {

	security_instance_t * targetP = NULL;
	st_context_t * contextP = (st_context_t *)(ctx->app);
	if (contextP->stateStep == PUMP_STATE_BOOTSTRAPPING)
	{
		targetP = prv_security_find(contextP, 1);
		if (NULL == targetP)
		{
			ECOMM_TRACE(UNILOG_ONENET, get_psk_info_1, P_WARNING, 0, "cannot find psk");
			return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
		}

	}
	else
	{
		targetP = prv_security_find(contextP, 0);
		if (NULL == targetP)
		{
			ECOMM_TRACE(UNILOG_ONENET, get_psk_info_2, P_WARNING, 0, "cannot find psk");
			return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
		}
	}
	switch (type) {
	case DTLS_PSK_IDENTITY:
		if (id_len) {
    		ECOMM_STRING(UNILOG_ONENET, get_psk_info_3, P_INFO, "got psk_identity_hint: '%s'", (uint8_t*)id);
		}

		if (result_length < targetP->id_len) {
			ECOMM_TRACE(UNILOG_ONENET, get_psk_info_4, P_WARNING, 0, "cannot set psk_identity -- buffer too small");
			return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
		}

		cis_memcpy(result, targetP->identify, targetP->id_len);
		ECOMM_STRING(UNILOG_ONENET, get_psk_info_5, P_INFO, "got indentity: '%s'", (uint8_t*)result);
		return targetP->id_len;
	case DTLS_PSK_KEY:
		if (id_len != targetP->id_len || cis_memcmp(targetP->identify, id, id_len) != 0) {
			ECOMM_TRACE(UNILOG_ONENET, get_psk_info_6, P_WARNING, 0, "PSK for unknown id requested, exiting");
			return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);
		}
		else if (result_length < targetP->psk_len) {
			ECOMM_TRACE(UNILOG_ONENET, get_psk_info_7, P_WARNING, 0, "cannot set psk -- buffer too small");
			return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
		}

		cis_memcpy(result, targetP->secret, targetP->psk_len);
		ECOMM_STRING(UNILOG_ONENET, get_psk_info_8, P_INFO, "got psk: '%s'", (uint8_t*)result);
		return targetP->psk_len;
	default:
		ECOMM_TRACE(UNILOG_ONENET, get_psk_info_9, P_WARNING, 1, "unsupported request type: %d", type);
	}

	return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
}

void set_psk_info(st_context_t * contextP)
{
	security_instance_t * targetP = NULL;
	targetP = prv_security_find(contextP, 0);
	if (NULL == targetP){
	    ECOMM_TRACE(UNILOG_ONENET, set_psk_info_0, P_INFO, 0,"NULL == targetP !!!");
		return;
	}
	targetP->id_len = strlen(contextP->identify);
	targetP->identify = (char *)cis_malloc(targetP->id_len+1);
	cis_memset(targetP->identify, 0, targetP->id_len+1);
	cis_memcpy(targetP->identify, contextP->identify, targetP->id_len);
	targetP->psk_len = strlen(contextP->secret);
	targetP->secret = (char *)cis_malloc(targetP->psk_len+1);
	cis_memset(targetP->secret, 0, targetP->psk_len+1);
	cis_memcpy(targetP->secret, contextP->secret, targetP->psk_len);
	ECOMM_STRING(UNILOG_ONENET, set_psk_info_1, P_INFO, "psk:%s",(const uint8_t *)contextP->secret);
	ECOMM_STRING(UNILOG_ONENET, set_psk_info_2, P_INFO, "identify:%s",(const uint8_t *)contextP->identify);
}

#endif
#endif /* DTLS_PSK */
