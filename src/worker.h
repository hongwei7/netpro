#ifndef __NETPRO__WORKER__H
#define __NETPRO__WORKER__H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "epoll.h"
#include "dbg.h"

char buffer[BUFSIZ];

enum worktype{WLISTEN = 1, WWRITE};


class worker{
public:
    worker(int file, worktype type, void(*raction)(char*,int, int), epoll* tree)
    : fd(file), wt((worktype)type), readAction(raction), ev(tree, file, (void*)this){
        dbg("worker created");
    };

    int workerRead(){
        while(1){
            int readSize = read(fd, buffer, BUFSIZ);
            if(readSize > 0){
                readAction(buffer, BUFSIZ, fd);
            }
            else if(readSize == 0){
                close(fd);
                return 0;                    //close
            }
            else if(readSize == EAGAIN){
                return 1;
            }
        }
    }
    const int getfd() const {return fd;}
    const int operator*() const {return getfd(); }
    bool operator==(const worker& other){
        return this->fd == other.fd;
    }

private:
    int fd;
    worktype wt;
    void (*readAction)(char* buf, int size, int fd);
    void (*writeAction)(char* buf, int size, int fd);
    event ev;
};



#endif