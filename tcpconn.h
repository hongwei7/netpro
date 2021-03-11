#ifndef __NETPRO__TCPCONN__H
#define __NETPRO__TCPCONN__H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "epoll.h"
#include "dbg.h"

void* dealWithClient(void*);
void closeTcpconn(void*);

// const int TRY_LOCK_WAIT_TIME = 1000;    //申请锁失败时睡眠1ms
int CLIENT_DONE = 0;

class tcpconn : noncopyable {
public:
	tcpconn(int file, void(*raction)(char*, int, tcpconn*), char*(*waction)(int*, tcpconn*), 
	epoll* tree, mutex* tlm)
	: fd(file), readAction(raction), writeAction(waction), listMutex(tlm){ } 

	friend void* dealWithClient(void* ev_) {
		dbg("DEALWITHCLIENT");
		epoll_event* cliev = static_cast<epoll_event*>(ev_);
		event<tcpconn>* even = (event<tcpconn>*)cliev->data.ptr;
		std::shared_ptr<tcpconn> cliTcp = (even)->tcpPtr;
		// dbg(cliev->events & EPOLLIN);
		// dbg(cliev->events & EPOLLOUT);


		if (cliev->events & EPOLLIN) {
			dbg("EPOLLIN");
			// if(cliTcp->closing)return NULL;
			int ret = cliTcp->tcpconnRead();
			if(ret == 0){
				//关闭连接
				delete even;
				return NULL;
			}
			else if(ret == -1)return NULL;
		}

		if(cliev->events & EPOLLOUT){
			dbg("EPOLLOUT");
			// dbg("BEFORE WRITE");
			int ret = cliTcp->tcpconnWrite(even);
			// dbg(ret);
			// dbg("BEHIND WRITE");
		}

		return NULL;
	}

	const int getfd() const { return fd; }
	const int operator*() const { return getfd(); }

	int tcpconnRead() {
		char buffer[BUFSIZ];
		while (1) {
			memset(buffer, 0, sizeof(buffer));
			int readSize = read(fd, buffer, sizeof(buffer));
			// dbg(readSize);
			if (readSize > 0) {
				readAction(buffer, readSize, this);
			}
			else if (readSize == 0) {
				return 0;                    //close
			}
			else if (readSize == -1 && errno == EAGAIN) {
				return 1;
			}
			else {
				perror("read");
				dbg(errno);
				return -1;
			}
		}
	}

	int tcpconnWrite(event<tcpconn>* even) {
		int size = 0;
		char* writeBuffer =  writeAction(&size, this);
		int writeSize = write(fd, writeBuffer, size);
		if(writeSize == -1)perror("write");
		assert(writeSize >= 0);
		delete even;
		return 0;
	}
	~tcpconn(){
		close(fd);
	}

public:
	mutex tcpLock;
	bool closing;

private:
	int fd;
	void (*readAction)(char* buf, int size, tcpconn* wk);
	char* (*writeAction)(int* size, tcpconn* wk);
	mutex* listMutex;
};



#endif
