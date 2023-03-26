/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    ct_api.h
 * Description:  EC616S ocean connect demo ct api header file
 * History:      Rev1.0   2020-03-19
 *
 ****************************************************************************/
#ifndef _CT_API_H_
#define _CT_API_H_

#define CT_MSG_TIMEOUT                1000

typedef struct
{
     uint8_t status_type;
}CT_STATUS_Q_MSG;

typedef struct
{
     uint8_t cmd_type;
}APP_CMD_Q_MSG;

typedef enum
{
    APP_INIT_STATE,
    APP_DEACTIVE_STATE,
    APP_CHECK_IPREADY_FROM_WAKE,
    APP_IPREADY_STATE,
    APP_WAITING_ONLINE_STATE,
    APP_OC_REG_SUCCESS_STATE,
    APP_OC_OBV_SUB_STATE,
    APP_OC_UPDATE_SUCCESS_STATE,
    APP_SENDING_PACKET_STATE,
    APP_OC_HEART_BEAT_ARRIVED,
    APP_OC_HEART_BEAT_UPDATE,
    APP_OC_HEART_BEAT_SEND,
    APP_OC_CONTEXT_RECOVER,
    APP_WAIT_APP_ACT,
    APP_CT_OBSV_RESP,
    APP_CT_REQUEST_RECV,
    APP_IDLE_STATE
} appStateMachine_t;

typedef enum
{
    APP_READY_TO_SEND_PACKET,
    APP_SEND_PACKET_DONE,
    APP_SEND_PACKET_FAILED,
    APP_UPDATE_FAILED,
    APP_RECV_RST_CMD,
    APP_NETWORK_DISC,
    APP_OC_OBSV_FAILED,
    APP_PARAM_ERROR,
    APP_INTER_ERROR,
    APP_OC_REG_FAILED,
    APP_GPS_RECEIVED
} appRunningState_t;

typedef enum
{
    APP_CMD_DEREG_REG,
    APP_CMD_REG_AGAIN,
    APP_CMD_REINIT_NETWORK,
    APP_CMD_TO_IDLE
} appCmdType_t;

typedef enum
{
    KEY_NOT_PRESS,
    KEY_PRESS,
    KEY_RESPONSING
} keyState_t;


typedef enum
{
    SYS_SLEEP,
    SYS_NOT_SLEEP
} sleepState_t;

void ctApiInit(void);
void appTimerExpFunc(uint8_t id);

#endif

