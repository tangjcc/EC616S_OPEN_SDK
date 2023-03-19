#include "stdio.h"
#include "string.h"

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "uart_ec616.h"
#elif defined CHIP_EC616S
#include "uart_ec616s.h"
#endif

#include "debug_log.h"
#include "mem_map.h"

const uint8_t dump_handshake_code[]= "enter assert dump mode";
const uint8_t dump_response_code[]= "ok";
const uint8_t PreambleDumpString[4] = {0x44, 0x55, 0x4d, 0x50};


#define DUMP_UART_INSTANCE    0
#define DUMP_SYNC_RSP_LEN 8
//#define DUMP_SYNC_RSP_LEN 2

#define UartFifoLen 16
#define CMD_FIX_LEN 8
#define CMD_FCS_LEN 4

#define PROTOCOL_RSP_FIX_LEN 8
#define PREAMBLE_CNT 2
#define PREAMBLE_STRING_LEN 4
#define PREAMBLE_WITH_NULL 1
#define MAX_CMD_DATALEN 32 //maximum data size for cmd
#define MAX_READ_DATALEN (48*1024) //maximum data size for cmd

#define MAX_RETRY_COUNT 32
#define DATA_DUMP_WAIT_SYNC_MAX_RETRY_COUNT    10
#define DATA_DUMP_GET_CMD_MAX_RETRY_COUNT      100
#define DUMP_CID 0xdc
#define N_DUMP_CID 0x23



#define ACK 0
#define NACK 1


#define GetDataCmd 0x20
#define GetInfoCmd 0x21

#define FinishCmd 0x25


#define WaitPeriod_1s 1000000

typedef struct {
    uint32_t ReadDataAddr;
    uint32_t ReadLen;
}ReadDataReqCell;

typedef struct {
    uint8_t Command;
    uint8_t Sequence;
    uint8_t CID;
    uint8_t NCID;
    uint16_t Status;
    uint16_t Length;//Length for Data filed
    //uint8_t Data[MAX_CMD_DATALEN];
    uint32_t FCS;
}DumpRspWrap, *PtrDumpRspWrap;

typedef struct {
    uint8_t Command;
    uint8_t Sequence;
    uint8_t CID;
    uint8_t NCID;
    uint32_t Length;//Length for Data filed
    uint8_t Data[MAX_CMD_DATALEN];
    uint32_t FCS;
}DumpReqWrap, *PtrDumpReqWrap;


extern void delay_us(uint32_t us);

const uint16_t wCRCTableAbs[] =
{
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400,
};

const uint32_t tDataInfoCell [][2]=
{
    {EC_EXCEPTION_RAM_BASE, EC_RAM_TOTAL_SIZE},
};

static uint16_t CRC16(uint16_t wCRC, uint8_t *pchMsg, uint16_t wDataLen)
{
    //uint16_t wCRC = 0xFFFF;
    uint16_t i;
    uint8_t chChar;
    for (i = 0; i < wDataLen; i++)
    {
        chChar = *pchMsg++;
        wCRC = wCRCTableAbs[(chChar^wCRC)&15]^(wCRC>>4);
        wCRC = wCRCTableAbs[((chChar >>4)^ wCRC)&15]^(wCRC>>4);
    }
    return wCRC;
}

uint32_t VerifyPreamble(uint8_t *Preamble)
{
    uint32_t idx;
    uint32_t CheckCnt = 0;
    for (idx = 0; idx < sizeof(PreambleDumpString); idx++)
    {
        if (PreambleDumpString[idx] == Preamble[idx]) {
            CheckCnt++;
        }
    }
    if (CheckCnt>1)
    {
        return TRUE;
    }
    return FALSE;
}

uint32_t EcDumpWaitSync(void)
{
    uint8_t idx = 0;
    uint8_t RecChar[UartFifoLen];
    uint8_t RetValue = 0;
    UART_Flush(DUMP_UART_INSTANCE);

    for (idx = 0; idx < PREAMBLE_CNT; idx++)
    {
        RetValue = UART_Send(DUMP_UART_INSTANCE, &PreambleDumpString[0], PREAMBLE_STRING_LEN, 1000);
        if (RetValue != PREAMBLE_STRING_LEN)
        {
         return 1;
        }
    }
    RetValue = UART_Send(DUMP_UART_INSTANCE, "\r\n", 2, 1000);
    if (RetValue != 2)
    {
     return 1;
    }

    //Host send Preamble twice
    RetValue = UART_Receive(DUMP_UART_INSTANCE, &RecChar[0], PREAMBLE_CNT*PREAMBLE_STRING_LEN + PREAMBLE_WITH_NULL, 200000);

    if(RetValue > 0)
    {
        UART_Send(DUMP_UART_INSTANCE, &RecChar[0], RetValue, 1000);
    }

    if (RetValue != (PREAMBLE_CNT*PREAMBLE_STRING_LEN+PREAMBLE_WITH_NULL))
    {
        return 1;
    }

    if (VerifyPreamble(&RecChar[0]) ||VerifyPreamble(&RecChar[4]))
    {
        return 0;
    }
    return 1;
}

uint32_t EcDumpWaitCmd(PtrDumpReqWrap ptrDumpReqWrap)
{
    uint32_t RetValue = 0;

    RetValue = UART_Receive(DUMP_UART_INSTANCE, (uint8_t *)ptrDumpReqWrap, CMD_FIX_LEN, 10000);
    if (RetValue != CMD_FIX_LEN)
    {
        return 1;
    }

    if (ptrDumpReqWrap->Length > MAX_CMD_DATALEN)
    {
        ptrDumpReqWrap->Length = MAX_CMD_DATALEN;
    }

    RetValue = UART_Receive(DUMP_UART_INSTANCE, (uint8_t *)(&ptrDumpReqWrap->Data[0]), ptrDumpReqWrap->Length, 10000);
    if (RetValue != ptrDumpReqWrap->Length)
    {
        return 1;
    }

    RetValue = UART_Receive(DUMP_UART_INSTANCE, (uint8_t *)(&ptrDumpReqWrap->FCS), CMD_FCS_LEN, 10000);
    if (RetValue != CMD_FCS_LEN)
    {
        return 1;
    }

    return 0;
}

uint32_t EcDumpCheckCmd(PtrDumpReqWrap ptrDumpReqWrap)
{
    uint32_t sum = 0;
    uint8_t * pStartAddr;


    if ((ptrDumpReqWrap->CID != DUMP_CID) ||(ptrDumpReqWrap->NCID != N_DUMP_CID))
    {
        return 1;
    }
    pStartAddr = (uint8_t*) (&ptrDumpReqWrap->Command);

    sum  = CRC16(0xFFFF, pStartAddr, CMD_FIX_LEN + ptrDumpReqWrap->Length);
    if (sum != ptrDumpReqWrap->FCS)
    {
        //return 0;//test now , no check sum
        return 1;
    }

    return 0;
}

uint32_t EcDumpHandleGetData(PtrDumpReqWrap ptrDumpReqWrap)
{
    uint32_t RetValue = 0;
    DumpRspWrap tDumpRspWrap;
    ReadDataReqCell * ptrReadDataReqCell;
    uint32_t Sum = 0xFFFF;
    uint32_t Idx;
    uint32_t DataBuff;

    ptrReadDataReqCell = (ReadDataReqCell*)(&ptrDumpReqWrap->Data[0]);
    tDumpRspWrap.Command = ptrDumpReqWrap->Command;
    tDumpRspWrap.Sequence = ptrDumpReqWrap->Sequence;
    tDumpRspWrap.CID = ptrDumpReqWrap->CID;
    tDumpRspWrap.NCID = ptrDumpReqWrap->NCID;
    if ((ptrReadDataReqCell->ReadLen > MAX_READ_DATALEN) || ((ptrReadDataReqCell->ReadDataAddr%4)!=0))
    {
        tDumpRspWrap.Length = 0;
        tDumpRspWrap.Status = NACK;
        Sum = CRC16(0xFFFF, &tDumpRspWrap.Command, PROTOCOL_RSP_FIX_LEN+tDumpRspWrap.Length);
        tDumpRspWrap.FCS = Sum;
        RetValue = UART_Send(DUMP_UART_INSTANCE, &tDumpRspWrap.Command, PROTOCOL_RSP_FIX_LEN+tDumpRspWrap.Length + CMD_FCS_LEN, 1000);
        return RetValue;
    }

    tDumpRspWrap.Status = ACK;
    tDumpRspWrap.Length = ptrReadDataReqCell->ReadLen;


    RetValue = UART_Send(DUMP_UART_INSTANCE, &tDumpRspWrap.Command, PROTOCOL_RSP_FIX_LEN, 1000);
    if (RetValue != PROTOCOL_RSP_FIX_LEN)
    {
        return 1;
    }
    Sum = CRC16(Sum, &tDumpRspWrap.Command,PROTOCOL_RSP_FIX_LEN);
    for (Idx = 0; Idx < ptrReadDataReqCell->ReadLen; Idx+=4)
    {
        *(uint32_t *) (&DataBuff) =  *(uint32_t *)(ptrReadDataReqCell->ReadDataAddr + Idx);

        RetValue = UART_Send(DUMP_UART_INSTANCE, (uint8_t*)(&DataBuff), sizeof(DataBuff), 1000);
        if (RetValue != sizeof(DataBuff))
        {
            return 1;
        }

        Sum = CRC16(Sum, (uint8_t*)(&DataBuff),sizeof(DataBuff));
    }

    tDumpRspWrap.FCS = Sum;
    RetValue = UART_Send(DUMP_UART_INSTANCE, (uint8_t *)(&tDumpRspWrap.FCS), CMD_FCS_LEN, 1000);
    if (RetValue != CMD_FCS_LEN)
    {
        return 1;
    }
    return RetValue;
}

uint32_t EcDumpHandleGetInfo(PtrDumpReqWrap ptrDumpReqWrap)
{
    uint32_t RetValue = 0;
    DumpRspWrap tDumpRspWrap;
    uint32_t Sum = 0xFFFF;

    tDumpRspWrap.Command = ptrDumpReqWrap->Command;
    tDumpRspWrap.Sequence = ptrDumpReqWrap->Sequence;
    tDumpRspWrap.CID = ptrDumpReqWrap->CID;
    tDumpRspWrap.NCID = ptrDumpReqWrap->NCID;

    tDumpRspWrap.Status = ACK;
    tDumpRspWrap.Length = sizeof(tDataInfoCell);

    RetValue = UART_Send(DUMP_UART_INSTANCE, &tDumpRspWrap.Command, PROTOCOL_RSP_FIX_LEN, 1000);
    if (RetValue != PROTOCOL_RSP_FIX_LEN)
    {
        return 1;
    }
    Sum = CRC16(Sum, &tDumpRspWrap.Command,PROTOCOL_RSP_FIX_LEN);

    RetValue = UART_Send(DUMP_UART_INSTANCE, (uint8_t *)&tDataInfoCell, sizeof(tDataInfoCell), 1000);
    if (RetValue != sizeof(tDataInfoCell))
    {
        return 1;
    }
    Sum = CRC16(Sum, (uint8_t *)&tDataInfoCell, sizeof(tDataInfoCell));

    tDumpRspWrap.FCS = Sum;
    RetValue = UART_Send(DUMP_UART_INSTANCE, (uint8_t *)(&tDumpRspWrap.FCS), CMD_FCS_LEN, 1000);
    if (RetValue != CMD_FCS_LEN)
    {
        return 1;
    }

    return 0;
}

uint32_t EcDumpDataFlow(void)
{
    uint32_t retryCount = DATA_DUMP_WAIT_SYNC_MAX_RETRY_COUNT;
    uint32_t RetValue = 0;
    DumpReqWrap tDumpReqWrap;
    UART_Init(DUMP_UART_INSTANCE, 921600, false);
    delay_us(WaitPeriod_1s);

    do
    {
        RetValue = EcDumpWaitSync();
        delay_us(WaitPeriod_1s);
    }while((RetValue != 0) && (retryCount--));

    retryCount = DATA_DUMP_GET_CMD_MAX_RETRY_COUNT;

    while(retryCount)
    {
        RetValue = EcDumpWaitCmd(&tDumpReqWrap);
        if (RetValue != 0)
        {
            retryCount--;
            continue;
        }
        RetValue = EcDumpCheckCmd(&tDumpReqWrap);
        if (RetValue != 0)
        {
            continue;
        }

        retryCount = DATA_DUMP_GET_CMD_MAX_RETRY_COUNT;

        switch(tDumpReqWrap.Command)
        {
            case GetDataCmd:
                EcDumpHandleGetData(&tDumpReqWrap);
                break;
            case GetInfoCmd:
                EcDumpHandleGetInfo(&tDumpReqWrap);
                break;
            case FinishCmd:
                return 0;
            default:
                break;

        }
    }
    return RetValue;
}

uint32_t EcDumpHandshakeProc(uint32_t SyncPeriod)
{
    uint8_t recv_buffer[DUMP_SYNC_RSP_LEN];
    uint32_t SyncCnt = 0;
    uint32_t RetValue = 0;
    uint32_t idx;

    UART_PurgeRx(DUMP_UART_INSTANCE);

    memset(recv_buffer, 0, sizeof(recv_buffer));

    while(SyncCnt++ < MAX_RETRY_COUNT)
    {
        ECOMM_TRACE(UNILOG_EXCEP_PRINT, ec_assert_start, P_ERROR, 0, "enter assert dump mode");

        RetValue = UART_Receive(DUMP_UART_INSTANCE, recv_buffer, DUMP_SYNC_RSP_LEN, SyncPeriod);

        if (RetValue >= 2)
        {
            for (idx = 0; idx < DUMP_SYNC_RSP_LEN - 2; idx++)
            {
                if ((recv_buffer[idx] == dump_response_code[0]) && (recv_buffer[idx+1] == dump_response_code[1]))
                {
                    return 0;
                }
            }
        }
    }

    return 1;
}


uint32_t EcDumpTopFlow(void)
{
    uint32_t RetValue = 0;
    RetValue = EcDumpHandshakeProc(WaitPeriod_1s>>1);

    if (RetValue==0)
    {
        EcDumpDataFlow();
        return 0;
    }

    return RetValue;
}

