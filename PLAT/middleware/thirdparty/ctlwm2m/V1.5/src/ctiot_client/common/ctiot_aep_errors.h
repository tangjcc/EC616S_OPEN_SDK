#ifndef _CTIOT_AEP_ERRORE_H
#define _CTIOT_AEP_ERRORE_H
typedef enum
{
    CTIOT_COAP_SUCCESS=0,                  //成功
    CTIOT_COAP_PARAM_OTHER=1,              //其它错误
    CTIOT_COAP_PARAM_NUMS=2,               //参数数量错误
    CTIOT_COAP_PARAM_VALUE=3,              //参数值错误
    CTIOT_COAP_NOT_LOGINED=4,              //终端未登录
    CTIOT_COAP_PLATFORM_DISABLED=7,        //平台Disable终端期间，无法响应
    CTIOT_COAP_NOT_INITLIZED=12,           //IOT平台连接参数未初始化
    CTIOT_COAP_DATALEN_NOT_EVEN=13,        //Data字段长度不是偶数
    CTIOT_COAP_DATALEN_OVERFLOW=14,        //Data字段长度超过上限
    CTIOT_COAP_PLATFORM_NOT_READY=15,      //平台未准备好接收数据
    CTIOT_COAP_NO_IMSI=16,                 //无法获取IMSI (如未插SIM卡)

    CTIOT_COAP_CONNECTING=32,              //终端与平台连接中，不能执行此操作（可选，LWM2M Engine根据自身情况确定是否返回）
    CTIOT_COAP_LOIGINED=33,                //终端已登录（可选，参见“处理说明”中的相关描述）
    CTIOT_COAP_CREATESESSION_FAIL          //创建LWM2M会话失败（如IP通道未建立导致的失败等）
}CTIOT_COAP_STATUS;
typedef enum
{
    CTIOT_COAP_REG_SUCCESS,                //登录成功
    CTIOT_COAP_REG_TIMEOUT,                //超时无响应
    CTIOT_COAP_REG_SENDFAIL,               //未发出报文
    CTIOT_COAP_REG_ERROR_ENDPOINTNAME=10,  //Endpoint Name无法识别或参数错误（平台返回 4.00 Bad Request）
    CTIOT_COAP_REG_AUTHORIZE_FAIL=13,      //鉴权失败，Server拒绝接入（平台返回 4.03 Forbidden）
    CTIOT_COAP_REG_PROTOCOL_NOT_SUPPORT=22 //IOT Protocol或LWM2M版本不支持（平台返回 4.12 Precondition Failed）
}CTIOT_COAP_REG_STATUS;
typedef enum 
{
    CTIOT_COAP_UB_SUCCESS,                  //更新成功（平台返回2.04 Changed）
    CTIOT_COAP_UB_TIMEOUT,                  //超时无响应
    CTIOT_COAP_UB_SENDFAIL,                 //未发出报文
    CTIOT_COAP_UB_ERROR_PARAM=10,           //参数错误（平台返回4.00 Bad Request）
    CTIOT_COAP_UB_AUTHORIZE_FAIL=13,        //鉴权失败，Server拒绝接入（平台返回 4.03 Forbidden）
    CTIOT_COAP_UB_NOT_LOGINED               //终端未登录（URI错误）（平台返回4.04 Not Found）
}CTIOT_COAP_UB_STATUS;
typedef enum
{
    CTIOT_COAP_UPDATE_ERROR_PARAM=10,        //参数错误（平台返回4.00 Bad Request）
    CTIOT_COAP_UPDATE_AUTHORIZE_FAIL=13,     //鉴权失败，Server拒绝接入（平台返回 4.03 Forbidden）
    CTIOT_COAP_UPDATE_NOT_LOGINED            //终端未登录（URI错误）（平台返回4.04 Not Found）
}CTIOT_COAP_UPDATE_STATUS;
typedef enum
{
    CTIOT_COAP_DEREG_SUCCESS=0,              //取消登录成功（平台返回2.02 Deleted）
    CTIOT_COAP_DEREG_TIMEOUT,                //超时无响应
    CTIOT_COAP_DEREG_SENDFAIL,               //未发出报文
    CTIOT_COAP_DEREG_UNKNOWN_REASON=10,      //未明原因导致的失败（平台返回4.00 Bad Request）
    CTIOT_COAP_DEREG_AUTHORIZE_FAIL=13,      //鉴权失败，Server拒绝接入（平台返回 4.03 Forbidden）
    CTIOT_COAP_DEREG_NOT_LOGINED             //终端未登录（URI错误）（平台返回4.04 Not Found）
}CTIOT_COAP_DEREG_STATUS;
typedef enum
{
    CTIOT_COAP_SEND_SUCCESS,                 //上报平台成功
    CTIOT_COAP_SEND_TIMEOUT,                 //超时无响应
    CTIOT_COAP_SEND_SENDFAIL,                //未发出报文
    CTIOT_COAP_SEND_PLATFORM_ERROR = 9       //平台不能处理上报数据（收到平台应答的RST报文, 需要注意的是，无论AT+CTM2MSEND采用CON还是NON模式，平台都可能应答RST。）
}CTIOT_COAP_SEND_STATUS;
typedef enum
{
    FOTA_SUCCESS=1,
    FOTA_STATUS_2,
    FOTA_STATUS_3,
    FOTA_STATUS_4,
    FOTA_STATUS_5,
    FOTA_STATUS_6,
    FOTA_STATUS_7,
    FOTA_STATUS_8,
    FOTA_STATUS_9
}CTIOT_COAP_FOTA_STATUS;
typedef enum
{
    CTIOT_COAP_RECVRSP_SUCCESS,              //上报平台成功
    CTIOT_COAP_RECVRSP_TIMEOUT,              //超时无响应
    CTIOT_COAP_RECVRSP_SENDFAIL,             //未发出报文
    CTIOT_COAP_RECVRSP_PLATFORM_ERROR = 9    //平台不能处理上报数据（收到平台应答的RST报文, 需要注意的是，无论AT+CTM2MRECVRSP采用CON还是NON模式，平台都可能应答RST。）
}CTIOT_COAP_RECVRSP_STATUS;
typedef enum
{
    CTIOT_COAP_PSMNOTIFY_QUIT                //TAU Timer到期导致NB连接退出PSM态
}CTIOT_COAP_PSMNOTIFY_STATUS;
typedef enum
{
    CTIOT_COAP_LWSTATUS_SESSION_FAIL         //lwm2m session已经失效（当前lwm2m session不再能正常收发数据了）
}CTIOT_COAP_LWSTATUS_STATUS;

typedef enum
{
    COAP_STATUS,
    REG_STATUS,
    UB_STATUS,
    UPDATE_STATUS,
    DEREG_STATUS,
    SEND_STATUS,
    RECVRSP_STATUS,
    PSMNOTIFY_STATUS,
    LWSTATUS_STATUS
}CTIOT_STATUS_TYPE;
typedef struct{
    CTIOT_STATUS_TYPE statusType;
    union
    {
        CTIOT_COAP_STATUS coapStatus;
        CTIOT_COAP_REG_STATUS regStatus;
        CTIOT_COAP_UB_STATUS ubStatus;
        CTIOT_COAP_UPDATE_STATUS updateStatus;
        CTIOT_COAP_DEREG_STATUS deregStatus;
        CTIOT_COAP_SEND_STATUS sendStatus;
        CTIOT_COAP_RECVRSP_STATUS recvrspStatus;
        CTIOT_COAP_PSMNOTIFY_STATUS psmnotifyStatus;
        CTIOT_COAP_LWSTATUS_STATUS lwStatus;
    }status;
}CTIOT_STATUS;
extern CTIOT_STATUS globalCoapStatus;
CTIOT_STATUS ctiot_coap_get_status();
void coap_set_status(CTIOT_STATUS* pStatus,CTIOT_STATUS status );
void ctiot_coap_set_status(CTIOT_STATUS status);
void ctiot_coap_clear_error();
#endif //_CTIOT_AEP_ERRORE_H
