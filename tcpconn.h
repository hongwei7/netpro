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

class tcpconn : noncopyable {
public:
	tcpconn(int file, void(*raction)(char*, int, tcpconn*), epoll* tree, std::list<tcpconn*>* li, mutex* litx)
		: fd(file), readAction(raction), ev(tree, file, (void*)this), tcplist(li), tcplistMutex(litx){};

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
	friend void* dealWithClient(void* cli) {
		tcpconn* cliTcp = static_cast<tcpconn*>(cli);
		if (cliTcp->ev.getEvent() & EPOLLIN) {
			if (cliTcp->tcpconnRead() == 0) {
				dbg("client exiting");
				mutex* lock = cliTcp->tcplistMutex;
				lock->lock();
				cliTcp->tcplist->remove(cliTcp);
				delete cliTcp;
				lock->unlock();
			}
		}
	}

	const int getfd() const { return fd; }
	const int operator*() const { return getfd(); }
	bool operator==(const tcpconn& other) {
		return this->fd == other.fd;
	}
	~tcpconn() {
		ev.destroy();
		close(fd);
	}

private:
	int fd;
	void (*readAction)(char* buf, int size, tcpconn* wk);
	void (*writeAction)(char* buf, int size, tcpconn* wk);
	event ev;
	std::list<tcpconn*>* tcplist;
	mutex* tcplistMutex;
};



#endif
