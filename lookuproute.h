#ifndef __FIND__
#define __FIND__
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include <arpa/inet.h>
#include <string>

struct route
{
    struct route *next;
    struct in_addr ip4prefix;
	unsigned int prefixlen;
    struct nexthop *nexthop;
};

struct nexthop
{
   //struct nexthop *next;
   std::string ifname;
   unsigned int ifindex;//zlw ifindex2ifname()获取出接�?   // Nexthop address 
   struct in_addr nexthopaddr;
   nexthop();
   nexthop(const char* ifname, unsigned int ifindex, unsigned long nexthop_addr);
   ~nexthop();
};

typedef nexthop nextaddr;
/*
struct nextaddr
{
   char *ifname;
   struct in_addr ipv4addr;
   unsigned int prefixl;
};
*/

extern struct route *route_table; 
int insert_route(unsigned long  ip4prefix,unsigned int prefixlen,const char *ifname,unsigned int ifindex,unsigned long  nexthopaddr);
int lookup_route(struct in_addr dstaddr,nextaddr *nexthopinfo);
int delete_route(struct in_addr dstaddr,unsigned int prefixlen);

#endif
