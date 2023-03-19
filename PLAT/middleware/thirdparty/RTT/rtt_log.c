#include <stdint.h>
#include <stdbool.h>
#include "rtt_log.h"


static char buf_normal_up[1024];
static char buf_down[128];

uint32_t log_rtt_init(void)
{
    static bool initialized = false;
    if (initialized)
    {
        return false;
    }

    if (SEGGER_RTT_ConfigUpBuffer(LOG_TERMINAL_NORMAL,
                                  "Normal",
                                  buf_normal_up,
                                  BUFFER_SIZE_UP,
                                  SEGGER_RTT_MODE_NO_BLOCK_TRIM
                                 )
        != 0)
    {
        return false;
    }

    if (SEGGER_RTT_ConfigDownBuffer(LOG_TERMINAL_INPUT,
                                   "Input",
                                   buf_down,
                                   BUFFER_SIZE_DOWN,
                                   SEGGER_RTT_MODE_NO_BLOCK_SKIP
                                  )
        != 0)
    {
        return false;
    }

    initialized = true;

    return true;
}



//end
