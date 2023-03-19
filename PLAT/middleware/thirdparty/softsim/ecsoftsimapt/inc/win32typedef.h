#ifndef  _WIN32_TYPEDEF_H_
#define  _WIN32_TYPEDEF_H_


#include <basetsd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INT8
typedef signed char             INT8;
#endif
#ifndef UINT8
typedef unsigned char           UINT8;
#endif

#ifndef INT16
typedef signed short            INT16;
#endif

#ifndef  UINT16
typedef unsigned short          UINT16;
#endif



#ifndef INT64
typedef __int64                 INT64;
#endif

#ifndef UINT64
typedef unsigned __int64        UINT64;
#endif


#ifndef CHAR
typedef char                    CHAR;
#endif

#ifdef __cplusplus
}
#endif

#endif   //end  _WIN32_TYPEDEF_H_
