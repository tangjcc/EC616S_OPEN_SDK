#ifndef _LOG_H_

#define _LOG_H_

#include <stdio.h>
#include <stdarg.h>
#include <time.h>


#define LOG_INFO "INFO"
#define LOG_DEBUG "DEBUG"
#define LOG_ERROR "ERROR"

#define LOG_FILE_PATH_PREFIX "./"

#define LOG_FILE __FILE__
#define LOG_FUNC __func__
#define LOG_LINE __LINE__

#define OUT_FILE_SIZE 1024

#define FSEEK_START 0
#define FSEEK_END 2

typedef enum {
	INFO_LEVEL , 
	DEBUG_LEVEL ,
	ERROR_LEVEL
}LOG_LEVEL;

typedef enum {
	LOG_SUCCESS = 0 , 
	FILE_PATH_ERROR = -404 , 
	LEVEL_NAME_NULL = -405
}LOG_STATUS;

LOG_STATUS log_print(int level , const char* logFile , const char* logFunc , int logLine , const char* format , ...);
void log_print_plain_text (char* format , ...);
void log_print_array(char* array , int len);
void log_print_mark_begin (void);
void log_print_mark_end(void);

#endif
