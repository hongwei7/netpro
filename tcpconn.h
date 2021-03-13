#ifndef __NETPRO__TCPCONN__H
#define __NETPRO__TCPCONN__H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <map>
#include "epoll.h"
#include "dbg.h"

void* dealWithClientRead(void*);
void* dealWithClientWrite(void*);
void closeTcpconn(void*);

// const int TRY_LOCK_WAIT_TIME = 1000;    //申请锁失败时睡眠1ms
int CLIENT_DONE = 0;


class tcpconn : noncopyable {
public:
	tcpconn(int file, void(*raction)(char*, int, tcpconn*), int(*waction)(char*, int, tcpconn*), 
	epoll* tree)
	: fd(file), readAction(raction), writeAction(waction) { } 

	friend void* dealWithClientRead(void* cli_fd) {
		// auto tcpMap = tcpMap;
		// auto listMutex = listMutex;
		int clifd = *((int*)cli_fd);
		listMutex->lock();
		auto iter = tcpMap->find(clifd);
		std::shared_ptr<event<tcpconn>> even;
		if(iter == tcpMap->end()){
			listMutex->unlock();
			return NULL;
		}
		else {
			even = (*iter).second;
		}
		listMutex->unlock();
		dbg("DEALWITHCLIENT READ");

		auto cliTcp = even->tcpPtr; 
		cliTcp->lock();
		int ret = cliTcp->tcpconnRead();
		// // if(!cliTcp->fin)cliTcp->fin = true;
		// else{ 
		// 	listMutex->lock();
		// 	tcpMap->erase(clifd);
		// 	listMutex->unlock();
		// }
		if(ret == 0){
			//关闭连接
			dbg("CLIENT EXIT");
			listMutex->lock();
			tcpMap->erase(clifd);
			listMutex->unlock();

			return NULL;
		}
		else if(ret == -1)return NULL;
		cliTcp->unlock();
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
			return NULL;
		}
		else {
			even = (*iter).second;
		}
		listMutex->unlock();
		auto cliTcp = even->tcpPtr; 
		assert(even.use_count() != 0);

		cliTcp->lock();
		// if(!cliTcp->fin)cliTcp->fin = true;
		// else{ 
		listMutex->lock();
		tcpMap->erase(clifd);
		listMutex->unlock();
		// }
		int ret = cliTcp->tcpconnWrite();
		cliTcp->unlock();
		dbg("WRITE LOOP END");
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
				unlock();
				return -1;
			}
		}
	}

	int tcpconnWrite() {
		dbg("WRITE-ACTION");
		assert(fcntl(fd, F_GETFL));
		char writeBuffer[BUFSIZ];

		int size = writeAction(writeBuffer, BUFSIZ, this); //error

		dbg("WRITE");
		int writeSize = write(fd, writeBuffer, size);
		assert(writeSize >= 0);
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
		tcpMap->erase(fd) == 0;
		dbg(close(fd));
		listMutex->unlock();
		dbg("------CONNECT OUT------");
	}

public:
	static mutex* listMutex;
	static std::map<int, std::shared_ptr<event<tcpconn>>> *tcpMap;
	// bool fin = false;

private:
	int fd;
	void (*readAction)(char* buf, int size, tcpconn* wk);
	int (*writeAction)(char* buf, int size, tcpconn* wk);
	mutex tcpLock;
};

std::map<int, std::shared_ptr<event<tcpconn>>> * tcpconn::tcpMap = nullptr;
mutex* tcpconn::listMutex = nullptr;

#endif
