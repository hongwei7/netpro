#ifndef __NETPRO__HTTP__H
#define __NETPRO__HTTP__H

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "dbg.h"

static const int FILE_NAME_LEN = 200;
static const int WRITE_BUFFER_SIZE = BUFSIZ;
static const int READ_BUFFER_SIZE = BUFSIZ;
enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH };
enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
enum LINE_STATUS { LINE_OK = 0, LINE_OPEN, LINE_BAD };

class httpconn {
public:
    httpconn(){
        bytesToSend = 0;
        bytesHaveSend = 0;
        mCheckState = CHECK_STATE_REQUESTLINE;
        mLinger = false;
        mMethod = GET;
        mUrl = 0;
        mVersion = 0;
        mContentLength = 0;
        mHost = 0;
        mStartLine = 0;
        mCheckedIdx = 0;
        mReadIdx = 0;
        mWriteIdx = 0;
        cgi = 0;

    memset(mReadBuf, '\0', READ_BUFFER_SIZE);
    memset(mWriteBuf, '\0', WRITE_BUFFER_SIZE);
    memset(mRealFile, '\0', FILE_NAME_LEN);
    }
public:
    char mReadBuf[READ_BUFFER_SIZE];               //储存请求报文数据
    int mReadIdx;                                  //缓冲区数据最后一个字节的下一个位置
    int mCheckedIdx;                               //已经读取的位置
    int mStartLine;                                //已经解析的字符个数

    char mWriteBuf[WRITE_BUFFER_SIZE];             //响应缓冲区
    int mWriteIdx;                                 //缓冲区数据长度

    CHECK_STATE mCheckState;                       //现在的状态机状态
    METHOD mMethod;                                //请求方法

    char mRealFile[FILE_NAME_LEN];
    char *mUrl, *mVersion, *mHost;
    int mContentLength;
    bool mLinger;

    char* mFileAddress;
    int cgi;                                       //是否启用 POST
    char* mString;                                 //请求头数据
    int bytesToSend;
    int bytesHaveSend;


public:
    HTTP_CODE processRead() {
        LINE_STATUS lineStatus = LINE_OK;
        HTTP_CODE ret = NO_REQUEST;
        char* text = 0;
        
        while((mCheckState == CHECK_STATE_CONTENT && lineStatus == LINE_OK )|| ((lineStatus = parseLine()) == LINE_OK)){

            text = getLine();
            mStartLine = mCheckedIdx;

            switch(mCheckState){
                case CHECK_STATE_REQUESTLINE: {
                    // dbg("CHECK_STATE_REQUESTLINE");
                    ret = parseRequestLine(text);
                    // dbg(ret);
                    if(ret == BAD_REQUEST)
                        return BAD_REQUEST;
                    break;
                }
                case CHECK_STATE_HEADER: {
                    // dbg("CHECK_STATE_HEADER");
                    ret = parseHeaders(text);
                    // dbg(ret);
                    if(ret == BAD_REQUEST)
                        return BAD_REQUEST;
                    else if(ret == GET_REQUEST)
                        return doRequest();
                    break;
                }
                case CHECK_STATE_CONTENT: {
                    // dbg("CHECK_STATE_CONTENT");
                    ret = parseContent(text);
                    // dbg(ret);
                    if(ret == GET_REQUEST)return doRequest();
                    lineStatus = LINE_OPEN;
                    break;
                }
                default: return INTERNAL_ERROR;
            }
        }
        return NO_REQUEST;
    }

    bool processWrite(HTTP_CODE ret) {
        char httpres[] = "HTTP/1.1 200 OK\r\nDate: Thu, 16 Mar 2021 23:59:59 GMT\r\nContent-Type: text/html;charset=ISO-8859-1\r\n\r\n<html><head><title>TEST</title></head><body>HELLO</body></html>\n";
		strcpy(mWriteBuf, httpres);
		bytesToSend = strlen(mWriteBuf) + 1;
        return true;
    }

    void print(){
        dbg(mReadBuf, mReadIdx, mCheckedIdx, mStartLine);
        dbg(mWriteBuf, mWriteIdx, mCheckState, mMethod, mRealFile, mUrl, mVersion, mHost, mContentLength, mLinger, mFileAddress, cgi, mString, bytesToSend, bytesHaveSend);
    }

private:
    HTTP_CODE parseRequestLine(char* text) {
        mUrl = strpbrk(text, " \t");                    //请求行中最先含有空格和\t任一字符的位置并返回
        
        if(!mUrl)return BAD_REQUEST;
        *mUrl++ = '\0';

        char* method = text;
        if(strcmp(method, "GET") == 0)mMethod = GET;
        else if(strcmp(method, "POST") == 0){
            mMethod = POST;
            cgi = 1;
        }
        else return BAD_REQUEST;

        mUrl += strspn(mUrl, " \t");
        mVersion = strpbrk(mUrl, " \t");

        if(!mVersion) return BAD_REQUEST;
        *mVersion++ = '\0';
        mVersion += strspn(mVersion, " \t");
        if(strcasecmp(mVersion, "HTTP/1.1") != 0)return BAD_REQUEST;
                                                        //只支持http1.1
        if(strncasecmp(mUrl, "http://", 7) == 0){
            mUrl += 7;
            mUrl = strchr(mUrl, '/');
        }
        if(strncasecmp(mUrl, "https://", 8) == 0){
            mUrl += 8;
            mUrl = strchr(mUrl, '/');
        }

        if(!mUrl || mUrl[0] != '/')return BAD_REQUEST;
        if(strlen(mUrl) == 1)
            strcat(mUrl, "judge.html");
        
        mCheckState = CHECK_STATE_HEADER;

        return NO_REQUEST;
    }

    HTTP_CODE parseHeaders(char* text) {
        if(text[0] == '\0'){
            if(mContentLength != 0){
                mCheckState = CHECK_STATE_CONTENT;
                return NO_REQUEST;
            }
            return GET_REQUEST;
        }
        else if(strncasecmp(text, "Connection:", 11) == 0){
            text += 11;

            text += strspn(text, " \t");
            if(strcasecmp(text, "keep-alive") == 0){
                mLinger = true;
            }
        }
        else if(strncasecmp(text, "Content-length:", 15) == 0){
            text += 15;
            text += strspn(text, " \t");
            mContentLength = atol(text);
        }
        else if(strncasecmp(text, "Host:", 5) == 0){
            text += 5;
            text += strspn(text, " \t");
            mHost = text;
        }
        else printf("UNKNOWN HEADER: %s\n", text);
        return GET_REQUEST;
        return NO_REQUEST;
    }

    HTTP_CODE parseContent(char* text) {
        if(mReadIdx >= (mContentLength + mCheckedIdx)){
            text[mContentLength] = '\0';
            mString = text;
            return GET_REQUEST;
        }
        return NO_REQUEST;
    }
    HTTP_CODE doRequest() {return NO_REQUEST;}

    char* getLine() { return mReadBuf + mStartLine; }   //后移指针

    LINE_STATUS parseLine() {                           //分析请求报文的部分
        char temp;
        for(; mCheckedIdx < mReadIdx; ++mCheckedIdx){
            temp = mReadBuf[mCheckedIdx];

            if(temp == '\r'){
                if(mCheckedIdx + 1 == mReadIdx)         //到达buffer结尾，接收不完整。
                    return LINE_OPEN;
                else if(mReadBuf[mCheckedIdx + 1] == '\n'){
                    mReadBuf[mCheckedIdx++] = '\0';
                    mReadBuf[mCheckedIdx++] = '\0';
                    return LINE_OK;
                }
                return LINE_BAD;
            }
            else if(temp == '\n'){
                if(mCheckedIdx > 1 && mReadBuf[mCheckedIdx - 1] == '\r'){
                    mReadBuf[mCheckedIdx - 1] = '\0';
                    mReadBuf[mCheckedIdx++] = '\0';
                    return LINE_OK;
                }
                return LINE_BAD;
            }
        }
        return LINE_OPEN;                               //需要继续接收
    }

    //doRequest 中的函数
    bool addResponse(const char* format, ...);
    bool addContent(const char* content);
    bool addStatusLine(int status, const char* title);
    bool addHeaders(int contentLength);
    bool addContentType();
    bool addContentLength();
    bool addLinger();
    bool addBlankLine();
};

#endif
