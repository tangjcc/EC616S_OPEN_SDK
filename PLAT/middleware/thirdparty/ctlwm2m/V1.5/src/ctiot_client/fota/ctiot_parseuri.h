#ifndef _CTIOT_PARSE_URI_H
#define _CTIOT_PARSE_URI_H

#include <stdbool.h>

typedef struct
{
    char protocol[6];
    char address[51];
    int port;
    char uri[201];
}CTIOT_URI;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

bool ctiot_funcv1_parse_url(char* url,CTIOT_URI* uri);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_CTIOT_PARSE_URI_H
