#include "arpfind.h"
#include <unordered_map>
#include <string>
#include <sstream>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

std::unordered_map<std::string, struct arpmac> arptable;
int pin=0;
//这query真丢人
int queryArp(const char* ifn, const char* caddr, char* mac){
	if(pin==0){
		pin=socket(AF_INET, SOCK_DGRAM, 0);
	}
	printf("Query arp from %s for caddr %s\n", ifn, caddr);
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

        //int pin=socket(AF_INET, SOCK_DGRAM, 0);
        int ret=ioctl(pin, SIOCGARP, &arp);
        if(ret<0){
		perror("Remote arp error");
        //shutdown(pin,2);

		return -1;
	}
        //shutdown(pin,2);

        if(arp.arp_flags & ATF_COM){
                memcpy(mac, arp.arp_ha.sa_data, 6);
                return 0;
        }else{
                return -2;
        }
}

int arpGet(struct arpmac *srcmac,const char *ifname, char *ipStr)  
{  
    std::ostringstream os;
    os<<std::string(ifname)<<"|"<<std::string(ipStr);
	std::string key=os.str();
	if(arptable.find(key)!=arptable.end()){
		*srcmac=(arptable.find(key)->second);
	}else{
		srcmac->index=0;
		char* mac=(char*)(&(srcmac->mac));
		//in_addr addr;
		//addr.s_addr=inet_addr(ipStr);
		int ret=queryArp(ifname, ipStr, mac);
		if(ret==0){
			arptable.insert(std::make_pair(key, *srcmac));
		}
		return ret;
	}
    return 0;  
}  
                                                                                                        
                                                                                                          
int getLocalArp(const char* ifname, char* mac){
        struct ifreq s;
        int fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        strcpy(s.ifr_name, ifname);
        
        if(ioctl(fd, SIOCGIFHWADDR, &s)){
                perror("Local arp error ");
		shutdown(fd,2);
                return -1;
        }else{
                shutdown(fd,2);
                memcpy(mac, s.ifr_addr.sa_data, 6);
                return 0;
        }
}                                                                              
                                                                                                              
