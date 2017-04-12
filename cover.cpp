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

#include "mysqldb.h"
#include "tinyxml.h"
#include "module.h"
#include "printlog.h"

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



typedef void*(*ThreadFunc)(void*);

//模块列表
vector<Module> ModuleList;

//模块访问列表访问锁
pthread_mutex_t	gModuleListMutex = PTHREAD_MUTEX_INITIALIZER;

//配置信息
Config config;

string GetChildContent( TiXmlElement *pTiElement, const char *pszElement ) {
	if ( pTiElement == NULL ){
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "pTiElement is null" );
		return string();
	}

	TiXmlElement *pTiChild = pTiElement->FirstChildElement( pszElement );
	if ( pTiChild == NULL )
	{
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "pTiChild is null" );
		return string();
	}

	TiXmlNode *pTiNode = pTiChild->FirstChild();
	if ( pTiNode == NULL )
	{
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "pTiNode is null" );
		return string();
	}

	return pTiNode->Value();
}

string GetFormatTime( time_t pTime ) {
	time_t tmTemp;
	if ( pTime ){
		tmTemp = pTime;
	}
	else{
		tmTemp = time( NULL );
	}

	tm *temp;
	temp = localtime( &tmTemp );
	char szTime[32];
	sprintf( szTime, "%d-%.2d-%.2d %.2d:%.2d:%.2d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday,
	         temp->tm_hour, temp->tm_min, temp->tm_sec );
	return szTime;
}

//读取配置信息
int ReadConfig(const char* ConfigFileName) {
	TiXmlDocument doc;
	string module;
	
	doc.LoadFile(ConfigFileName);
	if ( doc.Error() ) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "parse xml error, description = %s", doc.ErrorDesc());
		return -1;
	}
	TiXmlElement *root = doc.RootElement();
	if ( root == NULL ){
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "the format of xml is illege");
		return -1;
	}
	config.Version = GetChildContent( root, "version");
	config.IsDebug = atoi( GetChildContent( root, "is_debug").c_str() );
	config.Port    = atoi( GetChildContent( root, "port").c_str() );
	config.Domain  = GetChildContent( root, "domain");
	config.StaticFileUrl = GetChildContent( root, "static");
	config.TemplateFileRoot = GetChildContent( root, "template");
	config.LogFileEnable= atoi( GetChildContent( root, "log_enable").c_str() );
	config.LogFilePath = GetChildContent( root, "log_path");
	
	return 0;
}

//初始化网络
int InitNetwork() {
	int sock = -1;
	sockaddr_in addr;
	
	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( sock == INVALID_SOCKET ) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "socket failed");
		return -1;
	}
	memset( &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr  = htonl( INADDR_ANY );
	addr.sin_port = htons( config.Port );
	if ( bind(sock, ( struct sockaddr * )&addr, sizeof( addr )) != 0) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "Bind Error %s", strerror(errno));
		return -1;
	}
	if ( listen( sock, 5 ) != 0 ) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "Listen Error %s", strerror(errno));
		return -1;
	}
	int flag = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))< 0) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "Reuse Address Error %s", strerror(errno));
	}
	return sock;
}


//处理客户端连接	
void* HandlerEntry(void *psock) {
	int sock = *(int*)psock;
	free(psock);
	HTTP http;
	char *line = (char *)malloc(sizeof(char) * 1024 * 5);
	if( line == NULL ) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "malloc %s", strerror(errno));
		return NULL;
	}
	// ParseHttp(sock, &http);
	GetRawData(sock, line, 1024 * 5);
	GetMethod(&http, line);
	_printlog(__FILE__, __LINE__, PRIORITY_INFO, "Method %s", http.Method.c_str());	
	if ( strcasecmp(http.Method.c_str(), "GET") == 0) {
		ParseGETLine(&http, line);
		ParseGETHead(&http, line);
		free(line);
#ifdef _Debug	
		printf("URI %s\n", http.Uri.c_str());		
#endif
		pthread_mutex_lock(&gModuleListMutex);
		for(int i = 0; i < ModuleList.size(); i++)
			if (ModuleList[i].ModuleName == http.Uri) {
				ModuleList[i].func(&http, sock);
			}
		pthread_mutex_unlock(&gModuleListMutex);	
		close(sock);
	}
	else if( strcasecmp(http.Method.c_str(), "POST") == 0) {
		ParsePOSTLine(&http, line);
		ParsePOSTHead(&http, line);
		ParsePOSTQueryString(&http, line);
		map<string, string>::iterator it;
		for(it = http.Params.begin(); it != http.Params.end(); it++)
		printf("key %s value %s\n", it->first.c_str(), it->second.c_str());
		//解析post提交的数据
	}
	return NULL;
}

static int Init(){
	int sock = -1;
	
	if( (sock = InitNetwork()) < 0) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "Iniatial NetWork Error");
		return -1;
	}
	return sock;
}

static void Run(int sock) {
	int client = -1;
	int *pclient = NULL;
	while(1) {
		client = accept( sock, NULL, NULL);
		pthread_t tid;
		if ( client != INVALID_SOCKET ) {
			pclient = (int*)malloc(sizeof(int));
			if(pclient == NULL) {
				_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "malloc error %s", strerror(errno));
				return;
			}
			*pclient = client;
			pthread_create(&tid, NULL, &HandlerEntry, (void*)pclient);
		}
		else{
			_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "Accept Failed");
		}
	}		
}

void ListenAndServe() {
	int sock = Init();
	if (sock == -1) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "Initial Server Error %s", strerror(errno));
		return;
	}
	_printlog(__FILE__, __LINE__, PRIORITY_INFO, "Server Start.");
	Run(sock);
}
