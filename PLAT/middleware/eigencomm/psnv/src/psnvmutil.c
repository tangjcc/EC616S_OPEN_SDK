/******************************************************************************
 ******************************************************************************
 Copyright:      - 2017- Copyrights of EigenComm Ltd.
 File name:      - psnvmutil.c
 Description:    - Protocol Stack NVM basic utility
 History:        - 13/09/2017, Originated by jcweng
 ******************************************************************************
******************************************************************************/
#include "psnvmutil.h"
#include "psnvm.h"

#undef  PS_NVM_WRITE_ON_TIME
#define PS_NVM_WRITE_ON_TIME    0

/******************************************************************************
 ******************************************************************************
 !!!
  1> API is NOT safe if called in multi tasks;
  2> A NVM file should only read/write in one task
 !!!
 ******************************************************************************
******************************************************************************/

/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
extern const PsNvmFileOper psNvmFileOperList[];

//#define PS_NVM_FILE_NUM (sizeof(psNvmFileOperList)/sizeof(PsNvmFileOper))

/******************************************************************************
 * As NVM can't be written on flash on time (during normal running time)
 * If any NVM need to updated, store the NVM into the local here;
 * And when power off (or before enter hibernated state), write to flash at that time
 *
 * UINT8   *pNvmFile;
 *  if NVM need to update, store here
 * Ponter to:
 * +------------------+------------------+
 * | PsNvmFileHeader  |    NVM body      |
 * +------------------+------------------+
******************************************************************************/
UINT8   *pNvmFile[PS_MAX_NVM];


/******************************************************************************
 ******************************************************************************
 * INERNAL/STATIC FUNCTION
 ******************************************************************************
******************************************************************************/

#if 0
/******************************************************************************
 * PsNvmCalcCrcValue
 * Description: Calculate the "CRC" value
 * input: UINT8 *dataBuf
 *        UINT16 bufSize
 * output: UINT8; // CRC value
 * Comment:
******************************************************************************/
static UINT8 PsNvmCalcCrcValue(const UINT8 *dataBuf, UINT16 bufSize)
{
    UINT8   crcValue    = 0;
    UINT16  tmpIdx      = 0;
    GosCheck(dataBuf != PNULL && bufSize > 0, dataBuf, bufSize, 0);

    while (tmpIdx < bufSize)
    {
        crcValue = (crcValue >> (tmpIdx&0x03)) ^ (dataBuf[tmpIdx]);
        tmpIdx++;
    }

    return crcValue;
}
#endif

/******************************************************************************
 * PsNvmCalcCrcValue
 * Description: Calculate the "CRC" value
 * input: UINT8 *dataBuf
 *        UINT16 bufSize
 * output: UINT8; // CRC value
 * Comment:
******************************************************************************/
static UINT8 PsNvmCalcCrcValue(const UINT8 *dataBuf, UINT16 bufSize)
{
    UINT32 i;
    UINT32 a = 1, b = 0;

    GosCheck(dataBuf != PNULL && bufSize > 0, dataBuf, bufSize, 0);

    for (i = bufSize; i > 0; )
    {
        a += (UINT32)(dataBuf[--i]);
        b += a;
    }

    return (UINT8)(((a>>24)&0xFF)^((a>>16)&0xFF)^((a>>8)&0xFF)^((a)&0xFF)^
                   ((b>>24)&0xFF)^((b>>16)&0xFF)^((b>>8)&0xFF)^((b)&0xFF)^
                   (bufSize&0xFF));
}


/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/

/******************************************************************************
 * PsNvmRead
 * Description: read NVM file, if not exist, just set to the default value
 * input: PsNvmFileIdEnum fileId; // which NVM file need to read
 *        void *dataBuf; // output buffer
 *        UINT16 bufSize; //output buffer size
 * output: BOOL
 * Comment:
******************************************************************************/
BOOL PsNvmRead(PsNvmFileIdEnum fileId, void *dataBuf, UINT16 bufSize)
{
    const PsNvmFileOper *fileOper = PNULL;
    GOSFILE fp = PNULL;
    PsNvmFileHeader fileHeader;
    UINT32 readSize = 0;
    BOOL   needAdjust = FALSE, adjustOK = FALSE;
    void   *readBodyBuf = dataBuf;
    UINT16 readBodyLen = bufSize;
    PsNvmFileHeader *pLocalHdr = PNULL; //if NVM updated before
    UINT8   crcValue = 0;

    GosCheck(fileId < PS_MAX_NVM && dataBuf != PNULL, fileId, dataBuf, PS_MAX_NVM);

    fileOper = &(psNvmFileOperList[fileId]);

    GosCheck(fileOper->fileId == fileId, fileOper->fileId, fileId, bufSize);

    if (fileOper->fileSize != bufSize)
    {
        //GosPrintf(P_ERROR, "PS NVM, NVM file: %d, file size(%d) != input buffer size(%d), can't read NVM",
        //          fileId, fileOper->fileSize, bufSize);

        ECOMM_TRACE(UNILOG_PS, PsNvmRead_1, P_ERROR, 3,
                   "PS NVM, NVM file: %d, file size(%d) != input buffer size(%d), can't read NVM",
                   fileId, fileOper->fileSize, bufSize);

        GosDebugBegin(FALSE, fileId, fileOper->fileSize, bufSize);
        GosDebugEnd();

        return FALSE;
    }

    /*
     * check whether the NVM read and updated before
    */
    if (pNvmFile[fileId] != PNULL)
    {
        pLocalHdr = (PsNvmFileHeader *)(pNvmFile[fileId]);

        if (pLocalHdr->version == fileOper->curVersion &&
            pLocalHdr->fileBodySize == bufSize &&
            strncmp((const CHAR *)(pLocalHdr->fName), (const CHAR *)(fileOper->fileName), strlen((const CHAR *)(fileOper->fileName))) == 0)
        {
            // same NVM file
            memcpy(dataBuf, (pNvmFile[fileId] + sizeof(PsNvmFileHeader)), bufSize);
            return TRUE;
        }
        else
        {
            GosDebugBegin(FALSE, fileId, pLocalHdr->version, pLocalHdr->fileBodySize);
            GosDebugEnd();

            GosFreeMemory(&(pNvmFile[fileId])); // free the old NVM buffer, and read from NVM again
        }
    }

    /*
     * open the NVM file
    */
    fp = GosFopen(fileOper->fileId, fileOper->fileName, "rb");
    if (fp == PNULL)
    {
        //GosPrintf(P_ERROR, "PS NVM, can't open file ID: %d, maybe not exist", fileId);

        ECOMM_TRACE(UNILOG_PS, PsNvmRead_2, P_ERROR, 1,
                   "PS NVM, can't open file ID: %d, maybe not exist, try to use the defult value", fileId);

        /*
         * try to use the default value
        */
        if (fileOper->setDefaultFunc != PNULL)
        {
            (*(fileOper->setDefaultFunc))(dataBuf, bufSize);

            /*
             * write and save to NVM
            */
            PsNvmWrite(fileId, dataBuf, bufSize);
            PsNvmSave(fileId);

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    /*
     * read the header
    */
    memset(&fileHeader, 0x00, sizeof(PsNvmFileHeader));
    readSize = GosFread(&fileHeader, sizeof(PsNvmFileHeader), 1, fp);
    if (readSize != 1 ||
        strncmp((const CHAR *)(fileHeader.fName), (const CHAR *)(fileOper->fileName), strlen((const CHAR *)(fileOper->fileName))) != 0)
    {
        // header read failed
        //GosPrintf(P_ERROR, "PS NVM, file ID: %d, can't read the file header, or filer header not right: %d",
        //          fileId, readSize);

        ECOMM_TRACE(UNILOG_PS, PsNvmRead_3, P_ERROR, 2,
                   "PS NVM, file ID: %d, can't read the file header, or filer header not right: %d, try to use the defult value",
                   fileId, readSize);

        //close the file
        GosFclose(fp);
        /*
         * try to use the default value
        */
        if (fileOper->setDefaultFunc != PNULL)
        {
            (*(fileOper->setDefaultFunc))(dataBuf, bufSize);
            /*
             * write and save to NVM
            */
            PsNvmWrite(fileId, dataBuf, bufSize);
            PsNvmSave(fileId);

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    if (fileHeader.version != fileOper->curVersion)
    {
        //GosPrintf(P_WARNING, "PS NVM, file: %.32s, NVM file version: %d, but current version: %d",
        //          fileOper->fileName, fileHeader.version, fileOper->curVersion);

        ECOMM_TRACE(UNILOG_PS, PsNvmRead_4, P_WARNING, 3,
                   "PS NVM, file ID: %d, NVM file version: %d, but current version: %d",
                   fileId, fileHeader.version, fileOper->curVersion);

        if (fileOper->adjustVerFunc == PNULL && fileOper->setDefaultFunc == PNULL)
        {
            //GosPrintf(P_WARNING, "PS NVM, NVM file version not latest, but adjustVerFunc & setDefaultFunc are all PNULL, read fail");

            ECOMM_TRACE(UNILOG_PS, PsNvmRead_5, P_WARNING, 0,
                       "PS NVM, NVM file version not latest, but adjustVerFunc & setDefaultFunc are all PNULL, read fail");

            //close the file
            GosFclose(fp);
            return FALSE;
        }

        if (fileHeader.fileBodySize <= 1024)    //CUR NVM file size should be < 1K
        {
            needAdjust = TRUE;
            readBodyBuf = GosAllocMemory(fileHeader.fileBodySize);
            GosCheck(readBodyBuf != PNULL, fileHeader.fileBodySize, fileHeader.version, 0);
            readBodyLen = fileHeader.fileBodySize;
        }
        else
        {
            GosDebugBegin(FALSE, fileHeader.fileBodySize, fileId, fileHeader.version);
            GosDebugEnd();
        }
    }
    else
    {
        if (fileHeader.fileBodySize != bufSize)
        {
            //GosPrintf(P_WARNING, "PS NVM, file Id: %d, file version is latest, but file size not right: (%d)/(%d)",
            //          fileId, fileHeader.fileBodySize, bufSize);

            ECOMM_TRACE(UNILOG_PS, PsNvmRead_6, P_WARNING, 3,
                       "PS NVM, file ID: %d, file version is latest, but file size not right: (%d)/(%d), try to use the defult value",
                       fileId, fileHeader.fileBodySize, bufSize);

            //close the file
            GosFclose(fp);

            /*
             * try to use the default value
            */
            if (fileOper->setDefaultFunc != PNULL)
            {
                (*(fileOper->setDefaultFunc))(dataBuf, bufSize);
                /*
                 * write and save to NVM
                */
                PsNvmWrite(fileId, dataBuf, bufSize);
                PsNvmSave(fileId);

                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }

        //else: (fileHeader.fileBodySize == bufSize)
    }

    /*
     * read the file body
    */
    readSize = GosFread(readBodyBuf, readBodyLen, 1, fp);
    if (readSize != 1)
    {
        // header read failed
        //GosPrintf(P_ERROR, "PS NVM, file ID: %d, can't read the file body", fileId);

        ECOMM_TRACE(UNILOG_PS, PsNvmRead_7, P_WARNING, 1,
                   "PS NVM, file ID: %d, can't read the file body, try to use the defult value", fileId);

        // free the read buf
        if (needAdjust)
        {
            GosFreeMemory(&readBodyBuf);
        }

        //close the file
        GosFclose(fp);

        /*
         * try to use the default value
        */
        if (fileOper->setDefaultFunc != PNULL)
        {
            (*(fileOper->setDefaultFunc))(dataBuf, bufSize);
            /*
             * write and save to NVM
            */
            PsNvmWrite(fileId, dataBuf, bufSize);
            PsNvmSave(fileId);

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    /*
     * need to CRC check the body value
    */
    crcValue = PsNvmCalcCrcValue(readBodyBuf, readBodyLen);
    if (crcValue != fileHeader.checkSum)
    {
        GosDebugBegin(FALSE, fileHeader.checkSum, crcValue, fileId);
        GosDebugEnd();

        ECOMM_TRACE(UNILOG_PS, PsNvmRead_8, P_WARNING, 3,
                   "PS NVM, file ID: %d, CRC check failed: %d/%d, just set to default value",
                   fileId, crcValue, fileHeader.checkSum);

        // free the read buf
        if (needAdjust)
        {
            GosFreeMemory(&readBodyBuf);
        }

        //close the file
        GosFclose(fp);

        /*
         * try to use the default value
        */
        if (fileOper->setDefaultFunc != PNULL)
        {
            (*(fileOper->setDefaultFunc))(dataBuf, bufSize);
            /*
             * write and save to NVM
            */
            PsNvmWrite(fileId, dataBuf, bufSize);
            PsNvmSave(fileId);

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }


    /*
     * CRC OK
     * if need to do VERSION adjustment
    */
    if (needAdjust)
    {
        if (fileOper->adjustVerFunc != PNULL)
        {
            adjustOK = (*(fileOper->adjustVerFunc))(fileHeader.version, readBodyBuf, readBodyLen, dataBuf, bufSize);
        }

        if (adjustOK != TRUE)
        {
            ECOMM_TRACE(UNILOG_PS, PsNvmRead_9, P_WARNING, 3,
                       "PS NVM, file: %d, old version (%d) can't convert to cur version (%d), just set to default value",
                       fileId, fileHeader.version, fileOper->curVersion);

            (*(fileOper->setDefaultFunc))(dataBuf, bufSize);
        }

        // free the read buf
        //if (needAdjust)
        //{
        GosFreeMemory(&readBodyBuf);
        //}

        //close the file
        GosFclose(fp);

        // WRITE and save to NVM
        PsNvmWrite(fileId, dataBuf, bufSize);
        PsNvmSave(fileId);

        return TRUE;
    }

    //close the file
    GosFclose(fp);
    return TRUE;
}



#if (PS_NVM_WRITE_ON_TIME == 0)
/******************************************************************************
 * PsNvmWrite
 * Description: WRITE the new NVM file
 * input: PsNvmFileIdEnum fileId; // which NVM file need to read
 *        void *dataBuf;
 *        UINT16 bufSize;
 * output: BOOL
 * Comment:
******************************************************************************/
BOOL PsNvmWrite(PsNvmFileIdEnum fileId, void *dataBuf, UINT16 bufSize)
{
    const PsNvmFileOper *fileOper = PNULL;
    PsNvmFileHeader *pFileHeader = PNULL;

    GosCheck(fileId < PS_MAX_NVM && dataBuf != PNULL, fileId, dataBuf, PS_MAX_NVM);

    fileOper = &(psNvmFileOperList[fileId]);

    GosCheck(fileOper->fileId == fileId, fileOper->fileId, fileId, bufSize);

    if (fileOper->fileSize != bufSize)
    {
        GosDebugBegin(FALSE, fileId, fileOper->fileSize, bufSize);
        GosDebugEnd();

        //GosPrintf(P_ERROR, "PS NVM, NVM file ID: %d, file size(%d) > input buffer size(%d), can't write NVM",
        //          fileId, fileOper->fileSize, bufSize);

        ECOMM_TRACE(UNILOG_PS, PsNvmWrite_1, P_ERROR, 3,
                   "PS NVM, NVM file ID: %d, file size(%d) != input buffer size(%d), can't write NVM", fileId, fileOper->fileSize, bufSize);

        return FALSE;
    }

    /*************************
     * As falsh can't be written during running (cost much time),
     * here, save the NVM in local, and before power off (or enter hibernate state), flush to FLASH at that time
     ************************/
    if (pNvmFile[fileId] != PNULL)
    {
        //already update before
        pFileHeader = (PsNvmFileHeader *)(pNvmFile[fileId]);

        if (pFileHeader->version != fileOper->curVersion ||
            pFileHeader->fileBodySize != bufSize ||
            strncmp((const CHAR *)(pFileHeader->fName), (const CHAR *)(fileOper->fileName), strlen((const CHAR *)(fileOper->fileName))) != 0)
        {
            GosDebugBegin(FALSE, pFileHeader->version, pFileHeader->fileBodySize, fileId);
            GosDebugEnd();

            GosFreeMemory(&(pNvmFile[fileId]));

            pNvmFile[fileId] = PNULL;
        }
    }

    if (pNvmFile[fileId] == PNULL)
    {
        // allocate the dynmaic memory
        pNvmFile[fileId] = (UINT8 *)GosAllocZeroMemory(sizeof(PsNvmFileHeader) + bufSize);

        GosDebugBegin(pNvmFile[fileId] != PNULL, sizeof(PsNvmFileHeader), bufSize, fileId);
        return FALSE;
        GosDebugEnd();
    }


    /*
     * write the header
    */
    pFileHeader = (PsNvmFileHeader *)(pNvmFile[fileId]);
    strncpy((CHAR *)(pFileHeader->fName), (const CHAR *)(fileOper->fileName), strlen((const CHAR *)(fileOper->fileName)));
    pFileHeader->fileBodySize = bufSize;
    pFileHeader->version = fileOper->curVersion;
    pFileHeader->checkSum = PsNvmCalcCrcValue(dataBuf, bufSize);

    /*
     * write the body
    */
    memcpy(pNvmFile[fileId]+sizeof(PsNvmFileHeader), dataBuf, bufSize);

    return TRUE;
}

#else

/******************************************************************************
 * PsNvmWrite
 * Description: WRITE the new NVM file
 * input: PsNvmFileIdEnum fileId; // which NVM file need to read
 *        void *dataBuf;
 *        UINT16 bufSize;
 * output: BOOL
 * Comment:
******************************************************************************/
BOOL PsNvmWrite(PsNvmFileIdEnum fileId, void *dataBuf, UINT16 bufSize)
{
    const PsNvmFileOper *fileOper = PNULL;
    GOSFILE fp = PNULL;
    PsNvmFileHeader *pFileHeader = PNULL;
    UINT32  writeSize = 0;
    UINT8   *nvmFileBuf = PNULL;

    GosCheck(fileId < PS_MAX_NVM && dataBuf != PNULL, fileId, dataBuf, PS_MAX_NVM);

    fileOper = &(psNvmFileOperList[fileId]);

    GosCheck(fileOper->fileId == fileId, fileOper->fileId, fileId, bufSize);

    if (fileOper->fileSize != bufSize)
    {
        GosDebugBegin(FALSE, fileId, fileOper->fileSize, bufSize);
        GosDebugEnd();

        //GosPrintf(P_ERROR, "PS NVM, NVM file: %.32s, file size(%d) > input buffer size(%d), can't write NVM",
        //          fileOper->fileName, fileOper->fileSize, bufSize);

        ECOMM_TRACE(UNILOG_PS, PsNvmWrite_1_1, P_ERROR, 3,
                    "PS NVM, NVM file ID: %d, file size(%d) != input buffer size(%d), can't write NVM", fileId, fileOper->fileSize, bufSize);

        return FALSE;
    }

    // here print a warning, just notify which NVM should be write
    ECOMM_TRACE(UNILOG_PS, PsNvmWrite_1_2, P_WARNING, 1,
                "PS NVM, File ID: %d, need to write to flash", fileId);


    /*
     * delete the old file
    */
    //GosFremove((const char*)(fileOper->fileName));

    /*
     * open the file
    */
    fp = GosFopen(fileOper->fileId, fileOper->fileName, "wb");
    if (fp == PNULL)
    {
        //GosPrintf(P_ERROR, "PS NVM, can't open/write file: %.32s", fileOper->fileName);
        ECOMM_TRACE(UNILOG_PS, PsNvmWrite_1_3, P_ERROR, 1,
                    "PS NVM, can't open/write file id: %d", fileId);

        return FALSE;
    }

    /*
     * allocate the nvm filer buffer
    */
    nvmFileBuf = (UINT8 *)GosAllocMemory(sizeof(PsNvmFileHeader) + bufSize);
    GosCheck(nvmFileBuf != PNULL, sizeof(PsNvmFileHeader), bufSize, 0);

    /*
     * write the header
    */
    pFileHeader = (PsNvmFileHeader *)nvmFileBuf;
    memset(pFileHeader, 0x00, sizeof(PsNvmFileHeader));
    strncpy((CHAR *)(pFileHeader->fName), (const CHAR *)(fileOper->fileName), strlen((const CHAR *)(fileOper->fileName)));
    pFileHeader->fileBodySize = bufSize;
    pFileHeader->version = fileOper->curVersion;
    pFileHeader->checkSum = PsNvmCalcCrcValue(dataBuf, bufSize);

    /*
     * write the body
    */
    memcpy(nvmFileBuf+sizeof(PsNvmFileHeader), dataBuf, bufSize);

    writeSize = GosFwrite(nvmFileBuf, sizeof(PsNvmFileHeader)+bufSize, 1, fp);

    GosFclose(fp);
    GosFreeMemory(&nvmFileBuf);

    if (writeSize != 1)
    {
        //GosPrintf(P_ERROR, "PS NVM, file: %.32s, can't write the file body", fileOper->fileName);
        ECOMM_TRACE(UNILOG_PS, PsNvmWrite_1_4, P_ERROR, 1, "PS NVM, can't write file id: %d", fileId);

        return FALSE;
    }

    return TRUE;
}
#endif

/******************************************************************************
 * PsNvmSaveAll
 * Description: Save the NVM file to the flash
 * input: void
 * output: void
 * Comment:
******************************************************************************/
void PsNvmSaveAll()
{
    UINT8   fileId = 0;

    for (fileId = 0; fileId < PS_MAX_NVM; fileId++)
    {
        if (pNvmFile[fileId] == PNULL)
        {
            continue;
        }

        PsNvmSave((PsNvmFileIdEnum)fileId);


        #if 0
        pFileHeader = (PsNvmFileHeader *)(pNvmFile[fileId]);

        ECOMM_TRACE(UNILOG_PS, PsNvmSaveAll_1, P_WARNING, 1,
                    "PS NVM, File ID: %d, need to write back to flash", fileId);

        /*
         * check the NVM file
        */
        fileOper = &(psNvmFileOperList[fileId]);
        if (pFileHeader->version != fileOper->curVersion ||
            pFileHeader->fileBodySize != fileOper->fileSize ||
            strncmp((const CHAR *)(pFileHeader->fName), (const CHAR *)(fileOper->fileName), strlen((const CHAR *)(fileOper->fileName))) != 0)
        {
            GosDebugBegin(FALSE, pFileHeader->version, pFileHeader->fileBodySize, fileId);
            GosDebugEnd();

            GosFreeMemory(&(pNvmFile[fileId]));
            pNvmFile[fileId] = PNULL;

            continue;
        }

        /*
         * delete the old file
        */
        GosFremove((const char *)(pFileHeader->fName));

        /*
         * open the file
        */
        fp = GosFopen(fileId, pFileHeader->fName, "wb");
        if (fp == PNULL)
        {
            //GosPrintf(P_ERROR, "PS NVM, can't open/write file ID: %d", fileId);

            ECOMM_TRACE(UNILOG_PS, PsNvmSaveAll_2, P_ERROR, 1,
                       "PS NVM, can't open/write file ID: %d", fileId);

            GosFreeMemory(&(pNvmFile[fileId]));
            pNvmFile[fileId] = PNULL;

            continue;
        }

        /*
         * write the file
        */
        writeSize = GosFwrite(pNvmFile[fileId], sizeof(PsNvmFileHeader)+pFileHeader->fileBodySize, 1, fp);

        GosFclose(fp);

        GosFreeMemory(&(pNvmFile[fileId]));
        pNvmFile[fileId] = PNULL;

        if (writeSize != 1)
        {
            //GosPrintf(P_ERROR, "PS NVM, file ID: %d, can't write the file body", fileId);

            ECOMM_TRACE(UNILOG_PS, PsNvmSaveAll_3, P_ERROR, 1,
                       "PS NVM, file ID: %d, can't write the file body", fileId);
        }
        #endif
    }

    return;
}

/******************************************************************************
 * PsNvmSave
 * Description: Save one NVM file (if changed) to the flash
 * input: PsNvmFileIdEnum fileId;   //which PS NVM file
 * output: BOOL
 * Comment:
******************************************************************************/
BOOL PsNvmSave(PsNvmFileIdEnum fileId)
{
    GOSFILE fp = PNULL;
    PsNvmFileHeader *pFileHeader = PNULL;
    UINT32  writeSize = 0;
    const PsNvmFileOper *fileOper = PNULL;

    GosCheck(fileId < PS_MAX_NVM, fileId, 0, PS_MAX_NVM);

    if (pNvmFile[fileId] == PNULL)
    {
        return TRUE;
    }

    ECOMM_TRACE(UNILOG_PS, PsNvmSave_1, P_WARNING, 1,
                "PS NVM, File ID: %d, need to write back to flash", fileId);

    pFileHeader = (PsNvmFileHeader *)(pNvmFile[fileId]);

    /*
     * check the NVM file
    */
    fileOper = &(psNvmFileOperList[fileId]);
    if (pFileHeader->version != fileOper->curVersion ||
        pFileHeader->fileBodySize != fileOper->fileSize ||
        strncmp((const CHAR *)(pFileHeader->fName), (const CHAR *)(fileOper->fileName), strlen((const CHAR *)(fileOper->fileName))) != 0)
    {
        GosDebugBegin(FALSE, pFileHeader->version, pFileHeader->fileBodySize, fileId);
        GosDebugEnd();

        GosFreeMemory(&(pNvmFile[fileId]));
        pNvmFile[fileId] = PNULL;

        ECOMM_TRACE(UNILOG_PS, PsNvmSave_2, P_WARNING, 5,
                    "PS NVM, File ID: %d, write failed, as version: %d/%d, fileSize: %d/%d, maybe not right",
                    fileId, pFileHeader->version, fileOper->curVersion,
                    pFileHeader->fileBodySize, fileOper->fileSize);


        return FALSE;
    }

    /*
     * delete the old file
    */
    //GosFremove((const char *)(pFileHeader->fName));

    /*
     * open the file
    */
    fp = GosFopen(fileId, pFileHeader->fName, "wb");
    if (fp == PNULL)
    {
        //GosPrintf(P_ERROR, "PS NVM, can't open/write file ID: %d", fileId);

        ECOMM_TRACE(UNILOG_PS, PsNvmSave_3, P_ERROR, 1,
                    "PS NVM, can't open/write file ID: %d", fileId);

        GosFreeMemory(&(pNvmFile[fileId]));
        pNvmFile[fileId] = PNULL;

        return FALSE;
    }

    /*
     * write the file
    */
    writeSize = GosFwrite(pNvmFile[fileId], sizeof(PsNvmFileHeader)+pFileHeader->fileBodySize, 1, fp);

    GosFclose(fp);

    GosFreeMemory(&(pNvmFile[fileId]));
    pNvmFile[fileId] = PNULL;

    if (writeSize != 1)
    {
        ECOMM_TRACE(UNILOG_PS, PsNvmSave_4, P_ERROR, 1,
                   "PS NVM, file ID: %d, can't write the file body", fileId);

        return FALSE;
    }

    return TRUE;
}



