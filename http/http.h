#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#include <syslog.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "printlog.h"
#include "tinyxml.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

using namespace std;


struct HTTP {
	int 	fd;			//网络套接字	
	string Method;			//请求方法
	string Uri;			//请求uri
	map<string,string> Heads;	//请求头字段
	map<string, string> Params;	//请求参数字段
	map<string, string> Body;	//post消息体
};

int ParseHttp(int sock, HTTP *http);

int Response(int sock, void *data, int length);
void ServeFile(HTTP *http, int sock, const char *filename);
