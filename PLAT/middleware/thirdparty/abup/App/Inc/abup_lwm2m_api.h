#ifndef _ADUPS_LWM2M_API_H
#define _ADUPS_LWM2M_API_H
#include "abup_typedef.h"
#include "liblwm2m.h"
#ifdef ABUP_WITH_TINYDTLS
#include "shared/dtlsconnection.h"
#else
#include "shared/connection.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _abup_security_instance_
{
    struct _abup_security_instance_ * next;        // matches lwm2m_list_t::next
    uint16_t                     instanceId;  // matches lwm2m_list_t::id
    char *                       uri;
    bool                         isBootstrap;    
    uint8_t                      securityMode;
    char *                       publicIdentity;
    uint16_t                     publicIdLen;
    char *                       serverPublicKey;
    uint16_t                     serverPublicKeyLen;
    char *                       secretKey;
    uint16_t                     secretKeyLen;
    uint8_t                      smsSecurityMode;
    char *                       smsParams; // SMS binding key parameters
    uint16_t                     smsParamsLen;
    char *                       smsSecret; // SMS binding secret key
    uint16_t                     smsSecretLen;
    uint16_t                     shortID;
    uint32_t                     clientHoldOffTime;
    uint32_t                     bootstrapServerAccountTimeout;
} abup_security_instance_t;


#ifdef LWM2M_CLIENT_MODE
// Returns a session handle that MUST uniquely identify a peer.
// secObjInstID: ID of the Securty Object instance to open a connection to
// userData: parameter to abup_lwm2m_init()
void * abup_lwm2m_connect_server(uint16_t secObjInstID, void * userData);
// Close a session created by lwm2m_connect_server()
// sessionH: session handle identifying the peer (opaque to the core)
// userData: parameter to abup_lwm2m_init()
void abup_lwm2m_close_connection(void * sessionH, void * userData);

char * abup_get_server_uri(lwm2m_object_t * objectP, uint16_t secObjInstID);
#endif

#ifdef __cplusplus
}
#endif

#endif
