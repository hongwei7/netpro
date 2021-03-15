#ifndef __NETPRO__HTTPCONN__H
#define __NETPRO__HTTPCONN__H

#include "tcpconn.h"

class httpconn: public tcpconn{
public:
    httpconn() = default;
    ~httpconn() = default;

    void httpClose(){};
    void process(){};
    virtual int readOnce() override {};
    virtual int doWrite() override {};

public:
    static const int FILE_NAME_LEN = 200;
    static const int WRITE_BUFFER_SIZE = BUFSIZ;
    static const int READ_BUFFER_SIZE = BUFSIZ;
    enum METHOD{GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH};
    enum CHECK_STATE{CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};
    enum HTTP_CODE{NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};
    enum LINE_STATUS{LINE_OK = 0, LINE_OPEN};

private:
    void init();
    HTTP_CODE processRead(){};
    bool processWrite(HTTP_CODE ret){};
    HTTP_CODE parseRequestLine(char* text){};
    HTTP_CODE parseHeaders(char* text){};
    HTTP_CODE parseContent(char* text){};
    HTTP_CODE doRequest();

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