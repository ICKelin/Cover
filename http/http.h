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
};

int GetRawData(int fd, char *line, int length);

void GetMethod(HTTP* http, const char*line);

void ParseGETLine(HTTP *http, char *line);

void ParseGETHead(HTTP *http, char *line);

void ParsePOSTHead(HTTP *http, char *line);

void ParsePOSTLine(HTTP *http, char *line);

void ParsePOSTQueryString(HTTP *http, char *line);

string GetRequestParameter(HTTP *http, string Parameter);
