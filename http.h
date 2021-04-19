#ifndef __NETPRO__HTTP__H
#define __NETPRO__HTTP__H

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include "sql.h"
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

    SQL sql;


public:
    HTTP_CODE processRead() {
        //dbg(mReadBuf);
        LINE_STATUS lineStatus = LINE_OK;
        HTTP_CODE ret = NO_REQUEST;
        char* text = 0;
        
        while(true){
            if(mCheckState != CHECK_STATE_CONTENT || lineStatus != LINE_OK){
                lineStatus = parseLine();
                if(lineStatus != LINE_OK)break; 
            }

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
                    //dbg(ret == GET_REQUEST);
                    //dbg(ret == NO_REQUEST);
                    if(ret == BAD_REQUEST)
                        return BAD_REQUEST;
                    else if(ret == GET_REQUEST)
                        return doRequest();
                    break;
                }
                case CHECK_STATE_CONTENT: {
                    dbg("ENTER CHECK_STATE_CONTENT");
                    ret = parseContent(text);
                    //dbg(mString);
                    // dbg(ret);
                    if(ret == GET_REQUEST)return doRequest();
                    lineStatus = LINE_OPEN;
                    break;
                }
                default: return INTERNAL_ERROR;
            }
        }
        //print();
        return NO_REQUEST;
    }

    bool processWrite(HTTP_CODE ret) {
        char badres[] = "HTTP/1.1 400 Bad Request\r\nDate: Thu, 16 Mar 2021 23:59:59 GMT\r\nContent-Type: text/html;charset=ISO-8859-1\r\n\r\n<html><head><title>ERROR</title></head><body>400 BAD REQUEST</body></html>\n";
        string goodres = "HTTP/1.1 200 OK\r\nDate: Thu, 16 Mar 2021 23:59:59 GMT\r\nContent-Type: text/html;charset=ISO-8859-1\r\n\r\n";
        switch(ret){
            case GET_REQUEST: 
                if(mMethod == GET){
                    fetchContent(goodres);
                    strcpy(mWriteBuf, goodres.c_str());
	                bytesToSend = strlen(mWriteBuf) + 1;
                    break;
                }
                else if(mMethod == POST){
                    goodres += retBuf;
                    strcpy(mWriteBuf, goodres.c_str());
	                bytesToSend = strlen(mWriteBuf) + 1;
                    break;

                }
                else break;
            case BAD_REQUEST: strcpy(mWriteBuf, badres);
	                          bytesToSend = strlen(mWriteBuf) + 1;
                              break;
        }
        return true;
    }

    void print(){
        dbg(mReadBuf, mReadIdx, mCheckedIdx, mStartLine);
        dbg(mWriteBuf, mWriteIdx, mCheckState, mMethod, mRealFile, mUrl, mVersion, mHost, mContentLength, mLinger, mFileAddress, cgi, mString, bytesToSend, bytesHaveSend);
    }

private:
    void fetchContent(string& goodres){
        string content;
        std::ifstream fs;
        fs.open("index.html");
        char ch;
        while(fs.get(ch))
            content.push_back(ch);
        goodres += content + "\n";
        dbg(goodres);
    }

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
        dbg(text);
        //dbg(text[0] == '\0');

        if(text[0] == '\0'){
            if(mContentLength != 0){
                dbg("ENTER CHECK_STATE_CONTENT");
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
        else if(strncasecmp(text, "Content-Length:", 15) == 0){
            //dbg("CHECK CONTENT LENGTH");
            text += 15;
            text += strspn(text, " \t");
            mContentLength = atol(text);
        }
        else if(strncasecmp(text, "Host:", 5) == 0){
            text += 5;
            text += strspn(text, " \t");
            mHost = text;
        }
        else {
            dbg("UNKNOWN HEADER");
        }
        return NO_REQUEST;
    }

    HTTP_CODE parseContent(char* text) {
        //dbg(mReadIdx >= mContentLength + mCheckedIdx);
        //dbg(text + 1);
        if(mReadIdx >= (mContentLength + mCheckedIdx)){
            text[mContentLength + 1] = '\0';
            mString = text;
            return GET_REQUEST;
        }
        return NO_REQUEST;
    }
    HTTP_CODE doRequest() {
        print();
        if(mMethod == GET)return GET_REQUEST;
        if(mString == nullptr)return BAD_REQUEST;

        char *op, *username, *password;

        //这里需要做格式检查
        dbg("DO REQUEST");
        if(strncasecmp(mString, "op=", 3) != 0)return BAD_REQUEST;
        op = mString + 3;
        mString = strchr(mString, '&');
        *(mString++) = '\0';
        if(strncasecmp(mString, "username=", 9) != 0)return BAD_REQUEST;
        username = mString + 9;
        mString = strchr(mString, '&');
        *(mString++) = '\0';
        if(strncasecmp(mString, "password=", 9) != 0)return BAD_REQUEST;
        password = mString + 9;
        dbg(op, username, password);

        if(strcmp(op, "signin") == 0){
            bool status = sql.checkUser(string(username), string(password));
            if(!status){
                retBuf = "Failed";
                return GET_REQUEST;
            }
        }
        else if(strcmp(op, "signup") == 0){
            bool status = sql.createUser(string(username), string(password));
            if(!status){
                retBuf = "Failed";
                return GET_REQUEST;
            }
        }
        else return BAD_REQUEST;

        retBuf = "OK";
        return GET_REQUEST;
    }

    char* getLine() { return mReadBuf + mStartLine; }   //后移指针

    LINE_STATUS parseLine() {                           //分析请求报文的部分
        //dbg(mReadIdx, mCheckedIdx);
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
    bool addResponse(const char* format);
    bool addContent(const char* content);
    bool addStatusLine(int status, const char* title);
    bool addHeaders(int contentLength);
    bool addContentType();
    bool addContentLength();
    bool addLinger();
    bool addBlankLine();

    string retBuf;
};

#endif
