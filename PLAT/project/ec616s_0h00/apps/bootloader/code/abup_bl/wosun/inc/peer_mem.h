#ifndef _PEER_MEMORY_H
#define _PEER_MEMORY_H


#ifdef __cplusplus 
extern "C" {
#endif
	
void * test_malloc_ptr(int size,	
						  char *file,		
						  int line);
void * test_remalloc_ptr(void* ptr_addr, int nsize,	
						  char *file,			
						  int line)	;
void test_free_ptr(void *ptr);

int start_memory_test( void );
void stop_memory_test( void );



#ifdef __cplusplus 
}
#endif
	
#endif  // _PEER_MEMORY_H

