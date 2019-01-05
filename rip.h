#ifndef __MYRIP_H
#define __MYRIP_H
#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "lookuproute.h"
#define RIP_VERSION    	2
#define RIP_REQUEST    	1
#define RIP_RESPONSE   	2
#define RIP_INFINITY  	16
#define RIP_PORT		520
#define RIP_PACKET_HEAD	4
#define RIP_MAX_PACKET  504
#define RIP_MAX_ENTRY   25
#define ROUTE_MAX_ENTRY 256
#define RIP_GROUP		"224.0.0.9"

#define RIP_CHECK_OK    1
#define RIP_CHECK_FAIL  0
#define AddRoute        24
#define DelRoute        25
#include "sockethandler.h"
#include <string>
typedef struct RipEntry
{
	unsigned short usFamily;
	unsigned short usTag;
	struct in_addr stAddr;
	struct in_addr stSubnetMask;
	struct in_addr stNexthop;
	unsigned int uiMetric;
}TRipEntry;

typedef struct  RipPacket
{
	unsigned char ucCommand;
	unsigned char ucVersion;
	unsigned short usZero; 
	TRipEntry RipEntries[RIP_MAX_ENTRY];
}TRipPkt;

struct RouteKey{
	struct in_addr stAddr;
	struct in_addr stSubnetMask;
	bool operator==(const RouteKey& k) const{
		return stAddr.s_addr==k.stAddr.s_addr && stSubnetMask.s_addr==k.stSubnetMask.s_addr;
	}
};
typedef struct RouteEntry
{
	//struct RouteEntry *pstNext;
	unsigned short usFamily;
	unsigned short usTag;
	struct in_addr stAddr;
	struct in_addr stSubnetMask;
	struct in_addr stNexthop;
	unsigned int uiMetric;
	unsigned int uiIfIndex;
	void print() const{
		printf("Router Entry [F: %d, T: %d] ", (int)(ntohs(usFamily)), (int)(ntohs(usTag)));
		printf("%s ", inet_ntoa(stAddr));
		printf("%s ", inet_ntoa(stSubnetMask));
		printf("%s ", inet_ntoa(stNexthop));
		printf("%d If:%d\n",uiMetric, uiIfIndex);
	}
	RouteKey extract() const{
		RouteKey k;
		k.stAddr=stAddr;
		k.stSubnetMask=stSubnetMask;
		return k;
	}
}TRtEntry;
/*
typedef struct SockRoute
{
	unsigned int uiPrefixLen;
	struct in_addr stIpPrefix;
	unsigned int uiIfindex;
	struct in_addr stNexthop;
	unsigned int uiCmd;
}TSockRoute;
*/

class RipClient {
private:
    struct Internal;
    Internal* internal;
public:
    RipClient();
    void start(int epoll_fd);
	bool checkDirectConnect(in_addr target, nextaddr* naddr);
private:

};


/*
void route_SendForward(unsigned int uiCmd,TRtEntry *pstRtEntry);
void requestpkt_Encapsulate();
void rippacket_Receive();
void rippacket_Send(struct in_addr stSourceIp);
void rippacket_Multicast(char *pcLocalAddr);
void request_Handle(struct in_addr stSourceIp);
void response_Handle(struct in_addr stSourceIp);
void rippacket_Update();
void routentry_Insert();
void localinterf_GetInfo();
void ripdaemon_Start();
*/
#endif

