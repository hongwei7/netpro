#ifndef __NETPRO__EPOLL__H
#define __NETPRO__EPOLL__H

#include <sys/epoll.h>
#include <assert.h>
#include <string.h>
#include <memory>
#include "dbg.h"
#include "noncopyable.h"
#include "threadPool.h"
#include "tcpconn.h"

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

template<typename T>
class event {
public:
    event(epoll* ep, int cli, T* wk, bool sock)
        : epollTree(ep), cliFd(cli), tcpPtr(wk), sharedPtr(new std::shared_ptr<event<T>>(this)){
        memset(&event_impl, 0, sizeof(event_impl));
        if(sock) event_impl.events = EPOLLIN;
        else event_impl.events = EPOLLIN | EPOLLOUT | EPOLLET; 
        event_impl.data.fd = cli;
        int ret = epoll_ctl(epollTree->getepfd(), EPOLL_CTL_ADD, cliFd, &event_impl);
        if (ret == -1) {
            perror("create event");
            exit(-1);
        }
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
    ~event(){
        dbg("EVENT DECRACE");
        delete tcpPtr;
    }

public:
    T* tcpPtr;
    std::shared_ptr<event<T>>* sharedPtr;

private:
    struct epoll_event event_impl;
    epoll* epollTree;
    int cliFd;
};

#endif
