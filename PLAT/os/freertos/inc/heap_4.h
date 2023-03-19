
#ifndef HEAP_4_H
#define HEAP_4_H

void *pvPortMallocEC( size_t xWantedSize, unsigned int funcPtr );
void vPortFree( void *pv );
void *pvPortRealloc( void *pv, size_t xWantedSize );

#define pvPortMalloc(xWantedSize)     pvPortMallocEC((xWantedSize), 0)

#endif

