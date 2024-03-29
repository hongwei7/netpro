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

const int TIME_WAIT_SEC = 5;

class tcpconn : noncopyable {
public:
	tcpconn(int file): fd(file) { 
	} 

	friend void* dealWithClientRead(void* cli_fd) {
		int clifd = *((int*)cli_fd);
		listMutex->lock();
		auto iter = connectMap->find(clifd);
		std::shared_ptr<event<tcpconn>> even;
		if(iter == connectMap->end()){
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
			connectMap->erase(clifd);
			listMutex->unlock();

			return NULL;
		}
		else if(ret == -1){
			return NULL;
		}
		dbg("READ LOOP END");
		return NULL;
	}
	friend void* dealWithClientWrite(void* cli_fd){
		dbg("DEALWITHCLIENT WRITE");
		int clifd = *((int*)cli_fd);

		listMutex->lock();
		auto iter = connectMap->find(clifd);
		std::shared_ptr<event<tcpconn>> even;
		if(iter == connectMap->end()){
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


		listMutex->lock();
		connectMap->erase(clifd);
		listMutex->unlock();

		forwardWrite.signal();

		int ret = cliTcp->doWrite();

		dbg("WRITE LOOP END");
		return NULL;
	}

	const int getfd() const { return fd; }
	const int operator*() const { return getfd(); }

	int readOnce() {
		while (1) {
			int readSize = read(fd, httpInfo.mReadBuf + httpInfo.mReadIdx, sizeof(httpInfo.mReadBuf));
			httpInfo.mReadIdx += readSize + 1;
			dbg(readSize);
			if (readSize > 0) {
				continue;
			}
			else if (readSize == 0) {
				return 0;  
			}				
			else if (errno == EWOULDBLOCK || errno == EAGAIN) {
				break;
			}
			else {
				perror("read");
				dbg(errno);
				return -1;
			}
		}
		process();
		return 1;
	}

	int doWrite() {
		dbg("WRITE-ACTION");
		assert(fcntl(fd, F_GETFL));
		if(needWrite.timeWait(TIME_WAIT_SEC, 0) == 1)return 0;
		dbg("WRITE");
		int writeSize = write(fd, httpInfo.mWriteBuf, httpInfo.bytesToSend);
		httpInfo.bytesHaveSend += writeSize;
		dbg("AFTER-WRITE");
		return 0;
	}

	~tcpconn(){
		listMutex->lock();
		connectMap->erase(fd);
		dbg(close(fd));
		listMutex->unlock();
		dbg("------CONNECT OUT------");
	}

private:
	void process() {
		dbg("PROCESS");

	 	HTTP_CODE readRet = dbg(httpInfo.processRead());
		if(readRet == NO_REQUEST)return;           		//继续获取报文
		dbg("PROCESS READ END");
		bool writeRet = httpInfo.processWrite(readRet);

		listMutex->lock();
		(*connectMap)[fd]->addWrite();
		listMutex->unlock();

		needWrite.signal();

		return;
	};

	void closeConn(){
		listMutex->lock();
		connectMap->erase(fd);
		listMutex->unlock();
	}

public:
	static mutex* listMutex;
	static std::map<int, std::shared_ptr<event<tcpconn>>> *connectMap;
	static sem forwardRead, forwardWrite;
	sem needWrite;
	httpconn http;
	

private:
	int fd;
	httpconn httpInfo;
};

std::map<int, std::shared_ptr<event<tcpconn>>> * tcpconn::connectMap = nullptr;
mutex* tcpconn::listMutex = nullptr;
sem tcpconn::forwardRead, tcpconn::forwardWrite;

#endif
