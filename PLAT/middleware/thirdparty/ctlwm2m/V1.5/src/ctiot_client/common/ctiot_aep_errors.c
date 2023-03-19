
#if 0
#include "ctiot_aep_errors.h"
CTIOT_STATUS globalCoapStatus;
CTIOT_STATUS ctiot_funcv1_coap_get_status()
{
    return globalCoapStatus;
}

void coap_set_status(CTIOT_STATUS* pStatus,CTIOT_STATUS status )
{

    pStatus->statusType=status.statusType;
    switch(pStatus->statusType)
    {
        case COAP_STATUS:
        {
            pStatus->status.coapStatus=status.status.coapStatus;
            break;
        }
        case REG_STATUS:
        {
            pStatus->status.regStatus=status.status.regStatus;
            break;
        }
        case UB_STATUS:
        {
            pStatus->status.ubStatus=status.status.ubStatus;
            break;
        }
        case UPDATE_STATUS:
        {
            pStatus->status.updateStatus=status.status.updateStatus;
            break;
        }
        case DEREG_STATUS:
        {
            pStatus->status.deregStatus=status.status.deregStatus;
            break;
        }
        case SEND_STATUS:
        {
            pStatus->status.sendStatus=status.status.sendStatus;
            break;
        }case RECVRSP_STATUS:
        {
            pStatus->status.recvrspStatus=status.status.recvrspStatus;
            break;
        }
        case PSMNOTIFY_STATUS:
        {
            pStatus->status.psmnotifyStatus=status.status.psmnotifyStatus;
            break;
        }
        case LWSTATUS_STATUS:
        {
            pStatus->status.lwstatus=status.status.lwStatus;
            break;
        }
        default:
        {
            pStatus->statusType=COAP_STATUS;
            pStatus->status.coapStatus=CTIOT_COAP_SUCCESS;
        }
    }
}

void ctiot_funcv1_coap_set_status(CTIOT_STATUS status)
{
    coap_set_status(&globalCoapStatus,status);
}
void ctiot_funcv1_coap_clear_error()
{

    globalCoapStatus.statusType=COAP_STATUS;
    globalCoapStatus.status.coapStatus=CTIOT_COAP_SUCCESS;
}
#endif
