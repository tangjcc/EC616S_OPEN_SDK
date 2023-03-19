#ifndef __RTT_LOG_H__
#define __RTT_LOG_H__

#include <SEGGER_RTT_Conf.h>
#include <SEGGER_RTT.h>

#define LOG_TERMINAL_NORMAL         (0)
#define LOG_TERMINAL_ERROR          (1)
#define LOG_TERMINAL_INPUT          (0)

#define LOG_CTRL_RESET         "\x1B[0m"         // Reset to default colors
#define LOG_CTRL_CLEAR         "\x1B[2J"         // Clear screen, reposition cursor to top left

#define LOG_COLOR_DEFAULT  "\x1B[0m"
#define LOG_COLOR_BLACK    "\x1B[1;30m"
#define LOG_COLOR_RED      "\x1B[1;31m"
#define LOG_COLOR_GREEN    "\x1B[1;32m"
#define LOG_COLOR_YELLOW   "\x1B[1;33m"
#define LOG_COLOR_BLUE     "\x1B[1;34m"
#define LOG_COLOR_MAGENTA  "\x1B[1;35m"
#define LOG_COLOR_CYAN     "\x1B[1;36m"
#define LOG_COLOR_WHITE    "\x1B[1;37m"

#define rtt_log(...)             {SEGGER_RTT_printf(LOG_TERMINAL_NORMAL, ##__VA_ARGS__);}


uint32_t log_rtt_init(void);

#endif

