#ifndef __NETPRO__HTTPCONN__H
#define __NETPRO__HTTPCONN__H

#include <assert.h>
#include "server.h"
#include "tcpconn.h"

class httpconn{
public:
    httpconn(int file){}

    void httpClose(){};
    void process(){
        char httpres[] = "HTTP/1.1 200 OK\r\nDate: Sat, 31 Dec 2005 23:59:59 GMT\r\nContent-Type: text/html;charset=ISO-8859-1\r\n\r\n<html><head><title>TEST</title></head><body>HELLO</body></html>\n";
        strcpy(mWriteBuf, httpres);
        bytesToSend = strlen(mWriteBuf) + 1;
    };
    int readOnce() override {
        while (mReadIdx < READ_BUFFER_SIZE) {
			int readSize = read(fd, mReadBuf + mReadIdx, READ_BUFFER_SIZE - mReadIdx);
			// dbg(readSize);
            mReadIdx += readSize;
			if (readSize > 0) {
                continue;
			}
			else if (readSize == 0) {
				return 0;                    //close
			}
			else if (errno == EWOULDBLOCK || errno == EAGAIN) {
				return 1;
			}
			else {
				perror("read");
				dbg(errno);
				unlock();
				return -1;
			}
		}
        process();
    };
    int doWrite() override {

		needWrite.wait();

		int writeSize = write(fd, mWriteBuf, bytesToSend);
		//assert(writeSize >= 0);
		return 0;
    };

public:
    static const int FILE_NAME_LEN = 200;
    static const int WRITE_BUFFER_SIZE = BUFSIZ;
    static const int READ_BUFFER_SIZE = BUFSIZ;
    enum METHOD{GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH};
    enum CHECK_STATE{CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};
    enum HTTP_CODE{NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};
    enum LINE_STATUS{LINE_OK = 0, LINE_OPEN};

private:
    void init(){
        memset(mReadBuf, 0, sizeof(mReadBuf));
        memset(mWriteBuf, 0, sizeof(mWriteBuf));
        mReadIdx = 0;
        mCheckedIdx = 0;
        mStartLine = 0;
        mWriteIdx = 0;

    }
    HTTP_CODE processRead(){}
    bool processWrite(HTTP_CODE ret){}
    HTTP_CODE parseRequestLine(char* text){}
    HTTP_CODE parseHeaders(char* text){}
    HTTP_CODE parseContent(char* text){}
    HTTP_CODE doRequest(){}

    char* getLine(){return mReadBuf + mStartLine;} //后移指针
    LINE_STATUS parseLine(){};                     //分析请求报文的部分

    //doRequest 中的函数
    bool addResponse(const char* format, ...);
    bool addContent(const char* content);
    bool addStatusLine(int status, const char* title);
    bool addHeaders(int contentLength);
    bool addContentType();
    bool addContentLength();
    bool addLinger();
    bool addBlankLine();

private:
    int fd;
    char mReadBuf[READ_BUFFER_SIZE];               //储存请求报文数据
    int mReadIdx;                                  //缓冲区数据最后一个字节的下一个位置
    int mCheckedIdx;                               //已经读取的位置
    int mStartLine;                                //已经解析的字符个数

    char mWriteBuf[WRITE_BUFFER_SIZE];             //响应缓冲区
    int mWriteIdx;                                 //缓冲区数据长度

    CHECK_STATE mCheckState;                       //现在的状态机状态
    METHOD mMethod;                                //请求方法

    char mRealFile[FILE_NAME_LEN];
    char* mUrl, mVersion, mHost, mContentLength;
    bool mLinger;

    char* mFileAddress;
    int cgi;                                       //是否启用 POST
    char* mString;                                 //请求头数据
    int bytesToSend;
    int bytesHaveSend;

};

#endif