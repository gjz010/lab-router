#include "checksum.h"
#include <netinet/in.h>

int check_sum(unsigned short *iphd,int len,unsigned short checksum)
{
    int counter=count_check_sum(iphd);
    printf("Checksum: %d\n", counter);
    return counter==0xffff;
}
unsigned short count_check_sum(unsigned short *iphd)
{
    unsigned long long total=0;
    for(int i=0;i<(sizeof(_iphdr)>>1);i+=1) {
        total+=iphd[i];
    }
    while(total>>16) total=(total&0xFFFF)+(total>>16);
    return total;
}

void countdown_ttl(_iphdr* header){
    unsigned long long total=0;
    unsigned short old=ntohs(header->checksum);
    unsigned char ttl=header->ttl;
    printf("TTL:%d %d\n", ttl, header->ttl);
    header->ttl=ttl-1;
    total=old;
    total=total+0x0100;
    while(total>>16) total=(total&0xFFFF)+(total>>16);
    header->checksum=htons(total);

}