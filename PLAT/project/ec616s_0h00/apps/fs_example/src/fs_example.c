/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616S fs example entry source file
 * History:      Rev1.0   2020-03-27
 *
 ****************************************************************************/

#include <stdio.h>
#include "bsp.h"
#include "osasys.h"

/**
  \fn          void fs_example(void *arg)
  \brief       This is a simple example that updates a file named boot_count every time example runs.
               The program can be interrupted at any time without losing track of how many times it has been booted.
  \return
*/
void fs_example(void *arg)
{
    OSAFILE fp = PNULL;
    int32_t ret = 0;

    // read current count
    uint32_t boot_count = 0;

    /*
     * open the config file
    */
    fp = OsaFopen("boot_count", "wb+");   //read & write & create
    if (fp == PNULL)
    {
        printf("Can't open/create 'boot_count' file\r\n");
        return;
    }

    /*
     * check file size
     */
    ret = OsaFsize(fp);

    if(ret == 0)
    {
        printf("First creation of 'boot_count' file\r\n");
    }
    else if(ret != -1)
    {
        /*
         * read file
         */
        ret = OsaFread(&boot_count, sizeof(boot_count), 1, fp);
        if(ret != 1)
        {
            printf("Can't read 'boot_count' file\r\n");
            OsaFclose(fp);
            return;
        }
    }
    else
    {
        printf("Can't get 'boot_count' file size\r\n");
        OsaFclose(fp);
        return;
    }

    // update boot count
    boot_count += 1;

    ret = OsaFseek(fp, 0, SEEK_SET);
    if(ret != 0)
    {
        printf("Seek 'boot_count' file failed\r\n");
        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    ret = OsaFwrite(&boot_count, sizeof(boot_count), 1, fp);
    if (ret != 1)
    {
        printf("Write 'boot_count' file failed\r\n");
    }

    OsaFclose(fp);

    // print the boot count
    printf("boot_count: %d\r\n", (int)boot_count);

    while(1);
}

