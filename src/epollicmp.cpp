

#include "epollicmp.h"

CEpollIcmp::CEpollIcmp()
{
    m_strRemoteIp.clear();
    m_iInterval = 0;
    m_iPkgLen = 0;
    m_iEpollFd = 0;
    m_iSocketIcmp = 0;
    m_bContinue = false;
    m_threadidSend = 0;

    m_iSendCount = 0;
    m_iRecvCount = 0;

    m_fRespMax = 0;
    m_fRespMin = 0xffff;
    m_fRespAll = 0;

    m_mapPacketStatus.clear();
    
}

CEpollIcmp::~CEpollIcmp()
{
    map<int, PACKET_STATUS*>::iterator iter;
    iter = m_mapPacketStatus.begin();
    for(; iter != m_mapPacketStatus.end(); iter++)
    {
        if(NULL != iter->second)
        {
            delete iter->second;
            iter->second = NULL;
        }
    }

    m_mapPacketStatus.clear();
}

int CEpollIcmp::start(const string &remote_ip, int interval, int pkglen)
{
    printf("CEpollIcmp::start start\n");
    m_strRemoteIp = remote_ip;
    m_iInterval = interval;
    m_iPkgLen = pkglen;
    m_bContinue = true;

    //epoll init
    m_iEpollFd = epoll_create(MAX_EVENTS_NUM);
    if(-1 == m_iEpollFd)
    {
        printf("epoll_create failed, errno = %d\n", errno);
        return 1;
    }

    //socket init
    int res = initSock();
    if(0 != res)
    {
        printf("initSock failed\n");
        return 1;
    }

    //thread send icmp
    if (pthread_create(&m_threadidSend, NULL, 
            CEpollIcmp::thread_func_send_icmp, (void*)this) != 0)
    {
        printf("Failed to pthread_create\n");
        return false;  
    }

    gettimeofday(&m_tStarttime, NULL);

    //event loop
    mainLoop();

    pthread_join(m_threadidSend, NULL);

    //statistic
    statistic_print();

    return 0;
}

int CEpollIcmp::stop()
{
    printf("CEpollIcmp::stop start\n");

    gettimeofday(&m_tEndtime, NULL);

    m_bContinue = false;

    return 0;
}

int CEpollIcmp::initSock()
{
    int res = 0;
    printf("CEpollIcmp::initSock start\n");

    int size = 128*1024;
    unsigned int inaddr = 1;

    //get proto
    struct protoent* protocol = NULL;
    protocol = getprotobyname("icmp"); //获取协议类型ICMP
    if(protocol == NULL)
    {
        printf("Fail to getprotobyname!\n");
        return 1;
    }

    //socket init
    m_iSocketIcmp = socket(AF_INET, SOCK_RAW, protocol->p_proto);
    if(m_iSocketIcmp < 0)
    {
        printf("Failed to socket, errno = %d\n", errno);
        return 1;
    }

    //socket flags
    if(0 != set_nonblock(m_iSocketIcmp))
    {
        printf("Failed to set nonblock\n");
        return 1;
    }
    
    //socket recv buf
    setsockopt(m_iSocketIcmp, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)); //增大接收缓冲区至128K
    setsockopt(m_iSocketIcmp, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)); 

    //add sock to epoll 
    struct epoll_event epe;
    epe.events =  EPOLLIN;
    epe.data.fd = m_iSocketIcmp;    
    res = epoll_ctl(m_iEpollFd, EPOLL_CTL_ADD, m_iSocketIcmp, &epe);
    if(-1 == res)
    {
        printf("epoll_ctl failed, errno = %d\n", errno);
        return 1;
    }

    //dest ip set
    bzero(&m_addrDest, sizeof(m_addrDest));
    m_addrDest.sin_family = AF_INET;
    inaddr = inet_addr(m_strRemoteIp.c_str());
    if(INADDR_NONE == inaddr)
    {
        //输入的是域名
        struct hostent* host = NULL;
        host = gethostbyname(m_strRemoteIp.c_str());
        if(NULL == host)
        {
            printf("Failed to gethostbyname\n");
            return 1;
        }

        memcpy((char*)&m_addrDest.sin_addr, host->h_addr, host->h_length);
    }
    else
    {
        //输入的是IP地址
        memcpy((char*)&m_addrDest.sin_addr, &inaddr, sizeof(inaddr));
    }   

    inaddr = m_addrDest.sin_addr.s_addr;
    printf("PING %s, (%d.%d.%d.%d) (%d) bytes of data every (%d) microsecond.\n", 
        m_strRemoteIp.c_str(),
        (inaddr&0x000000ff), (inaddr&0x0000ff00)>>8, 
        (inaddr&0x00ff0000)>>16, (inaddr&0xff000000)>>24,
        m_iPkgLen, m_iInterval);
    
    return 0;
}


int CEpollIcmp::mainLoop()
{
    printf("CEpollIcmp::mainLoop start\n");
    
    int nfds; 
    struct epoll_event events[WAIT_EVENTS_NUM];
    int res;
    int timeout = 100;
    int continueflag = 0;

    while(continueflag < 5)
    {
        if(!m_bContinue)
        {// 多跑3次，等待最后的响应消息
            continueflag++;
        }
        
        nfds = epoll_wait(m_iEpollFd, events, WAIT_EVENTS_NUM, timeout);
        if(-1 == nfds)
        {
            if(EINTR == errno)
            {
                continue;
            }
            printf("epoll_wait failed, errno = %d\n", errno);
            return 1;
        }

        //printf("epoll_wait return, nfds = %d\n", nfds);

        for(int i = 0; i < nfds; i++)
        {
            if(events[i].events & EPOLLIN)
            {
                //printf("events[%d].events & EPOLLIN\n", i);

                int sockfd = events[i].data.fd;
                if(sockfd <= 0)
                {
                    printf("sockfd <= 0\n");
                    continue;
                }

                int readlen = 0;
                char buff[BUFF_LEN];
                memset(buff, 0x00, BUFF_LEN);

                readlen = recv(sockfd, buff, BUFF_LEN, 0);
                if(readlen < 0)
                {
                    printf("Failed to recv, readlen < 0\n");
                    continue;
                }

                buff[readlen] = 0;

                if(-1 == icmp_unpack(buff, readlen))
                {
                    printf("Failed to icmp_unpack\n");
                    continue;
                }
                
                m_iRecvCount++;

            }
            else if(events[i].events & EPOLLOUT)
            {
                //printf("events[%d].events & EPOLLOUT\n", i);

                EVENT_DATA *ped = (EVENT_DATA*)events[i].data.ptr;
                if(NULL == ped)
                {
                    printf("NULL == ped\n");
                    continue;
                }

                int sockfd = ped->fd;
                if(sockfd < 0)
                {
                    printf("sockfd < 0\n");
                    continue;
                }

                PACKET_STATUS *ppacket_status = new PACKET_STATUS;
                if(NULL == ppacket_status)
                {
                    printf("NULL == ppacket_status\n");
                    continue;
                }
                
                gettimeofday(&(ppacket_status->begin_time), NULL);
                ppacket_status->flag = 0;
                ppacket_status->seq = ped->sequence;

                res = sendto(sockfd, ped->buff, ped->len, 0, (struct sockaddr*)&m_addrDest, sizeof(m_addrDest));
                if(res < 0)
                {
                    printf("Failed to sendto icmp packet\n");
                    ppacket_status->flag = 2;
                }
                else
                {
                    //printf("Success to sendto icmp packet\n");
                    ppacket_status->flag = 1;
                    m_iSendCount++;
                }

                m_mtxMap.lock();
                m_mapPacketStatus[ppacket_status->seq] = ppacket_status;
                m_mtxMap.unlock();

                free(ped);
                ped = NULL;
                events[i].data.ptr = NULL;

                struct epoll_event epe;
                epe.events = EPOLLIN;
                epe.data.fd = sockfd;
                
                int res = epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, sockfd, &epe);
                if(-1 == res)
                {
                    printf("epoll_ctl failed, errno = %d\n", errno);
                    return 0;
                }

            }
            else
            {
                printf("(Unknown event\n");
            }
        }

        //memset(events, 0x00, WAIT_EVENTS_NUM*sizeof(epoll_event));
    }

    printf("CEpollIcmp::mainLoop end\n");

    return 0;
}

void *CEpollIcmp::thread_func_send_icmp(void *argv)
{
    CEpollIcmp *pei = (CEpollIcmp*)argv;
    if(NULL == pei)
    {
        printf("thread_func_send_icmp ERROR, NULL == pei\n");
        return NULL;
    }

    int sequence = 1;

    while(pei->m_bContinue)
    {
        /*
        //send event
        EVENT_DATA *ped = (EVENT_DATA*)calloc(sizeof(EVENT_DATA), 1);
        ped->fd = pei->m_iSocketIcmp;
        ped->len = pei->m_iPkgLen;
        pei->icmp_pack((struct icmp*)ped->buff, sequence, ped->len);
        ped->sequence = sequence;

        struct epoll_event epe;
        epe.events = EPOLLOUT;
        epe.data.ptr = ped;
        
        int res = epoll_ctl(pei->m_iEpollFd, EPOLL_CTL_MOD, pei->m_iSocketIcmp, &epe);
        if(-1 == res)
        {
            printf("epoll_ctl failed, errno = %d\n", errno);
            return NULL;
        }
*/

        PACKET_STATUS *ppacket_status = new PACKET_STATUS;
        if(NULL == ppacket_status)
        {
            printf("NULL == ppacket_status\n");
            break;
        }        
        gettimeofday(&(ppacket_status->begin_time), NULL);
        ppacket_status->flag = 0;
        ppacket_status->seq = sequence;

        pei->m_mtxMap.lock();
        pei->m_mapPacketStatus[ppacket_status->seq] = ppacket_status;
        pei->m_mtxMap.unlock();

        char buff[BUFF_LEN];
        pei->icmp_pack((struct icmp*)buff, sequence, pei->m_iPkgLen);

        int res = sendto(pei->m_iSocketIcmp, buff, 
                        pei->m_iPkgLen, 0, 
                        (struct sockaddr*)&(pei->m_addrDest), sizeof(pei->m_addrDest));
        if(res < 0)
        {
            printf("Failed to sendto icmp packet\n");
            ppacket_status->flag = 2;
        }
        else
        {
            //printf("Success to sendto icmp packet\n");
            ppacket_status->flag = 1;
            pei->m_iSendCount++;
        }

        sequence = (sequence>=65535)?1:(sequence+1);

        usleep(pei->m_iInterval * 1000);
    }
    
    return NULL;
}

unsigned short CEpollIcmp::cal_chksum(unsigned short *addr, int len)
{       
    int nleft=len;
    int sum=0;
    unsigned short *w=addr;
    unsigned short answer=0;

    /*把ICMP报头二进制数据以2字节为单位累加起来*/
    while(nleft>1)
    {       
        sum+=*w++;
        nleft-=2;
    }
    /*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
    if( nleft==1)
    {       
        *(unsigned char *)(&answer)=*(unsigned char *)w;
        sum+=answer;
    }
    sum=(sum>>16)+(sum&0xffff);
    sum+=(sum>>16);
    answer=~sum;
    return answer;
}

struct timeval CEpollIcmp::cal_time_offset(struct timeval begin, struct timeval end)
{
    struct timeval ans;
    ans.tv_sec = end.tv_sec - begin.tv_sec;
    ans.tv_usec = end.tv_usec - begin.tv_usec;
    if(ans.tv_usec < 0) //如果接收时间的usec小于发送时间的usec，则向sec域借位
    {
        ans.tv_sec--;
        ans.tv_usec+=1000000;
    }
    return ans;
}

void CEpollIcmp::icmp_pack(struct icmp* icmphdr, int seq, int length)
{
    int i = 0;
    pid_t pid;
    pid = getpid();

    icmphdr->icmp_type = ICMP_ECHO;
    icmphdr->icmp_code = 0;
    icmphdr->icmp_cksum = 0;
    icmphdr->icmp_seq = seq;
    icmphdr->icmp_id = pid & 0xffff;
    for(i=0; i<length; i++)
    {
        icmphdr->icmp_data[i] = i;
    }

    icmphdr->icmp_cksum = cal_chksum((unsigned short*)icmphdr, length);
}

int CEpollIcmp::icmp_unpack(char* buf, int len)
{
    int iphdr_len = 0;
    struct timeval begin_time, offset_time;
    float rtt = 0.0;  //round trip time
    pid_t pid;
    pid = getpid();

    struct ip* ip_hdr = (struct ip *)buf;
    iphdr_len = ip_hdr->ip_hl*4;
    struct icmp* icmp = (struct icmp*)(buf+iphdr_len);
    len-=iphdr_len;  //icmp包长度
    if(len < 8)   //判断长度是否为ICMP包长度
    {
        fprintf(stderr, "Invalid icmp packet.Its length is less than 8\n");
        return -1;
    }

    //判断该包是ICMP回送回答包且该包是我们发出去的
    if((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == (pid & 0xffff))) 
    {
        if((icmp->icmp_seq < 0) )
        {
            fprintf(stderr, "icmp packet seq(%d) is out of range(0-%d)\n", icmp->icmp_seq, m_iSendCount);
            return -1;
        }

        m_mtxMap.lock();
        PACKET_STATUS *pps = m_mapPacketStatus[icmp->icmp_seq];
        m_mtxMap.unlock();
        
        if(NULL == pps)
        {
            printf("NULL == pps, sequence=%d\n", icmp->icmp_seq);
            return -1;
        }

        pps->flag = 3;
        begin_time = pps->begin_time;
        gettimeofday(&(pps->end_time), NULL);

        offset_time = cal_time_offset(begin_time, pps->end_time);
        rtt = (float)offset_time.tv_sec*1000 + (float)offset_time.tv_usec/1000; //毫秒为单位

        if(rtt <= m_fRespMin)
        {
            m_fRespMin = rtt;
        }
        if(rtt >= m_fRespMax)
        {
            m_fRespMax = rtt;
        }
        m_fRespAll += rtt;

        printf("%d bytes from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n",
            len, inet_ntoa(ip_hdr->ip_src), icmp->icmp_seq, ip_hdr->ip_ttl, rtt);        

    }
    else
    {
        fprintf(stderr, "Invalid ICMP packet! Its id is not matched!\n");
        return -1;
    }
    return 0;
}

int CEpollIcmp::set_nonblock(int sockfd)
{
    int flags;
    flags = fcntl(sockfd, F_GETFL, 0);
    if(-1 == flags)
    {
        printf("fcntl get failed, errno = %d\n", errno);
        return 1;
    }
    flags |= O_NONBLOCK;
    
    int res = fcntl(sockfd, F_SETFL, flags);
    if(-1 == res)
    {
        printf("fcntl set failed, errno = %d\n", errno);
        return 1;
    }

    return 0;
}

void CEpollIcmp::statistic_print()
{
    //printf("CEpollIcmp::statistic_print start\n");
    
    struct timeval interval = cal_time_offset(m_tStarttime, m_tEndtime);
    float duration = (float)interval.tv_sec + (float)interval.tv_usec/1000000;

    float loserate = (((float)m_iSendCount-(float)m_iRecvCount)/(float)m_iSendCount)*100;

    int inaddr = m_addrDest.sin_addr.s_addr;
    printf("\nPING %s, (%d.%d.%d.%d) (%d) bytes of data every (%d) microsecond.\n", 
        m_strRemoteIp.c_str(),
        (inaddr&0x000000ff), (inaddr&0x0000ff00)>>8, 
        (inaddr&0x00ff0000)>>16, (inaddr&0xff000000)>>24,
        m_iPkgLen, m_iInterval);

    printf( "\nSTATISTIC:\n"
            "ping duration:%.3f second\n"
            "send count:%d\n"
            "recv count:%d\n"
            "lose rate:%.2f%%\n"
            "resp min:%.3f ms\n"
            "resp max:%.3f ms\n"
            "resp average:%.3f ms\n\n",
            duration,
            m_iSendCount,
            m_iRecvCount,
            loserate,
            m_fRespMin,
            m_fRespMax,
            m_fRespAll/(m_iRecvCount==0?1:m_iRecvCount));
}


