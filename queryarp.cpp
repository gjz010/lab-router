#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_arp.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
int queryArp(const char* caddr, char* mac){
        std::string cmd="ping -c 1 "+std::string(caddr);
        system(cmd.c_str());
        in_addr ip;
        ip.s_addr=inet_addr(caddr);
        struct arpreq arp;
        memset(&arp, 0, sizeof(struct arpreq));
        struct sockaddr_in* addr=(struct sockaddr_in*)(&(arp.arp_pa));
        addr->sin_family=AF_INET;
        addr->sin_addr=ip;
        strncpy(arp.arp_dev, ifn, 15);

        int pin=socket(AF_INET, SOCK_DGRAM, 0);
        int ret=ioctl(pin, SIOCGARP, &arp);
        shutdown(pin,2);
        if(ret<0) return -1;
        if(arp.arp_flags & ATF_COM){
                memcpy(mac, arp.arp_ha.sa_data, 6);
                return 0;
        }else{
                return -2;
        }
}
/*
char buffer[2000];
int main(){
	struct in_addr inaddr;
	inaddr.s_addr=inet_addr("192.168.1.30");
	int ret=queryArp("eth0", inaddr, buffer);
	printf("%d %s\n", ret, buffer);
	return 0;

}
*/
