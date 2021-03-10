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

const int TRY_LOCK_WAIT_TIME = 1000;    //申请锁失败时睡眠1ms
int CLIENT_DONE = 0;

class tcpconn : noncopyable {
public:
	tcpconn(int file, void(*raction)(char*, int, tcpconn*), char*(*waction)(int*, tcpconn*), 
	epoll* tree)
	: fd(file), readAction(raction), writeAction(waction),
	ev(tree, file, (void*)this), closing(false), count(0)
	{} 

	int tcpconnRead() {
		char buffer[BUFSIZ];
		while (1) {
			memset(buffer, 0, sizeof(buffer));
			// dbg("enter read loop");
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
		if (cliTcp->ev.getEvent() & EPOLLIN) {
			if (cliTcp->tcpconnRead() == 0) {
				cliTcp->closing = true;
			}
		}
		if(cliTcp->ev.getEvent() & EPOLLOUT) {
			int ret = cliTcp->tcpconnWrite();
			assert(ret == 0);
		}
		cliTcp->tcpLock.unlock();
	}

	const int getfd() const { return fd; }
	const int operator*() const { return getfd(); }
	void setReadOnly(){
		ev.hangRD();
	}
	void setWrite(){
		ev.hangWR();
	}
	void waitToClose(){
		closing = true;
	}

	~tcpconn() {
		ev.destroy();
		close(fd);
	}

public:
	mutex tcpLock;
	int count;

private:
	int fd;
	void (*readAction)(char* buf, int size, tcpconn* wk);
	char* (*writeAction)(int* size, tcpconn* wk);
	event ev;
	bool closing;
};



#endif
