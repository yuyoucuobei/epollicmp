

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "epollicmp.h"

CEpollIcmp *pei = NULL;


void printusage()
{
    printf("USAGE:  ./epollicmp destaddr(url/ip) icmp_interval(ms) icmp_pkglen(byte) \n"
                 "\n");
}

void icmp_sigint(int signo)
{
    if(NULL != pei)
    {
        pei->stop();
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, icmp_sigint);
    
    if(argc < 4)
    {
        printusage();
        return 1;
    }

    string destaddr = argv[1];
    int icmp_interval = atoi(argv[2]);
    int icmp_pkglen = atoi(argv[3]);

    printf("epollicmp start %s %s %s\n", argv[1], argv[2], argv[3]);

    pei = new CEpollIcmp();
    if(NULL == pei)
    {
        printf("Failed to new CEpollIcmp\n");
        return 1;
    }

    pei->start(destaddr, icmp_interval, icmp_pkglen);

    if(pei)
    {
        delete pei;
        pei = NULL;
    }

    printf("epollicmp end\n");
    return 0;
}
