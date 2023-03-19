#ifndef _ADUPS_SYSTEM_API_H
#define _ADUPS_SYSTEM_API_H
#include "abup_typedef.h"
#ifdef __cplusplus
extern "C" {
#endif

extern void abup_system_reboot(void);
void abup_init_value_change(lwm2m_context_t * lwm2m);
#ifdef LWM2M_CLIENT_MODE
void * abup_lwm2m_connect_server(uint16_t secObjInstID, void * userData);
void abup_lwm2m_close_connection(void * sessionH, void * userData);
char * abup_get_server_uri(lwm2m_object_t * objectP, uint16_t secObjInstID);
#endif

#ifdef __cplusplus
}
#endif

#endif
