#include "lookuproute.h"
#include <vector>

struct route *route_table;


class TrieRoute {

private:
    struct Node{
        Node(){
            val=nullptr;
            next[0]=next[1]=nullptr;
        }
        nexthop* val;
        Node* next[2];
    };
    Node* root;

public:
    TrieRoute(){
        root=new Node();
    }
    int query(unsigned long prefix, nexthop* ret){
        unsigned char* caster=(unsigned char*)(&prefix);
        Node* iter=root;
        nexthop* val=root->val;
        printf("%d\n", prefix);
        //Can be manually unrolled to speed up
        for(int i=0;i<4;i++){
            for(int j=7;j>=0;j--){
                //printf("%d\n", iter);
                
                int dir=(caster[i]>>j)&1;
                printf("addr: %d %d next: %d %d\n", iter, dir, iter->next[0], iter->next[1]);
                iter=iter->next[dir];
                if(iter==nullptr){
                    goto query_done;
                }else{
                    printf("addr2: %d\n", iter);
                    if(iter->val!=nullptr) val=iter->val;
                }
            }
        }
        query_done:
        if(val!=nullptr){
            *ret=*val;
            return 0;
        }else{
            return -1;
        }
    }
    void insert(unsigned long prefix, unsigned int prefixlen, nexthop* val){
        if(prefixlen==0){
            root->val=new nexthop();
            (*root->val)=*val;
            return;
        }
        printf("Inserting %d %d\n", prefix, prefixlen);
        unsigned char* caster=(unsigned char*)(&prefix);
        Node* iter=root;
        for(int i=0;i<4;i++){
            for(int j=7;j>=0;j--){
                int dir=(caster[i]>>j)&1;
                if(iter->next[dir]==nullptr){
                    iter->next[dir]=new Node();
                }
                iter=iter->next[dir];
                prefixlen--;
                if(prefixlen==0) goto insert_done;
            }

        }
        insert_done:
        if(iter->val!=nullptr) delete iter->val;
        iter->val=new nexthop();
        *(iter->val)=*(val);

    }
    void remove(unsigned long prefix, unsigned int prefixlen){
        if(prefixlen==0){
            if(root->val!=nullptr){
                delete root->val;
                root->val=nullptr;
            }
            return;
        }
        unsigned char* caster=(unsigned char*)(&prefix);
        Node* iter=root;
        Node* parent=root;
        int plen=prefixlen;
        std::vector<Node*> chain;
        std::vector<int> digits;
        chain.reserve(34);
        chain.push_back(root);
        digits.reserve(34);     
        for(int i=0;i<4;i++){
            for(int j=7;j>=0;j--){
                int dir=(caster[i]>>j)&1;
                iter=iter->next[dir];
                if(iter==nullptr){
                    return;
                }
                digits.push_back(dir);
                chain.push_back(iter);
                prefixlen--;
                if(prefixlen==0) goto remove_done;
            }

        }
        remove_done:
        delete chain[plen]->val;
        chain[plen]->val=nullptr;
        for(int i=plen;i>0;i--){
            Node* curr=chain[i];
            Node* parent=chain[i-1];
            int dir=digits[i-1];
            if(curr->next[0]==nullptr && curr->next[1]==nullptr && curr->val==nullptr){
                parent->next[dir]=nullptr;
                delete curr;
            }else{
                break;
            }
        }
    }

};
TrieRoute router;
nexthop::nexthop(){

}
nexthop::nexthop(char* ifname, unsigned int ifindex, unsigned long nexthop_addr){
    this->ifname=std::string(ifname);
    this->ifindex=ifindex;
    this->nexthopaddr.s_addr=nexthop_addr;
}
nexthop::~nexthop(){
    //delete ifname;
}
int insert_route(unsigned long  ip4prefix,unsigned int prefixlen,char *ifname,unsigned int ifindex,unsigned long  nexthopaddr)
{
    nexthop addr(ifname, ifindex, nexthopaddr);
	router.insert(ip4prefix, prefixlen, &addr);
    return 0;
			
}

int lookup_route(struct in_addr dstaddr,nextaddr *nexthopinfo)
{
    printf("Routing...");
	return router.query(dstaddr.s_addr, nexthopinfo);
}

int delete_route(struct in_addr dstaddr,unsigned int prefixlen)
{
	router.remove(dstaddr.s_addr, prefixlen);
    return 0;
}

