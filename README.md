# netpro
epoll+ET触发模式+线程池实现Web服务器。 [<img alt="Build Status" src="https://api.travis-ci.org/hongwei7/netpro.svg?branch=master" height="20">][travis-url]

技术特点：
- 采用Reactor模型和epoll的ET边缘触发模式，更充分发挥I/O复用模型的效率。
- 使用线程池技术，降低了多线程创建和销毁所带来的额外资源消耗。
- 封装 Linux 的系统函数，采用面向对象技术中的RAII思想，资源可以更自然地释放。
- 使用有限状态机解析HTTP请求。
