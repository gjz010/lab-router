#pragma once
#include <sys/epoll.h>
class SocketHandler{
public:
	SocketHandler(){

	}
	SocketHandler(int fd);
	virtual void onEvent()=0;
	int bind(int epoll_fd);
	int unbind();
protected:
	int fd;
	int efd;
};
