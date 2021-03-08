#ifndef __NETPRO__EPOLL__H
#define __NETPRO__EPOLL__H

#include <sys/epoll.h>
#include <assert.h>
#include <string.h>
#include "noncopyable.h"

const int EPOLL_INIT_SIZE = 5;
const int MAX_CLIENTS = 60000;

class epoll: noncopyable{
public:
    epoll(): epfd(epoll_create(EPOLL_INIT_SIZE)){
        assert(epfd > 0);
    };
    const int getfd(){return epfd;}
    struct epoll_event eventsList[MAX_CLIENTS];
private:
    int epfd;
};

class event: noncopyable{
public:
    event() = delete;
    explicit event(epoll* ep, int cli, void*(*worker)(void*))
    :epollTree(ep), cliFd(cli){
        event_impl.data.ptr = (void*) worker;
        epoll_ctl(epollTree->getfd(), EPOLL_CTL_ADD, cliFd, &event_impl);
    }
    void hangRD(){
        event_impl.events = EPOLLIN;
        int ret = epoll_ctl(epollTree->getfd(), EPOLL_CTL_MOD, cliFd, &event_impl);
        assert(ret == 0);
    }
    void hangWR(){
        event_impl.events = EPOLLOUT;
        int ret = epoll_ctl(epollTree->getfd(), EPOLL_CTL_MOD, cliFd, &event_impl);
        assert(ret == 0);
    }
    const int getfd() const{ return cliFd;}
    virtual ~event(){
        int ret = epoll_ctl(epollTree->getfd(), EPOLL_CTL_DEL, cliFd, &event_impl);
        assert(ret == 0);
    }

private:
    struct epoll_event event_impl;
    epoll* epollTree;
    int cliFd;
};

#endif