#include "rip.h"
#include "sockethandler.h"
#include <cstdio>
#include <sys/timerfd.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <net/if.h>
#include <sys/socket.h>
#include <list>
#include <unordered_map>
#include <algorithm>
#include "lookuproute.h"
struct IfInfo{
    struct in_addr ip;
    std::string iface;
    unsigned int iface_index;
    struct in_addr mask;
};

namespace std{
    template<>
    struct hash<RouteKey>{
        private:
        void combine(std::size_t& seed, const std::size_t newhash) const{
            seed=seed*31+newhash;
        }
        public:
        std::size_t operator()(const RouteKey& k) const{
            using std::size_t;
            using std::hash;
            using std::string;
            
            size_t sd=17;
            //combine(sd, hash<unsigned short>()(k.usFamily));
            combine(sd, hash<unsigned int>()(k.stAddr.s_addr));
            combine(sd, hash<unsigned int>()(k.stSubnetMask.s_addr));
            return sd;
        }

    };
};
//Modified to be more beautiful in C++.
std::vector<IfInfo> localIfInfo()
{
    
	struct ifaddrs *pstIpAddrStruct = NULL;
	struct ifaddrs *pstIpAddrStCur  = NULL;
	void *pAddrPtr=NULL;
	const char *pcLo = "127.0.0.1";
	
	getifaddrs(&pstIpAddrStruct); //linux系统函数
	pstIpAddrStCur = pstIpAddrStruct;
	
	//int i = 0;
    IfInfo info;
    std::vector<IfInfo> ret;
	while(pstIpAddrStruct != NULL)
	{
        printf("%d %s\n", pstIpAddrStruct->ifa_addr->sa_family, (const char*)pstIpAddrStruct->ifa_name) ;
		if(pstIpAddrStruct->ifa_addr->sa_family==AF_INET)
		{
            
			pAddrPtr = &((struct sockaddr_in *)pstIpAddrStruct->ifa_addr)->sin_addr;
			char cAddrBuf[INET_ADDRSTRLEN];
			memset(&cAddrBuf,0,sizeof(INET_ADDRSTRLEN));
			inet_ntop(AF_INET, pAddrPtr, cAddrBuf, INET_ADDRSTRLEN);
			if(strcmp((const char*)&cAddrBuf,pcLo) != 0 )
			{
                info.ip.s_addr=inet_addr((const char*)&cAddrBuf);
                info.iface=std::string((const char*)pstIpAddrStruct->ifa_name);
                info.iface_index=if_nametoindex((const char*)pstIpAddrStruct->ifa_name);
		info.mask=((struct sockaddr_in *)pstIpAddrStruct->ifa_netmask)->sin_addr;
                printf("%s\n", (const char*)pstIpAddrStruct->ifa_name);
                ret.push_back(info);
				//pcLocalAddr[i] = (char *)malloc(sizeof(INET_ADDRSTRLEN));
				//pcLocalName[i] = (char *)malloc(sizeof(IF_NAMESIZE));
				//strcpy(pcLocalAddr[i],(const char*)&cAddrBuf);
				//strcpy(pcLocalName[i],(const char*)pstIpAddrStruct->ifa_name);
				//i++;
			}	
		}
		pstIpAddrStruct = pstIpAddrStruct->ifa_next;
	}
	freeifaddrs(pstIpAddrStCur);//linux系统函数
	return ret;
}






//Greatest Socket Handler
class RIPSocketManager : public SocketHandler{
private:
    char buf[100000];
    char cmsgbuf[100000];
    std::vector<IfInfo> info;
    std::unordered_map<RouteKey, RouteEntry> entries;
    void addTestEntry(){
        RouteEntry test;
        test.usFamily=htons(2);
        test.usTag=0;
        test.stAddr.s_addr=inet_addr("192.168.81.89");
        test.stSubnetMask.s_addr=inet_addr("255.255.255.255");
        test.stNexthop.s_addr=inet_addr("192.168.1.40");
        test.uiMetric=4;
        test.uiIfIndex=info[0].iface_index;
        entries.insert(std::make_pair(test.extract(), test));
    }
public:
    bool checkDirectConnect(in_addr target, nextaddr* naddr){
	for(auto iter=info.begin(); iter!=info.end(); iter++){
		if(target.s_addr!=iter->ip.s_addr && (target.s_addr & iter->mask.s_addr) == (iter->ip.s_addr & iter->mask.s_addr)){
			naddr->ifname=iter->iface;
			naddr->ifindex=iter->iface_index;
			naddr->nexthopaddr=target;
			return true;
		}
	}
	return false;
    }
    RIPSocketManager(std::vector<IfInfo> info):info(info){
        //Create the holy socket.
        addTestEntry();
        fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if(fd==-1){
            perror("create udp socket error\n");
            exit(1);
        }
        int optval=1;
        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))<0){
            perror("SO_REUSEADDR fail\n");
            exit(1);
        }
        optval=1;
       if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))<0){
            perror("SO_REUSEPORT, fail\n");
            exit(1);
        }
        optval=1;
        if(setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &optval, sizeof(optval))<0){
            perror("IP_PKTINFO fail\n");
            exit(1);
        }

        int loop = 0;
        int err = setsockopt(fd,IPPROTO_IP, IP_MULTICAST_LOOP,&loop, sizeof(loop));
        if(err < 0)
        {
            perror("setsockopt():IP_MULTICAST_LOOP\n");
            exit(1);
        }
        sockaddr_in addr;
        memset(&addr, 0, sizeof(sockaddr_in));
        addr.sin_family=PF_INET;
        addr.sin_addr.s_addr=htonl(INADDR_ANY);
        addr.sin_port=htons(520);
        if(::bind(fd, (sockaddr*)&addr, sizeof(sockaddr))==-1){
            perror("udp bind fail\n");
            exit(1);
        }
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr=inet_addr(RIP_GROUP);
        for(auto iter=info.begin();iter!=info.end();iter++){
            printf("Add %s\n", iter->iface.c_str());
            mreq.imr_interface=iter->ip;
            if(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq))==-1){
                perror("fail add to multicast\n");
                exit(1);
            }
        }
    }
private:
    in_addr last_iface;
    int last_iface_index;
    std::string last_iface_name;
    void reply(char* buffer, int len){
        setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &last_iface, sizeof(last_iface));
        sockaddr_in destination;
        memset(&destination, 0, sizeof(sockaddr_in));
        destination.sin_addr.s_addr = inet_addr(RIP_GROUP);
        destination.sin_family = AF_INET;
        destination.sin_port = htons(520);
        sendto(fd, buffer, len, 0, (struct sockaddr*)&destination, sizeof(destination));
    }
    unsigned int prefix(in_addr ip){
        return 24;
    }
    void updateEntry(const RouteEntry& entry){
        RouteKey k=entry.extract();
        if(entries.count(k)){
            auto& oent=entries[k];
            if(oent.stNexthop.s_addr==entry.stNexthop.s_addr){
                printf("Updating existed route metric\n");
                oent=entry;
                if(oent.uiMetric>=16){
                    printf("Route broken!\n");
                    oent.uiMetric=16;
                    
                    //TODO: Do route erase.
                    delete_route(entry.stAddr, prefix(entry.stSubnetMask));
                    //entries.erase(k);
                }
            }else{
                printf("Faster route?\n");
                if(entry.uiMetric<oent.uiMetric){
                    oent=entry;
                    delete_route(entry.stAddr, prefix(entry.stSubnetMask));
                    insert_route(entry.stAddr.s_addr, prefix(entry.stSubnetMask), last_iface_name.c_str(), last_iface_index, entry.stNexthop.s_addr);
                    //TODO: Do route replace.
                }
            }
        }else{
            if(entry.uiMetric<16){
                entries.insert(std::make_pair(k, entry));
                insert_route(entry.stAddr.s_addr, prefix(entry.stSubnetMask), last_iface_name.c_str(), last_iface_index,  entry.stNexthop.s_addr);
                //TODO: Do route insert.
            }else{
                printf("Dead route received and nothing happens.\n");
            }
        }
    }
    void sendAllEntries(in_addr their){
        printf("Sending all routes.\n");
        RipPacket packet;
        packet.ucCommand=2;
        packet.ucVersion=2;
        packet.usZero=0;
        int cent=0;
        int sent=0;
	for(auto iter=info.begin(); iter!=info.end(); iter++){
	    packet.RipEntries[cent].usFamily=htons(AF_INET);
            packet.RipEntries[cent].usTag=0;
            packet.RipEntries[cent].stAddr=iter->ip;
            packet.RipEntries[cent].stSubnetMask.s_addr=inet_addr("255.255.255.0");
	    packet.RipEntries[cent].stNexthop.s_addr=inet_addr("0.0.0.0");
	    cent++;
            if(cent==RIP_MAX_ENTRY){
                reply((char*)(&packet), cent*sizeof(RipEntry)+RIP_PACKET_HEAD);
                cent=0;
                sent=1;
            }
	}
        for(auto iter=entries.begin();iter!=entries.end();iter++){
            auto& re=iter->second;
            re.print();
            packet.RipEntries[cent].usFamily=re.usFamily;
            packet.RipEntries[cent].usTag=re.usTag;
            packet.RipEntries[cent].stAddr=re.stAddr;
            packet.RipEntries[cent].stSubnetMask=re.stSubnetMask;
            packet.RipEntries[cent].stNexthop=re.stNexthop;
            if(re.stNexthop.s_addr==their.s_addr){
                packet.RipEntries[cent].uiMetric=htonl(16);
            }else{
                packet.RipEntries[cent].uiMetric=htonl(re.uiMetric);
            }
            cent++;
            if(cent==RIP_MAX_ENTRY){
                reply((char*)(&packet), cent*sizeof(RipEntry)+RIP_PACKET_HEAD);
                cent=0;
                sent=1;
            }
        }
        if(cent!=0 || sent==0){
            printf("Sending last route.\n");
            reply((char*)(&packet), cent*sizeof(RipEntry)+RIP_PACKET_HEAD);
            printf("Sending last routes done.\n");
        }
        printf("Sending all routes done.\n");
    }

    void parsePacket(const char* buffer, int len, sockaddr_in src){
        printf("Packet(%s) received with size %d\n", inet_ntoa(src.sin_addr), len);
        int entry_items=(len-sizeof(RIP_PACKET_HEAD))/sizeof(RipEntry);
        RipPacket* packet=(RipPacket*)buffer;
        if(packet->ucCommand==1){ //request
            if(entry_items==1 
            && packet->RipEntries[0].usFamily==0 
            && ntohl(packet->RipEntries[0].uiMetric)==16){
                printf("Request received!\n");
                //Should send entire table.
                sendAllEntries(src.sin_addr);
            }
        }else if(packet->ucCommand==2){ //response
            for(int i=0;i<entry_items;i++){
                RouteEntry entry;
                entry.usFamily=packet->RipEntries[i].usFamily;
                entry.usTag=packet->RipEntries[i].usTag;
                entry.stAddr=packet->RipEntries[i].stAddr;
                entry.stSubnetMask=packet->RipEntries[i].stSubnetMask;
                entry.stNexthop=src.sin_addr;
                entry.uiMetric=ntohl(packet->RipEntries[i].uiMetric)+1;
                updateEntry(entry);
            }
        }else printf("Bad packet!\n");
    }
    bool handlePacket(){
        struct iovec iov[1];
        iov[0].iov_base=buf;
        iov[0].iov_len=sizeof(buf);
        struct cmsghdr* cmsg;
        struct msghdr message;
        sockaddr_in their;
        message.msg_name=&their;
        message.msg_namelen=sizeof(their);
        message.msg_iov=iov;
        message.msg_iovlen=1;
        message.msg_control=cmsgbuf;
        message.msg_controllen=sizeof(cmsgbuf);
        int size;
        if((size=recvmsg(fd, &message, 0))<0){
			if(errno==EAGAIN || errno==EWOULDBLOCK) return false;
			else{printf("Receive multicast error.");return false;}
        }
        
        for(cmsg=CMSG_FIRSTHDR(&message); cmsg!=NULL; cmsg=CMSG_NXTHDR(&message, cmsg)){
            if(cmsg->cmsg_level!=IPPROTO_IP || cmsg->cmsg_type!=IP_PKTINFO) continue;
            struct in_pktinfo* pi=(in_pktinfo*)CMSG_DATA(cmsg);
            for(auto iter=info.begin();iter!=info.end();iter++){
                if(pi->ipi_ifindex==iter->iface_index){
                    last_iface=iter->ip;
                    last_iface_index=iter->iface_index;
                    last_iface_name=iter->iface;
                    break;
                }

            }
            //last_iface=pi->ipi_spec_dst;
            //last_iface_index=pi->ipi_ifindex;
            //last_iface_name=pi->pip
        }
        parsePacket(buf, size, their);
        return false;
    }
public:
    void multicast(char* buffer, int len){
        for(auto iter=info.begin();iter!=info.end();iter++){
            last_iface=iter->ip;
            last_iface_index=iter->iface_index;
            reply(buffer, len);
        }
    }
    void onEvent(){
        while(handlePacket()){}

    }
};

char REQUEST_PACKET[]={
1,2,0,0,
0,0,0,0,
0,0,0,0,
0,0,0,0,
0,0,0,0,
0,0,0,16};
//Timer for sending requests.
class RipRequestSender : public SocketHandler{
private:
    RIPSocketManager* manager;
public:
    RipRequestSender(RIPSocketManager* manager):manager(manager){
        this->fd=timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    }
    void start(int epoll_fd){
        
        
        struct itimerspec new_value;
        struct timespec interval;
        interval.tv_sec=5;
        interval.tv_nsec=0;
        new_value.it_interval=interval;
        new_value.it_value=interval;
        if(timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, NULL)==-1){
            perror("Timer failed.\n");
        }
        bind(epoll_fd);
        printf("RipRequestSender started.\n");

    }
    void onEvent(){
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!PingPong!\n");
        long long buf;
        while(read(fd, &buf, sizeof(uint64_t))>0) ;
        manager->multicast(REQUEST_PACKET, sizeof(REQUEST_PACKET));
        //Fire a packet.
    }
    
};

struct RipClient::Internal{
    RIPSocketManager* manager;
    RipRequestSender* sender;
};

RipClient::RipClient(){
    internal=new Internal;
    internal->manager=new RIPSocketManager(localIfInfo());
    internal->sender=new RipRequestSender(internal->manager);
}



void RipClient::start(int efd){
    printf("RIP Client Starting...\n");
    internal->sender->start(efd);
    internal->manager->bind(efd);
    printf("RIP Client Started!\n");
}

bool RipClient::checkDirectConnect(in_addr target, nextaddr* naddr){
    return internal->manager->checkDirectConnect(target, naddr);
}
