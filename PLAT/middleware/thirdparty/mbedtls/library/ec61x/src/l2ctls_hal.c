#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "ec616.h"
#elif defined CHIP_EC616S
#include "ec616s.h"
#endif

#include "string.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_SHA256_HW_ADD) || defined(MBEDTLS_AES_ALT)
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "l2ctls_ec616.h"
#elif defined CHIP_EC616S
#include "l2ctls_ec616s.h"
#endif



#include "l2ctls_hal.h"

#define TLS_POLL_MODE 0
#define TLS_IRQ_MODE 1
#define TLS_MODE TLS_IRQ_MODE

static int8_t LockState;
static int8_t InitState;

static int32_t L2CTlsLock(void);
static  void L2CTlsUnLock(void);

#define L2CTLSDebug 0
static int32_t L2CTlsLock(void)
{
    uint32_t mask;
    int32_t Ret;
    mask = SaveAndSetIRQMask();

    if (LockState == 0)
    {
        LockState = 1;
        Ret = 0;
    }
    else
    {
        Ret = 1;
    }
    
    RestoreIRQMask(mask);
    return Ret;
}

static void L2CTlsUnLock(void)
{
    LockState = 0;
}

static void L2CHalInit(void)
{
    if (InitState == 0)
    {
        InitState = 1;
        L2CTlsInit();
    }
}

#if defined(MBEDTLS_SHA256_HW_ADD)
HalShaStatusType HalSha256Start(HalSha256Context *context)
{
    int32_t RetValue;

    
    if (NULL == context)
    {
        //TBD LOG ERROR
        return HAL_SHA_STATUS_ERROR;
    }    

    RetValue = L2CTlsLock();
    if (RetValue!=0)
    {
        return HAL_SHA_STATUS_ERROR;
    }
    
    L2CHalInit();
    
    if (context->is224)
    {
        L2CShaComInit(L2C_SHA_TYPE_224);        
    }
    else
    {
        L2CShaComInit(L2C_SHA_TYPE_256);
    }
    
    L2CTlsUnLock();

    return HAL_SHA_STATUS_OK;
}

HalShaStatusType HalSha256Append(HalSha256Context *context, const uint8_t *message, uint32_t length, uint32_t LastFlag)
{
    int32_t RetValue;

    
    if ((NULL == context) || (NULL == message) ||(length >=0x10000))
    {
        //TBD LOG ERROR
        return HAL_SHA_STATUS_ERROR;
    }
    
    RetValue = L2CTlsLock();
    if (RetValue!=0)
    {
        return HAL_SHA_STATUS_ERROR;
    }
    RetValue = L2CTlsCheckMode(L2C_SHA_MODE, (context->is224)? L2C_SHA_TYPE_224: L2C_SHA_TYPE_256);
    if (RetValue != 0)
    {
        L2CTlsUnLock();            
        return HAL_SHA_STATUS_ERROR;
    }
    
    while (length>0) {
        if (length >= HAL_MAX_APPEND_LENGTH) {
            RetValue  = L2CShaUpdate((uint32_t)message, (uint32_t)(&context->state[0]), HAL_MAX_APPEND_LENGTH, LastFlag);
            if (RetValue != 0)
            {
                break;
            }
            message += HAL_MAX_APPEND_LENGTH;
            length -= HAL_MAX_APPEND_LENGTH;
        } else {
            RetValue = L2CShaUpdate((uint32_t)message, (uint32_t)(&context->state[0]), length, LastFlag);
            break;
        }
    }

    L2CTlsUnLock();        
    
    if (RetValue != 0)
    {
        return HAL_SHA_STATUS_ERROR;
    }
    else
    {
        return HAL_SHA_STATUS_OK;
    }
}

HalShaStatusType  HalSha256End(HalSha256Context *context, uint8_t digest_message[32])
{
    int32_t RetValue;
    uint32_t ResidueDataBufLen = 0;
#define ResidueMaxSize  64  
    
    RetValue = L2CTlsLock();
    if (RetValue!=0)
    {
        return HAL_SHA_STATUS_ERROR;
    }
    
    if (context->total[0]>0)  {
        ResidueDataBufLen = context->total[0]%ResidueMaxSize;
        if (ResidueDataBufLen == 0)  { ResidueDataBufLen = ResidueMaxSize;}
    }   
    
    RetValue = L2CTlsCheckMode(L2C_SHA_MODE, (context->is224)? L2C_SHA_TYPE_224: L2C_SHA_TYPE_256);
    if (RetValue != 0)
    {
        L2CTlsUnLock();                    
        return HAL_SHA_STATUS_ERROR;
    }
    RetValue = L2CShaUpdate((uint32_t)(context->buffer), (uint32_t)(&context->state[0]), ResidueDataBufLen, 1);
    L2CTlsUnLock();        
    
    if (RetValue != 0)
    {
        return HAL_SHA_STATUS_ERROR;
    }

    if (context->is224 ==0)
    {
        memcpy(digest_message, &context->state[0], 32);    
    }
    else
    {
        memcpy(digest_message, &context->state[0], 28);
    }
    return HAL_SHA_STATUS_OK;    
}
#endif

#if defined(MBEDTLS_AES_ALT)
HalAesStatusType HalAesEncrypt(HalAesStruct *AesInfoPtr)
{
    int32_t RetValue;
    RetValue = L2CTlsLock();
    if (RetValue!=0)
    {
        return HAL_AES_STATUS_ERROR;
    }

    L2CHalInit();
    
    RetValue = L2CTlsCheckMode(L2C_AES_MODE, (L2CSHAType)0);
    if (RetValue != 0)
    {
        L2CTlsUnLock();                    
        return HAL_AES_STATUS_ERROR;
    }
    
    RetValue = L2CTlsAesProcess(AesInfoPtr);
    
    L2CTlsUnLock();        
    if (RetValue!=0)
    {
        return HAL_AES_STATUS_ERROR;
    }
    return HAL_AES_STATUS_OK;
}
#endif

#endif
