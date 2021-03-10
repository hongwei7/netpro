#ifndef __NETPRO__EPOLL__H
#define __NETPRO__EPOLL__H

#include <sys/epoll.h>
#include <assert.h>
#include <string.h>
#include "dbg.h"
#include "noncopyable.h"
#include "threadPool.h"

const int EPOLL_INIT_SIZE = 5;
const int MAX_CLIENTS = 60000;

class epoll : public noncopyable {
public:
    epoll() : epfd(epoll_create(EPOLL_INIT_SIZE)){
        assert(epfd > 0);
    };
    int getepfd() const { return epfd; }
    virtual ~epoll() {
        close(epfd);
    }

public:
    struct epoll_event eventsList[MAX_CLIENTS];

private:
    int epfd;
};

class event : public noncopyable {
public:
    event(epoll* ep, int cli, void* wk)
        : epollTree(ep), cliFd(cli){
        event_impl.events = EPOLLIN; 
        if (ep->getepfd() != cli)event_impl.events |= EPOLLET; //ET模式
        event_impl.data.ptr = (void*)wk;
        int ret = epoll_ctl(epollTree->getepfd(), EPOLL_CTL_ADD, cliFd, &event_impl);
        if (ret == -1) {
            perror("create event");
            exit(-1);
        }
        assert(ret == 0);
    }
    void hangRD() {           //只监听ET读
        event_impl.events = EPOLLIN | EPOLLET;
        int ret = epoll_ctl(epollTree->getepfd(), EPOLL_CTL_MOD, cliFd, &event_impl);
        if (ret == -1)perror("hangRD");
        assert(ret == 0);
    }
    void hangWR() {           //允许写
        event_impl.events |= EPOLLOUT | EPOLLET;
        int ret = epoll_ctl(epollTree->getepfd(), EPOLL_CTL_MOD, cliFd, &event_impl);
        assert(ret == 0);
    }
    const int getfd() const { return cliFd; }
    const int getEvent() const { return event_impl.events; }
    void destroy() {
        int ret = epoll_ctl(epollTree->getepfd(), EPOLL_CTL_DEL, cliFd, nullptr);
        if (ret == -1) {
            perror("delete epoll_event");
        }
        assert(ret == 0);
    }

private:
    struct epoll_event event_impl;
    epoll* epollTree;
    int cliFd;
};

#endif