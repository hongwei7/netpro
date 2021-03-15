#ifndef __NETPRO__TCPCONN__H
#define __NETPRO__TCPCONN__H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <map>
#include <signal.h>
#include "sem.h"
#include "http.h"
#include "epoll.h"
#include "dbg.h"

void* dealWithClientRead(void*);
void* dealWithClientWrite(void*);
void closeTcpconn(void*);

const int TIME_WAIT_SEC = 5;

class tcpconn : noncopyable {
public:
	tcpconn(int file): fd(file) { 
		memset(&httpInfo, 0, sizeof(httpInfo));
	} 

	friend void* dealWithClientRead(void* cli_fd) {
		// auto tcpMap = tcpMap;
		// auto listMutex = listMutex;
		int clifd = *((int*)cli_fd);
		listMutex->lock();
		auto iter = tcpMap->find(clifd);
		std::shared_ptr<event<tcpconn>> even;
		if(iter == tcpMap->end()){
			listMutex->unlock();
			forwardRead.signal();
			return NULL;
		}
		else {
			even = (*iter).second;
		}
		listMutex->unlock();
		dbg("DEALWITHCLIENT READ");

		auto cliTcp = even->tcpPtr; 
		forwardRead.signal();

		int ret = cliTcp->readOnce();
		dbg("SIGNAL");

		if(ret == 0){
			//关闭连接
			dbg("CLIENT EXIT");
			listMutex->lock();
			tcpMap->erase(clifd);
			listMutex->unlock();

			return NULL;
		}
		else if(ret == -1)return NULL;

		dbg("READ LOOP END");
		return NULL;
	}
	friend void* dealWithClientWrite(void* cli_fd){
		dbg("DEALWITHCLIENT WRITE");
		int clifd = *((int*)cli_fd);
		// auto listMutex = listMutex;
		// auto tcpMap = tcpMap;

		listMutex->lock();
		auto iter = tcpMap->find(clifd);
		std::shared_ptr<event<tcpconn>> even;
		if(iter == tcpMap->end()){
			dbg("DEALWITHCLIENT WRITE OUT");
			listMutex->unlock();
			forwardWrite.signal();
			return NULL;
		}
		else {
			even = (*iter).second;
		}
		listMutex->unlock();

		auto cliTcp = even->tcpPtr; 

		assert(even.use_count() != 0);

		listMutex->lock();
		tcpMap->erase(clifd);
		listMutex->unlock();

		forwardWrite.signal();

		int ret = cliTcp->doWrite();

		dbg("WRITE LOOP END");
		return NULL;
	}

	const int getfd() const { return fd; }
	const int operator*() const { return getfd(); }

	int readOnce() {
		char buffer[BUFSIZ];
		while (1) {
			char readBuf[BUFSIZ];
			int readSize = read(fd, httpInfo.mReadBuf + httpInfo.mReadIdx, sizeof(buffer));
			httpInfo.mReadIdx += readSize;
			dbg(readSize);
			if (readSize > 0) {
				break;
			}
			else if (readSize == 0) {
				return 0;                    //close
			}
			else if (errno == EWOULDBLOCK || errno == EAGAIN) {
				break;
			}
			else {
				perror("read");
				dbg(errno);
				unlock();
				return -1;
			}
		}
		process();
		needWrite.signal();
		return 1;
	}

	int doWrite() {
		dbg("WRITE-ACTION");
		assert(fcntl(fd, F_GETFL));
		needWrite.wait();
		dbg("WRITE");
		int writeSize = write(fd, httpInfo.mWriteBuf, httpInfo.bytesToSend);
		//assert(writeSize >= 0);
		dbg("AFTER-WRITE");
		return 0;
	}

	void lock(){
		if(tcpLock.lock() != 0){
			listMutex->unlock();
			unlock();
			pthread_exit(NULL);
		}
	}
	void unlock(){
		if(tcpLock.unlock() != 0){
			listMutex->unlock();
			pthread_exit(NULL);
		}
	}
	~tcpconn(){
		listMutex->lock();
		tcpMap->erase(fd);
		dbg(close(fd));
		listMutex->unlock();
		free(http);
		dbg("------CONNECT OUT------");
	}

private:
	void process() {
		dbg("PROCESS");
		char httpres[] = "HTTP/1.1 200 OK\r\nDate: Sat, 31 Dec 2005 23:59:59 GMT\r\nContent-Type: text/html;charset=ISO-8859-1\r\n\r\n<html><head><title>TEST</title></head><body>HELLO</body></html>\n";
		strcpy(httpInfo.mWriteBuf, httpres);
		httpInfo.bytesToSend = strlen(httpInfo.mWriteBuf) + 1;
	};

public:
	static mutex* listMutex;
	static std::map<int, std::shared_ptr<event<tcpconn>>> *tcpMap;
	static sem forwardRead, forwardWrite;
	sem needWrite;
	httpconn* http;
	

private:
	int fd;
	mutex tcpLock;
	httpconn httpInfo;
};

std::map<int, std::shared_ptr<event<tcpconn>>> * tcpconn::tcpMap = nullptr;
mutex* tcpconn::listMutex = nullptr;
sem tcpconn::forwardRead, tcpconn::forwardWrite;

#endif
