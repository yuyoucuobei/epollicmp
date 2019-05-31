

#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <string>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <netdb.h>
#include <vector>
#include <map>
#include <mutex>


using namespace std;

const int DEFAULT_PORT      = 12345;
const int MAX_EVENTS_NUM    = 1024;
const int WAIT_EVENTS_NUM   = 64;
const int BUFF_LEN          = 8192;
const int DEFAULT_INTERVAL  = 1000; //micro second
const int DEFAULT_PKGLEN    = 64;


typedef struct ST_EVENT_DATA
{
    int fd;
    char buff[BUFF_LEN]; //recv data buffer
    int len;
    int sequence;

    ST_EVENT_DATA()
    {
        memset(this, 0, sizeof(ST_EVENT_DATA));
    };
} EVENT_DATA;


typedef struct ST_PACKET_STATUS
{
    struct timeval begin_time;
    struct timeval end_time;
    int flag;   //发送标志,0-未发送，1-已发送，2-发送失败, 3-接收成功
    int seq;     //包的序列号

    ST_PACKET_STATUS()
    {
        memset(this, 0x00, sizeof(ST_PACKET_STATUS));
    };
} PACKET_STATUS;



