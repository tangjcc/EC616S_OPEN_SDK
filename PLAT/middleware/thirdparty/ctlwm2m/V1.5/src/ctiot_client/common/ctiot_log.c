#include <string.h>
#include <stdlib.h>
#include "ctiot_log.h"
#include "ct_liblwm2m.h"

char* strcatex(char* str1, char* str2) {
    char* buf = (char*)ct_lwm2m_malloc(strlen(str1) + strlen(str2) + 1);
    sprintf(buf , "%s" , str1);
    strcat(buf , str2);
    return buf;
}


int check_level(int level) {
    
    if (level != INFO_LEVEL && level != DEBUG_LEVEL && level != ERROR_LEVEL ) {
        level = INFO_LEVEL;
    }

    return level;
}

char* set_level_name(int level) {

    switch (level) {
        case INFO_LEVEL :
            #ifdef LOG_INFO
                return LOG_INFO;
            #else
                goto exit;
            #endif
        case DEBUG_LEVEL :
            #ifdef LOG_DEBUG
                return LOG_DEBUG;
            #else
                goto exit;
            #endif
        case ERROR_LEVEL :
            #ifdef LOG_ERROR
                return LOG_ERROR;
            #else
                goto exit;
            #endif
        default:
            return NULL;
    }
#ifndef LOG_DEBUG
 exit:
        return NULL;
#endif
}

#if defined (FILE_PRINT) || defined (FILE_AND_SOUT_PRINT)
#include "pthread.h"
struct tm* newtime;

pthread_mutex_t logMut = PTHREAD_MUTEX_INITIALIZER;

LOG_STATUS log_file_print(int level , const char* logFile , const char* logFunc , int logLine , const char* format , va_list args) {
#ifdef WITH_CTIOT_LOGS

    LOG_STATUS result = LOG_SUCCESS;
    
    char* levelName = set_level_name(level);

    if (levelName == NULL) {
        result = LEVEL_NAME_NULL;
        printf("[ERROR][log_file_print]:LevelName is null,please check it.\n");
        goto exit;
    }

    char out[OUT_FILE_SIZE];
    time_t t1;
    t1 = time(NULL);
    newtime=localtime(&t1);

    char* logFilePathPrefix = LOG_FILE_PATH_PREFIX;
    char* logFilePathSuffix = "data_%Y%m%d.log";
    char* logFileFullPath = strcatex(logFilePathPrefix , logFilePathSuffix);
    
    strftime(out , OUT_FILE_SIZE , logFileFullPath , newtime);
    FILE* fp = fopen(out , "a+");

    if (NULL == fp) {
        result = FILE_PATH_ERROR;
        printf("[ERROR][log_file_print]:File path directory can not exist,please check it.\n");
        goto exit;
    }
    
    //指到文件最后
    fseek(fp , FSEEK_START , FSEEK_END);

    pthread_mutex_lock(&logMut);
    if (logFile != NULL) {
        fprintf(fp , "[%s][%s][%s][%s %d]>\n" , __TIME__ , levelName , logFile , logFunc , logLine);
    }
    vfprintf (fp, format, args);
    va_end (args);
    fclose(fp);
    
    pthread_mutex_unlock(&logMut);
exit:
    return result;
#endif  
}
#endif

#if defined (SOUT_PRINT) || defined (FILE_AND_SOUT_PRINT)
void log_stdout_print(int level , const char* logFile , const char* logFunc , int logLine , const char* format , va_list args) {
#ifdef WITH_CTIOT_LOGS

    char* levelName = set_level_name(level);
    if (levelName == NULL) {
        printf("levelName is NULL!\n");
        va_end (args);
        return;
    }

    if (logFile != NULL) {
        fprintf(stdout , "[%s][%s][%s][%s %d]>\n" , __TIME__ , levelName , logFile , logFunc , logLine);
    }

    char out[OUT_FILE_SIZE] = {0};

    vsprintf (out, format, args);
    va_end (args);
    printf("%s" , out);
#endif  
}
#endif

LOG_STATUS log_print_common(int level , const char* logFile , const char* logFunc , int logLine , const char* format , va_list args) {
    LOG_STATUS result = LOG_SUCCESS;
#ifdef WITH_CTIOT_LOGS
    #ifdef FILE_PRINT
        return log_file_print(level , logFile , logFunc , logLine , format , args); 
    #endif

    #ifdef SOUT_PRINT
        log_stdout_print(level , logFile , logFunc , logLine , format , args);
    #endif

    va_end (args);
exit:
    return result;
#else
    return result;
#endif
}

LOG_STATUS log_print(int level , const char* logFile , const char* logFunc , int logLine , const char* format , ...) {
   LOG_STATUS result = LOG_SUCCESS;

#ifdef WITH_CTIOT_LOGS
   va_list args;
   va_start (args, format);
   level = check_level(level);
   result = log_print_common(level , logFile , logFunc , logLine , format , args);
   if (result != LOG_SUCCESS) {
       goto exit;
   }

   #ifdef FILE_AND_SOUT_PRINT
       result = log_file_print(level , logFile , logFunc , logLine , format , args);
       if (result != LOG_SUCCESS) {
           goto exit;
       }
       va_start(args , format);
       log_stdout_print(level , logFile , logFunc , logLine , format , args);
   #endif

exit:
   return result;
#else
   return result;
#endif
}

void log_print_mark_begin() {
#ifdef WITH_CTIOT_LOGS

    LOG_STATUS result = LOG_SUCCESS;
    result = log_print(0 , NULL , NULL , 0 , "\n");
    if (result != LOG_SUCCESS) {
        goto exit;
    }

    char begin[101] = {0};
    int cursor = 0; 
    for(; cursor < 100 ; cursor++) {
        begin[cursor] = '^';
    }
    begin[100] = '\0';
    log_print(0 , NULL , NULL , 0 , "%s\n" , begin);
exit:
    return;
#endif
}

void log_print_mark_end() {
#ifdef WITH_CTIOT_LOGS

    LOG_STATUS result = LOG_SUCCESS;
    int cursor = 0;
    result = log_print(0 , NULL , NULL , 0 , "\n");
    if (result != LOG_SUCCESS) {
        goto exit;
    }
    char end[101] = {0};
    for(; cursor < 100 ; cursor++) {
        end[cursor] = 'v';
    }
    end[100] = '\0';
    log_print(0 , NULL , NULL , 0 , "%s\n" , end);
exit:
    return;
#endif
}

void log_print_array(char* array , int len) {
#ifdef WITH_CTIOT_LOGS

    LOG_STATUS result = LOG_SUCCESS;

    int cursor = 0;
    
    for( ; cursor < len ; cursor++)
    {
        result = log_print(0 , NULL , NULL , 0 , "%02x" , array[cursor]);

        if (result != LOG_SUCCESS) {
            goto exit;
        }
    }

    log_print(0 , NULL , NULL , 0 , "\n");
exit:
    return;
#endif
}

void log_print_plain_text (char* format , ...) {

#ifdef WITH_CTIOT_LOGS
   LOG_STATUS result = LOG_SUCCESS;
   
   va_list args;
   va_start (args, format);
   
   result = log_print_common(0, NULL , NULL , 0 , format , args);
   if (result != LOG_SUCCESS) {
       goto exit;
   }

   #ifdef FILE_AND_SOUT_PRINT
       log_file_print(0, NULL , NULL , 0 , format , args);
       if (result != LOG_SUCCESS) {
           goto exit;
       }
       va_start(args , format);
       log_stdout_print(0, NULL , NULL , 0 , format , args);
   #endif
exit:
   return;
 #endif
}
