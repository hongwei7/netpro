#ifndef __NETPRO__SERVER__H
#define __NETPRE__SERVER__H

#include<sys/socket.h>
#include "epoll.h"
#include<assert.h>
#include<sys/types.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include "worker.h"
#include "dbg.h"
#include "mutex.h"
#include <list>

void defaultRead(char* buf, int size, int fd){
	dbg(buf);
}
class server {
	public:
		server(int port, void(*raction)(char*, int, int)) 
			: sockfd(socket(AF_INET, SOCK_STREAM, 0)), readAction(raction){
				if(readAction == nullptr)readAction = defaultRead;
				assert(sockfd != 0);
				dbg(sockfd);
				dbg(epolltree.getepfd());
				memset(&servAddr, 0, sizeof(servAddr));
				memset(&cliAddr, 0, sizeof(cliAddr));
				servAddr.sin_family = AF_INET;
				servAddr.sin_port = htons(port);
				servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
				int opt = 1;
				setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
				assert(bind(sockfd, (sockaddr*)&servAddr, sizeof(servAddr)) == 0);
				assert(listen(sockfd, 128) == 0);
				workersListMutex.lock();
				workersList.push_back(new worker(sockfd, WLISTEN, nullptr, &epolltree));
				workersListMutex.unlock();
			}
		void mainloop(){
			while(true){

				dbg("epoll wait");

				int ret = epoll_wait(epolltree.getepfd(), epolltree.eventsList, MAX_CLIENTS, -1);
				assert(ret > 0);
				for(int i = 0; i < ret; ++i){
					dbg("deal with event");
					dbg(ret);
					worker* ptr = (worker*)epolltree.eventsList[i].data.ptr;
					dbg(ptr->getfd() == sockfd);
					if(ptr->getfd() == sockfd) newClient();
					else{
						int excuteRes = ptr->workerRead();
						if(excuteRes == 0){ //close
							workersListMutex.lock();
							workersList.remove(ptr);
							workersListMutex.unlock();
							close(ptr->getfd());
							delete ptr;
						}
					}
				}
			}
		}
		void newClient(){
			dbg("newClient");
			socklen_t socksize = sizeof(cliAddr);
			int clifd = accept(sockfd, (sockaddr*)&cliAddr, &socksize);
			dbg(clifd);
			assert(clifd > 0);
			workersListMutex.lock();
			workersList.push_back(new worker(clifd, WLISTEN, readAction, &epolltree));
			workersListMutex.unlock();
		}

		virtual ~server(){
			workersListMutex.lock();
			for(auto & cli: workersList){
				close(cli->getfd());
			}   
			workersListMutex.unlock();
		}

	private:
		int sockfd;
		struct sockaddr_in servAddr, cliAddr;
		epoll epolltree;
		void(*readAction)(char*, int, int);
		std::list<worker*> workersList;
		mutex workersListMutex;
};

#endif
