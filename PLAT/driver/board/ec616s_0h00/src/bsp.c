/****************************************************************************
 *
 * Copy right:   2018 Copyrigths of EigenComm Ltd.
 * File name:    bsp.c
 * Description:
 * History:
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "clock_ec616s.h"
#include "bsp.h"
#include "system_ec616s.h"
#include "debug_log.h"
#include "hal_uart.h"
#include "mem_map.h"

extern ARM_DRIVER_USART Driver_USART0;
extern ARM_DRIVER_USART Driver_USART1;

ARM_DRIVER_USART *UsartPrintHandle = NULL;
ARM_DRIVER_USART *UsartUnilogHandle = NULL;
ARM_DRIVER_USART *UsartAtCmdHandle = NULL;

static uint8_t OSState = 0;     // OSState = 0 os not start, OSState = 1 os started

__WEAK void pmuInit(void){};
__WEAK void uniLogFlushOut(uint32_t flush_data){};

#ifdef UINILOG_FEATURE_ENABLE
#define UNILOG_RECV_BUFFER_SIZE       (96)
static uint8_t unilog_uart_recv_buf[UNILOG_RECV_BUFFER_SIZE];
#endif

void BSP_SetPrintUart(usart_port_t port)
{
    if(port == PORT_USART_0)
    {
#if (RTE_UART0)
        UsartPrintHandle = &CREATE_SYMBOL(Driver_USART, 0);
#endif
    }
    else if(port == PORT_USART_1)
    {
#if (RTE_UART1)
        UsartPrintHandle = &CREATE_SYMBOL(Driver_USART, 1);
#endif
    }

    if (UsartPrintHandle == NULL)
        return;
    else
    {
        UsartPrintHandle->Initialize(NULL);
        UsartPrintHandle->PowerControl(ARM_POWER_FULL);
        UsartPrintHandle->Control(ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 |
                            ARM_USART_PARITY_NONE       | ARM_USART_STOP_BITS_1 |
                            ARM_USART_FLOW_CONTROL_NONE, 115200ul);

    }
}

#if defined ( __GNUC__ )

/*
 *  retarget for _write implementation
 *  Parameter:      ch: character will be out
 */
int io_putchar(int ch)
{
    if (UsartPrintHandle != NULL)
        UsartPrintHandle->Send((uint8_t*)&ch, 1);
    return 0;
}

/*
 *  retarget for _read implementation
 *  Parameter:      ch: character will be read
 */
int io_getchar()
{
    uint8_t ch = 0;
    if (UsartPrintHandle != NULL)
        UsartPrintHandle->Receive(&ch, 1);
    return (ch);
}

__attribute__((weak,noreturn))
void __aeabi_assert (const char *expr, const char *file, int line) {
  printf("Assert, expr:%s, file: %s, line: %d\r\n", expr, file, line);
  while(1);
}

#elif defined (__CC_ARM)

/*
 *  retarget for printf implementation
 *  Parameter:      ch: character will be out
 *                  f:  not used
 */
int fputc(int ch, FILE *f)
{
    if (UsartPrintHandle != NULL)
        UsartPrintHandle->Send((uint8_t*)&ch,1);
    return 0;
}

/*
 *  retarget for scanf implementation
 *  Parameter:      f:  not used
 */
int fgetc(FILE *f)
{
    uint8_t ch = 0;
    if (UsartPrintHandle != NULL)
        UsartPrintHandle->Receive(&ch,1);
    return (ch);
}

__attribute__((weak,noreturn))
void __aeabi_assert (const char *expr, const char *file, int line) {
  printf("Assert, expr:%s, file: %s, line: %d\r\n", expr, file, line);
  while(1);
}

#endif

/*
 *  cpu cycles delay, every loop spends 2 cpu cycles, so the actual delay is 2*cycles
 *  Parameter: cycles
 */
#if defined(__CC_ARM)
__asm static void delay_cycles(uint32_t cycles)
{
loop
    SUBS r0, r0, #1
    BNE loop
    BX lr
}
#elif defined (__GNUC__)
static void delay_cycles(uint32_t cycles)
{
asm volatile(
    "mov r0, %0\n\t"
    "loop:\n\t"
    "SUBS r0, r0, #1\n\t"
    "BNE loop\n\t"
    : : "r" (cycles)
);
}
#endif

void delay_us(uint32_t us)
{
    uint32_t ticks;

    // cpu frequency ranges from 26MHz to 204.8MHz
    // first divide 0.1M to get rid of multiply overflow
    // considering 2 cpu cycles are taken to execute one loop operation(sub and branch) in delay_cycles function,
    // and 0.1M in first step, so we shall divide another 20 before passing the result to delay_cycles function
    ticks = SystemCoreClock / 100000U;
    ticks = ticks * us / 20U;

    delay_cycles(ticks);
}

/*
 *  Reference: http://src.gnu-darwin.org/src/lib/libc/string/strnstr.c.html
 */
char* ec_strnstr(const char *s, const char *find, size_t slen)
{
    char c, sc;
    size_t len;

    if((c = *find++) != '\0')
    {
        len = strlen(find);

        do
        {
            do
            {
                if((sc = *s++) == '\0' || slen-- < 1)
                    return NULL;
            } while(sc != c);

            if(len > slen)
                return NULL;

        } while(strncmp(s, find, len) != 0);

        s--;
    }

    return (char*)s;
}

void setOSState(uint8_t state)
{
    OSState = state;
}

PLAT_CODE_IN_RAM uint8_t getOSState(void)		//1 os started. 0 no OS or OS not started yet
{
    return OSState;
}


uint8_t* getBuildInfo(void)
{
    return (uint8_t *)BSP_HEADER;
}

uint8_t* getVersionInfo(void)
{
    return (uint8_t *)VERSION_INFO;
}

//move here since this is an common and opensource place
uint8_t* getDebugDVersion(void)
{
    return DB_VERSION_UNIQ_ID;
}


//used to get high tempreture threshold for protecting. should not inline(may called in lib)
__attribute__ ((noinline)) uint8_t getHighTempThreshold(void)

{
    return HIGH_TEMPR_HIB_THR;
}

//used to get SDK_MAJOR_VERSION. should not inline(may called in lib)
__attribute__ ((noinline)) uint8_t * getSdkMajorVerion(void)

{
    return SDK_MAJOR_VERSION;
}

__attribute__ ((noinline)) uint32_t getBatMonResetMode(void)
{
    return 0;
}

#ifdef UINILOG_FEATURE_ENABLE
/**
  \fn           void logToolCommandHandle(uint8_t *atcmd_buffer, uint32_t len)
  \brief        handle downlink command sent from unilog tool EPAT
                if need to handle more command in future, add command-handler table
  \param[in]    event         UART event, note used in this function
  \param[in]    cmd_buffer    command received from UART
  \param[in]    len           command length
  \returns      void
*/
void logToolCommandHandle(uint32_t event, uint8_t *cmd_buffer, uint32_t len)
{
    (void)event;

    uint8_t * LogDbVserion=getDebugDVersion();

    if(ec_strnstr((const char *)cmd_buffer, "^logversion", len))
    {

        ECOMM_STRING(UNILOG_PLA_INTERNAL_CMD, get_log_version, P_SIG, "LOGVERSION:%s",LogDbVserion);

    }
    else
    {

        ECOMM_STRING(UNILOG_PLA_STRING, get_log_version_1, P_ERROR, "%s", "invalid command from EPAT");

    }

    return;

}

uint8_t* getUnilogRecvBuffer(void)
{
    return unilog_uart_recv_buf;
}


/*
 *  set unilog uart port
 *  Parameter:      port: for unilog
 *  Parameter:      baudrate: uart baudrate
 */
void SetUnilogUart(usart_port_t port, uint32_t baudrate, bool startRecv)
{
    hal_uart_config_t halUartConfig = {0};

    hal_uart_hardware_config_t hwConfig = {
                                            ARM_POWER_FULL,
                                            ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 |
                                            ARM_USART_PARITY_NONE       | ARM_USART_STOP_BITS_1 |
                                            ARM_USART_FLOW_CONTROL_NONE,
                                            baudrate
                                           };

    if (port == PORT_USART_0)
    {
#if (RTE_UART0)
        UsartUnilogHandle = &CREATE_SYMBOL(Driver_USART, 0);
#endif
    } else if (port == PORT_USART_1)
    {
#if (RTE_UART1)
        UsartUnilogHandle = &CREATE_SYMBOL(Driver_USART, 1);
#endif
    }

    if (UsartUnilogHandle == NULL)
        return;

    halUartConfig.uartDriverHandler = UsartUnilogHandle;
    halUartConfig.recv_cb = (hal_uart_recv_cb_t)logToolCommandHandle;
    halUartConfig.recvBuffPtr = unilog_uart_recv_buf;
    halUartConfig.recvBuffSize = UNILOG_RECV_BUFFER_SIZE;

    if(startRecv)
    {
        HAL_UART_InitHandler(port, &halUartConfig, &hwConfig, HAL_UART_TASK_CREATE_FLAG_RECV);
    }
    else
    {
        HAL_UART_InitHandler(port, &halUartConfig, &hwConfig, HAL_UART_TASK_CREATE_FLAG_NONE);
    }
}
#endif

void FlushUnilogOutput(uint32_t flush_data)
{
    uniLogFlushOut(flush_data);

    if(UsartUnilogHandle == NULL)
        return;
    UsartUnilogHandle->Control(ARM_USART_CONTROL_FLUSH_TX, NULL);
}


uint32_t GET_PMU_RAWFLASH_OFFSET(void)
{
	return FLASH_MEM_BACKUP_ADDR;
}


void BSP_CommonInit(void)
{
    SystemCoreClockUpdate();

    CLOCK_DriverInit();

    PAD_DriverInit();

    GPR_Initialize();

    pmuInit();

    //interrupt config
    IC_PowupInit();
}

