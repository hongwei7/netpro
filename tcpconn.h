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
		tcpconn* cliTcp = even->tcpPtr;

		// dbg(cliev->events & EPOLLIN);
		// dbg(cliev->events & EPOLLOUT);

		dbg("READING EVENT");

		if (cliev->events & EPOLLIN) {
			dbg("EPOLLIN");
			cliTcp->tcpLock.lock();
			int ret = cliTcp->tcpconnRead();
			cliTcp->tcpLock.unlock();
			if(ret == 0){
				//关闭连接
				dbg("CLIENT EXIT");
				cliTcp->tcpLock.lock();
				delete even->sharedPtr;
				cliTcp->tcpLock.unlock();
				return NULL;
			}
			else if(ret == -1)return NULL;
		}

		if(cliev->events & EPOLLOUT){
			dbg("EPOLLOUT");
			// dbg("BEFORE WRITE");
			cliTcp->tcpLock.lock();
			int ret = cliTcp->tcpconnWrite();
			cliTcp->tcpLock.unlock();
			// dbg("BEHIND WRITE");
		}
		dbg(even->sharedPtr->use_count());
		cliTcp->tcpLock.lock();
		delete even->sharedPtr;
		cliTcp->tcpLock.unlock();
		dbg("LOOP END");
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
				tcpLock.unlock();
				pthread_exit(NULL);
			}
		}
	}

	int tcpconnWrite() {
		int size = 0;
		char* writeBuffer =  writeAction(&size, this);
		int writeSize = write(fd, writeBuffer, size);
		dbg("WRITE");
		if(writeSize == -1)perror("write");
		assert(writeSize >= 0);
		return 0;
	}
	void lock(){
		if(tcpLock.lock() != 0){
			pthread_exit(NULL);
		}
	}
	void unlock(){
		if(tcpLock.unlock() != 0){
			pthread_exit(NULL);
		}
	}
	~tcpconn(){
		// dbg(close(fd));
	}


private:
	int fd;
	void (*readAction)(char* buf, int size, tcpconn* wk);
	char* (*writeAction)(int* size, tcpconn* wk);
	mutex* listMutex;
	mutex tcpLock;
};



#endif
