#ifndef __NETPRO_TCPCONN_H
#define __NETPRO__TCPCONN__H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "epoll.h"
#include "dbg.h"
#include <memory>

void* dealWithClient(void*);
void closeTcpconn(void*);

// const int TRY_LOCK_WAIT_TIME = 1000;    //申请锁失败时睡眠1ms
int CLIENT_DONE = 0;

class tcpconn : noncopyable {
public:
	tcpconn(int file, void(*raction)(char*, int, tcpconn*), char*(*waction)(int*, tcpconn*), 
	epoll* tree, mutex* tlm)
	: fd(file), readAction(raction), writeAction(waction),
	ev(tree, file, (void*)this), closing(false), listMutex(tlm){ } 



	friend void* dealWithClient(void* ev_) {
		// dbg("DEALWITHCLIENT");
		epoll_event* cliev = static_cast<epoll_event*>(ev_);
		tcpconn* cliTcp = static_cast<tcpconn*>(cliev->data.ptr);
		// dbg(cliev->events & EPOLLIN);
		// dbg(cliev->events & EPOLLOUT);


		if (cliev->events & EPOLLIN) {
			// if(cliTcp->closing)return NULL;
			int ret = cliTcp->tcpconnRead();
			if(ret == 0){
				cliTcp->tcpLock.lock();
				cliTcp->ev.destroy();
				close(cliTcp->fd);
				cliTcp->closing = true; 
				cliTcp->tcpLock.unlock();
			}
			else if(ret == -1)return NULL;
		}

		if(cliev->events & EPOLLOUT){
			// dbg("BEFORE WRITE");
			int ret = cliTcp->tcpconnWrite();
			// dbg(ret);
			// dbg("BEHIND WRITE");
		}


		return NULL;
	}

	const int getfd() const { return fd; }
	const int operator*() const { return getfd(); }
private:
	int tcpconnRead() {
		char buffer[BUFSIZ];
		while (1) {
			memset(buffer, 0, sizeof(buffer));
			tcpLock.lock();
			if(closing){
				close(fd);
				tcpLock.unlock();
				return -1;
			}
			int readSize = read(fd, buffer, sizeof(buffer));
			tcpLock.unlock();
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

	int tcpconnWrite() {
		int size = 0;
		char* writeBuffer =  writeAction(&size, this);
		tcpLock.lock();
		listMutex->lock();
		if(!closing){
			int writeSize = write(fd, writeBuffer, size);
			if(writeSize == -1)perror("write");
			assert(writeSize >= 0);
		}
		ev.destroy();
		close(fd);
		closing = true;
		listMutex->unlock();
		tcpLock.unlock();
		return 0;
	}

public:
	mutex tcpLock;
	bool closing;

private:
	int fd;
	void (*readAction)(char* buf, int size, tcpconn* wk);
	char* (*writeAction)(int* size, tcpconn* wk);
	event ev;
	mutex* listMutex;
};



#endif
