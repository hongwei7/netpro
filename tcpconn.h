#ifndef __NETPRO_TCPCONN_H
#define __NETPRO__TCPCONN__H

#include <stdio.h>
#include <list>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "epoll.h"
#include "dbg.h"
#include <memory>

void* dealWithClient(void*);
void closeTcpconn(void*);

class tcpconn : noncopyable {
public:
	tcpconn(int file, void(*raction)(char*, int, tcpconn*), char*(*waction)(int*, tcpconn*), 
		epoll* tree, std::list<tcpconn*>* li, mutex* litx)
	: fd(file), readAction(raction), writeAction(waction),
	ev(tree, file, (void*)this), tcplist(li), tcplistMutex(litx), closing(false)
	{} 

	int tcpconnRead() {
		char buffer[BUFSIZ];
		while (1) {
			memset(buffer, 0, sizeof(buffer));
			// dbg("enter read loop");
			int readSize = read(fd, buffer, sizeof(buffer));
			dbg(readSize);
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
				return -1;
			}
		}
	}

	int tcpconnWrite() {
		int size = 0;
		char* writeBuffer =  writeAction(&size, this);
		int writeSize = write(fd, writeBuffer, size); 
		assert(writeSize >= 0);
		return 0;
	}

	friend void* dealWithClient(void* cli) {
		tcpconn* cliTcp = static_cast<tcpconn*>(cli);
		cliTcp->tcpLock.lock();
		if(cliTcp->closing)return NULL;
		if (cliTcp->ev.getEvent() & EPOLLIN) {
			if (cliTcp->tcpconnRead() == 0) {
				closeTcpconn(cliTcp);
				return NULL;
			}
		}
		if(cliTcp->ev.getEvent() & EPOLLOUT) {
			int ret = cliTcp->tcpconnWrite();
			assert(ret == 0);
		}
		if(cliTcp->closing){
			closeTcpconn(cli);
			return NULL;
		}
		cliTcp->tcpLock.unlock();
	}

	const int getfd() const { return fd; }
	const int operator*() const { return getfd(); }
	bool operator==(const tcpconn& other) {
		return this->fd == other.fd;
	}
	void ListenRead(){
		ev.hangRD();
	}
	void ListenWrite(){
		ev.hangWR();
	}
	void tryClose(){
		closing = true;
	}
	friend void closeTcpconn(void* cli){
		dbg("client exiting");
		tcpconn* cliTcp = (tcpconn*) cli;
		mutex* lock = cliTcp->tcplistMutex;
		lock->lock();
		cliTcp->tcplist->remove(cliTcp);
		delete cliTcp;
		lock->unlock();
	}
	~tcpconn() {
		ev.destroy();
		close(fd);
	}

private:
	int fd;
	void (*readAction)(char* buf, int size, tcpconn* wk);
	char* (*writeAction)(int* size, tcpconn* wk);
	event ev;
	std::list<tcpconn*>* tcplist;
	mutex* tcplistMutex;
	mutex tcpLock;
	bool closing;
};



#endif
