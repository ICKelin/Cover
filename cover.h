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
#include "module.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define _Debug

using namespace std;

struct Config {
	string  Version;		//版本
	bool	IsDebug;		//是否调试版
	int 	Port;			//监听端口
	string  Domain;			//域名
	string  StaticFileUrl;		//静态文件url
	string  StaticFileRoot;		//静态文件路径
	string  TemplateFileRoot;	//模版文件路径
	bool	RequestLogEnable;	//请求是否记录日志
	
	int 	LogFileEnable;		//文件日志是否启动
	string	LogFilePath;		//日志文件的路径
			
};

//读取配置信息
int ReadConfig(const char* ConfigFileName);

//添加模块
void AddModule(string patterni, ModuleFunc func);
void ListenAndServe();
