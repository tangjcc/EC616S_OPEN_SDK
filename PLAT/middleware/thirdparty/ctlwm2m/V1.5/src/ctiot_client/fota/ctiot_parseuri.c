/************************************************************************

            (c) Copyright 2018 by 中国电信上海研究院. All rights reserved.

**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ct_liblwm2m.h"
#include "ctiot_parseuri.h"



bool ctiot_funcv1_parse_url(char* url,CTIOT_URI* uri)
{
    //printf("url:%s\n",url);
    if(url==NULL || strlen(url)<15)
    {
        return false;
    }
    if(uri==NULL)
    {
        printf("malloc failed\n");
        return false;
    }
    int protocollen=8;
    if(strncasecmp(url,"https",5)==0)
    {
        protocollen=8;
        memcpy(uri->protocol,url,5);
        uri->port=443;
    }
    else if(strncasecmp(url,"coaps",5)==0)
    {
        protocollen=8;
        memcpy(uri->protocol,url,5);
        uri->port=5684;
    }
    else if(strncasecmp(url,"http",4)==0)
    {
        protocollen=7;
        memcpy(uri->protocol,url,4);
        uri->port=80;
    }
    else if(strncasecmp(url,"coap",4)==0)
    {
        protocollen=7;
        memcpy(uri->protocol,url,4);
        uri->port=5683;
    }
    else{
        printf("no coap/http protocol header found\n");
        free(uri);
        return false;
    }
    //printf("protocol:%s\n",uri->protocol);
    int i= protocollen;
    for(;i<strlen(url);i++)
    {
        if(i>50+protocollen)
        {
            free(uri);
            printf("url error\n");
            return false;
        }
        if(url[i] != 47 && url[i] != 92 && url[i] != 58) //':'=58 '\'=92 '/'=47
        {
            uri->address[i-protocollen]=url[i];
        }
        else
        {
            break;
        }
    }

    //printf("address:%s,cur url[%d]=%d\n",uri->address,i,url[i]);
    if(url[i]==58)
    {
        i++;
        char tmpport[5]={0};
        int j=0;
        for(;i<strlen(url);i++)
        {
            if(url[i] != 47 && url[i] != 92)
            {
                if(url[i]>=48 && url[i]<=57)//'0'and '9'
                {
                    tmpport[j++]=url[i];
                }
            }
            else
            {
                uri->port=atoi(tmpport);
                break;
            }
            if(j>=5)
            {
                uri->port=atoi(tmpport);
                break;
            }
        }
    }
    //printf("port:%d,cur url[%d]=%d\n",uri->port,i,url[i]);;
    int start=0;
    int pos=0;
    for(;i<strlen(url);i++)
    {
        if(url[i] == 47 || url[i] == 92)// '\'=92 '/'=47
        {
            start=1;
        }
        if(start==1)
        {
            uri->uri[pos++]=url[i];
        }
    }
    //printf("uri:%s\n",uri->uri);
    return true;
}
