#ifndef __L2CTLS_SHA_H__
#define __L2CTLS_SHA_H__
#include "mbedtls/sha256.h"
#include "mbedtls/sha1.h"
#include "mbedtls/aes.h"

#include "l2ctls_ec616.h"

typedef enum {
    HAL_SHA_STATUS_OK = 0, 
    HAL_SHA_STATUS_ERROR = 1
}HalShaStatusType;

typedef enum {
    HAL_AES_STATUS_OK = 0, 
    HAL_AES_STATUS_ERROR = 1
}HalAesStatusType;

typedef mbedtls_sha256_context HalSha256Context;
typedef mbedtls_sha1_context HalSha1Context;

typedef mbedtls_aes_context HalAesContext;
typedef L2CTlsAesStruct HalAesStruct;

HalShaStatusType HalSha256Start(HalSha256Context *context);
HalShaStatusType  HalSha256Append(HalSha256Context *context, const uint8_t *message, uint32_t length, uint32_t LastFlag);
HalShaStatusType  HalSha256End(HalSha256Context *context, uint8_t digest_message[32]);

#define HAL_MAX_APPEND_LENGTH 1000
#define ResidueMaxSize  64  

#endif
