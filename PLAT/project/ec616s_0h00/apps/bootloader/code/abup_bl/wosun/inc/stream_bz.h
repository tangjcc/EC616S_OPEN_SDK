#ifndef _STREAM_BZ_H
#define _STREAM_BZ_H

#include "bzlib.h"

typedef struct patch_stream
{
	void* opaque;
	int (*read)(struct patch_stream* stream, void* buffer, int length);
}ad_bz_stream;

extern void *stream_init_bz_decoder(ML_PEER_FILE fp);
extern int stream_bz_read(void *stream, ML_U8* buf, int len); 
extern void stream_bz_end(void *strm);

#endif  // _STREAM_BZ_H

