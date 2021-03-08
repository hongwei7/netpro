#ifndef __NETPRO_TCPCONN_H
#define __NETPRO__TCPCONN__H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "epoll.h"
#include "dbg.h"

char buffer[BUFSIZ];

class tcpconn : noncopyable {
public:
	tcpconn(int file, void(*raction)(char*, int, tcpconn*), epoll* tree)
		: fd(file), readAction(raction), ev(tree, file, (void*)this) {
		dbg("tcpconn created");
	};

	int tcpconnRead() {
		while (1) {
			memset(buffer, 0, sizeof(buffer));
			dbg("enter read loop");
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
				exit(-1);
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
};



#endif
