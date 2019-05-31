

#pragma once

#include "define.h"


class CEpollIcmp
{
public:
    CEpollIcmp();
    ~CEpollIcmp();

    int start(const string &remote_ip, 
                int interval=DEFAULT_INTERVAL, 
                int pkglen=DEFAULT_PKGLEN);

    int stop();
    
private:
    int initSock();
    int mainLoop();
    static void *thread_func_send_icmp(void *argv);

    //checksum
    unsigned short cal_chksum(unsigned short *addr,int len);

    struct timeval cal_time_offset(struct timeval begin, struct timeval end);

    void icmp_pack(struct icmp* icmphdr, int seq, int length);

    int icmp_unpack(char* buf, int len);

    int set_nonblock(int sockfd);

    void statistic_print();

    
private:
    string m_strRemoteIp;
    int m_iInterval;  //microseconds
    int m_iPkgLen;
    int m_iEpollFd;
    int m_iSocketIcmp;
    bool m_bContinue;
    sockaddr_in m_addrDest;
    pthread_t m_threadidSend;

    int m_iSendCount;
    int m_iRecvCount;
    struct timeval m_tStarttime;
    struct timeval m_tEndtime;
    float m_fRespMax;
    float m_fRespMin;
    float m_fRespAll;

    map<int, PACKET_STATUS*> m_mapPacketStatus;
    mutex m_mtxMap;
};


