#ifndef __NETPRO__NONCOPYABLE__H
#define __NETPRO__NONCOPYABLE__H

class noncopyable{
protected:
    noncopyable() = default;
    ~noncopyable() = default;
public:
    noncopyable(const noncopyable& other) = delete;
    const noncopyable& operator=(const noncopyable& other) = delete;
};

#endif
