//#include "analyseip.h"
#include "checksum.h"
#include "lookuproute.h"
#include "arpfind.h"
#include "sendetherip.h"
#include "recvroute.h"
#include <pthread.h>
#include <sys/epoll.h>
#include <fcntl.h>
#define IP_HEADER_LEN sizeof(struct ip)
#define ETHER_HEADER_LEN sizeof(struct ether_header)

//解决多线程互斥问题的办法：用单线程就完事了
//接收路由信息的子程序
union {
	selfroute tmp;
	char buf[sizeof(selfroute)];
} routebuf;
int bufcounter;

int route_receiver;
int static_route_get(struct selfroute *selfrt)
{

}

bool SetSocketBlockingEnabled(int fd, bool blocking)
{
   if (fd < 0) return false;

#ifdef _WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#else
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return false;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
#endif
}

class SocketHandler{
public:
	SocketHandler(){

	}
	SocketHandler(int fd){
		this->fd=fd;
	}
	virtual void onEvent()=0;
	int bind(int epoll_fd){
		efd=epoll_fd;
		epoll_event ev;
		ev.events=EPOLLIN |EPOLLET;
		ev.data.ptr=this;
		if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev)){
			printf("Start poll in failed.");
			return -1;
		}
		return 0;
	}
	int unbind(){
		epoll_event ev;
		if(epoll_ctl(efd, EPOLL_CTL_DEL, fd, &ev)){
			printf("Stop poll in failed.");
			return -1;
		}

	}
protected:
	int fd;
	int efd;
};

int epoll_fd;
epoll_event events[100];


class SlaveSocketHandler : public SocketHandler{
private:
	union {
		selfroute selfrt;
		char bs[sizeof(selfroute)];
	} data;
	int counter=0;
public:

	SlaveSocketHandler(int fd):SocketHandler(fd){
		
	}
	void onEvent(){
		while(true){
			int size=recv(fd, &(data.bs[counter]), sizeof(selfroute)-counter, MSG_DONTWAIT);
			if(size<0){
				if(errno==EAGAIN || errno==EWOULDBLOCK) break;
				else{printf("Receive error.");break;}
			}
			counter+=size;
			if(counter==sizeof(selfroute)){
				handleFetchRoute();
				printf("Route changed.\n");
				unbind();
				close(fd);
				delete this;
				break;
			}
			
		}
	}
	void handleFetchRoute()
	{
		int st=0;
		struct selfroute &selfrt=data.selfrt;


		//get if.name
		struct if_nameindex *head, *ifni;
		ifni = if_nameindex();
		head = ifni;
		char *ifname;

		// add-24 del-25
		if(selfrt.cmdnum == 24)
		{
			while(ifni->if_index != 0) {
				if(ifni->if_index==selfrt.ifindex)
				{
					printf("if_name is %s\n",ifni->if_name);
						ifname= ifni->if_name;
					break;
				}
				ifni++;
			}

			{
				insert_route(selfrt.prefix.s_addr, selfrt.prefixlen, ifname, ifni->if_index, selfrt.nexthop.s_addr);
			}
		}
		else if(selfrt.cmdnum == 25)
		{
			//从路由表里删除路由
			delete_route(selfrt.prefix, selfrt.prefixlen);
		}

	}
};



class MasterSocketHandler : public SocketHandler{
public:
	MasterSocketHandler(int port){
		
		if((fd=socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0))==-1){
			printf("create master socket error\n");
			
		}else{
			struct sockaddr_in server_addr = { 0 };
			server_addr.sin_family=AF_INET;
			server_addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
			server_addr.sin_port=htons(port);
			if((::bind(fd, (sockaddr*)&server_addr, sizeof(server_addr)))==-1){
				perror("Bind error.\n");

			}else{
				if(listen(fd, 128)==-1){
					printf("listen error\n");
				}
			}
		}
		

	}
	void onEvent(){
		struct sockaddr_in cliaddr;  
		socklen_t len = sizeof(cliaddr);  
		while(true){
			int connfd = accept(fd, (sockaddr *)&cliaddr, &len);
			if(connfd<0){
				if(errno==EAGAIN || errno==EWOULDBLOCK) break;
				else {
					printf("Accept Error.\n");
					break;
				}
			}
			printf("Accepted connection from Quagga.\n");
			SetSocketBlockingEnabled(connfd, false);
			SocketHandler* handler;
			handler=new SlaveSocketHandler(connfd);
			handler->bind(efd);
		}
	}
private:

};




class RawSocketHandler : public SocketHandler{
private:
	char skbuf[1600];
	char data[1580];
	int datalen;
	int recvlen;		
	struct ip *ip_recvpkt;
public:
	RawSocketHandler(){
		if((fd=socket(AF_PACKET,SOCK_RAW|SOCK_NONBLOCK,htons(ETH_P_IP)))==-1)	
		{
			printf("recvfd() error\n");
			//return -1;
		}	
		//ip_recvpkt = (struct ip*)malloc(sizeof(struct ip));
	}
	~RawSocketHandler(){
		delete ip_recvpkt;
		close(fd);
	}
	void onEvent(){
		while(true){
			recvlen=recv(fd,skbuf,sizeof(skbuf),0);
			if(recvlen>0)
			{
				printf("%d\n", recvlen);
				for(int i=0;i<recvlen;i++){
					printf("%02X ", (unsigned int)(skbuf[i] & 0xFF));
				}
				printf("\n");
				ip_recvpkt = (struct ip *)(skbuf+ETHER_HEADER_LEN);
				
				//192.168.1.10是测试服务器的IP，现在测试服务器IP是192.168.1.10到192.168.1.80.
				//使用不同的测试服务器要进行修改对应的IP。然后再编译。
				//192.168.6.2是测试时候ping的目的地址。与静态路由相对应。
				if(ip_recvpkt->ip_src.s_addr == inet_addr("192.168.1.1") && ip_recvpkt->ip_dst.s_addr == inet_addr("192.168.114.54") )
				{
					printf("Caught!\n");
					//分析打印ip数据包的源和目的ip地址
				//	analyseIP(ip_recvpkt);
					/*
					int s;
					memset(data,0,1480);			
					for(s=0;s<1480;s++)
					{
						data[s]=skbuf[s+34];
					}
					*/
					
						// 校验计算模块
						//struct _iphdr *iphead;
						int c=0;

						//iphead=(struct _iphdr *)malloc(sizeof(struct _iphdr));
						
						{
							c=check_sum((unsigned short*)ip_recvpkt, sizeof(_iphdr), ((_iphdr*)ip_recvpkt)->checksum);
						//调用校验函数check_sum，成功返回1
						}
						if(c ==1)
						{
								printf("checksum is ok!!\n");
						}else
						{
								printf("checksum is error !!\n");
								continue;
								//return -1;
						}

						{
							countdown_ttl((_iphdr*)ip_recvpkt);
							c=check_sum((unsigned short*)ip_recvpkt, sizeof(_iphdr), ((_iphdr*)ip_recvpkt)->checksum);
							if(c!=1){
								printf("countdown_ttl wrong!\n");
								continue;
							}
						//调用计算校验和函数count_check_sum，返回新的校验和 
						} 


						//查找路由表，获取下一跳ip地址和出接口模块
						nextaddr nexthopinfo;
						{
							printf("Finding route\n");
							if(lookup_route(ip_recvpkt->ip_dst, &nexthopinfo)){
								printf("No route found!\n");
								continue;
							}
							printf("Route found\n");
						//调用查找路由函数lookup_route，获取下一跳ip地址和出接口
						}

						
						//arp find
						struct arpmac dstmac;
						{
							if(arpGet(&dstmac, nexthopinfo.ifname.c_str(), inet_ntoa(nexthopinfo.nexthopaddr))){

								printf("Arp failed!\n");
								continue;
							};
						//调用arpGet获取下一跳的mac地址		
						}

						//send ether icmp
						{
							if(getLocalArp(nexthopinfo.ifname.c_str(), skbuf+6)!=0){
								printf("Get local arp failed!\n");
								continue;
							}
							memcpy(skbuf, dstmac.mac, 6);
							sockaddr_ll socket_address;
							socket_address.sll_ifindex=nexthopinfo.ifindex;
							socket_address.sll_halen=ETH_ALEN;
							memcpy(socket_address.sll_addr, dstmac.mac, 6);
							if(sendto(fd, skbuf, recvlen, 0, (sockaddr*)&socket_address, sizeof(sockaddr_ll))<0){
								printf("Send failed\n");
							}

						//调用ip_transmit函数   填充数据包，通过原始套接字从查表得到的出接口(比如网卡2)将数据包发送出去
						//将获取到的下一跳接口信息存储到存储接口信息的结构体ifreq里，通过ioctl获取出接口的mac地址作为数据包的源mac地址
						//封装数据包：
						//<1>.根据获取到的信息填充以太网数据包头，以太网包头主要需要源mac地址、目的mac地址、以太网类型eth_header->ether_type = htons(ETHERTYPE_IP);
						//<2>.再填充ip数据包头，对其进行校验处理；
						//<3>.然后再填充接收到的ip数据包剩余数据部分，然后通过raw socket发送出去
						}
				}
			
				

			}else{
				break;
			}
		}
	}
};


int main()	
{
	printf("Starting epoll\n");
	//Initialize epoll
	epoll_fd=epoll_create1(0);
	if(epoll_fd==-1){
		printf("epoll error.\n");
		return -1;
	}

	
	//pthread_t tid;
	//创建raw socket套接字

	
	RawSocketHandler* rawSocket=new RawSocketHandler();
	//Linking raw socket into epoll
	rawSocket->bind(epoll_fd);

	MasterSocketHandler* master=new MasterSocketHandler(800);
	master->bind(epoll_fd);


	
	
	//路由表初始化
	
	route_table=(struct route*)malloc(sizeof(struct route));

	if(route_table==NULL)
	{
			printf("malloc error!!\n");
			return -1;
	}
	memset(route_table,0,sizeof(struct route));


	{

	//调用添加函数insert_route往路由表里添加直连路由
	
	}

	//创建线程去接收路由信息
	//int pd;
	//pd = pthread_create(&tid,NULL,thr_fn,NULL);

// test!
	while(1)
	{
		printf("Polling in...\n");
		int ev_count=epoll_wait(epoll_fd, events, 100, 30000);
		printf("Polling in %d events.\n", ev_count);
		for(int i=0;i<ev_count;i++){
			SocketHandler* handler=(SocketHandler*)(events[i].data.ptr);
			handler->onEvent();
		}
	}

	close(epoll_fd);
	return 0;
}

