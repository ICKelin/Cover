#include "cover.h"

typedef void*(*ThreadFunc)(void*);

vector<Module> ModuleList;

//模块访问列表访问锁
pthread_mutex_t	gModuleListMutex = PTHREAD_MUTEX_INITIALIZER;

//配置信息
Config config;

void Log( const char *format, ... ) {
	va_list ap;
	va_start( ap, format );
	char szBuf[256];
	memset( szBuf, 0, 256 );
	vsnprintf( szBuf, sizeof( szBuf ) - 1, format, ap );
	va_end( ap );

#ifdef _Debug
	fprintf( stderr, "%s\n", szBuf );
#else
	syslog( LOG_ERR, szBuf );
#endif
}

string GetChildContent( TiXmlElement *pTiElement, const char *pszElement ) {
	if ( pTiElement == NULL ){
		Log( "pTiElement is null" );
		return string();
	}

	TiXmlElement *pTiChild = pTiElement->FirstChildElement( pszElement );
	if ( pTiChild == NULL )
	{
		Log( "pTiChild is null" );
		return string();
	}

	TiXmlNode *pTiNode = pTiChild->FirstChild();
	if ( pTiNode == NULL )
	{
		Log( "pTiNode is null" );
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
int readConfig(const char* ConfigFileName) {
	TiXmlDocument doc;
	string module;
	
	doc.LoadFile(ConfigFileName);
	if ( doc.Error() ) {
		Log("parse xml error, description = %s", doc.ErrorDesc());;
		return -1;
	}
	TiXmlElement *root = doc.RootElement();
	if ( root == NULL ){
		Log("the format of xml is illege");
		return -1;
	}
	config.Version = GetChildContent( root, "version");
	config.IsDebug = atoi( GetChildContent( root, "is_debug").c_str() );
	config.Port    = atoi( GetChildContent( root, "port").c_str() );
	config.Domain  = GetChildContent( root, "domain");
	config.LogFileEnable= atoi( GetChildContent( root, "log_enable").c_str() );
	config.LogFilePath = GetChildContent( root, "log_path");
	
	return 0;
}

//初始化网络
int initNetwork() {
	int sock = -1;
	sockaddr_in addr;
	
	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( sock == INVALID_SOCKET ) {
		Log("socket failed");
		return -1;
	}
	memset( &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr  = htonl( INADDR_ANY );
	addr.sin_port = htons( config.Port );
	if ( bind(sock, ( struct sockaddr * )&addr, sizeof( addr )) != 0) {
		Log("Bind Error %s", strerror(errno));
		return -1;
	}
	if ( listen( sock, 5 ) != 0 ) {
		Log("listen Error %s\n", strerror(errno));
		return -1;
	}
	int flag = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
		Log("Reuse Address Error %s", strerror(errno));
	}
	return sock;
}

//处理客户端连接	
void *handlerEntry(void *psock) {
	int sock = *(int*)psock;
	free(psock);
	HTTP http;
	char *line = (char *)malloc(sizeof(char) * 1024 * 5);
	if( line == NULL ) {
		Log("malloc %s\n", strerror(errno));
		return NULL;
	}
	GetRawData(sock, line, 1024 * 5);
	//超过最大负载
	if(strlen(line) > MAXPAYLOAD) {
		return NULL;
	}
	GetMethod(&http, line);
	if ( strcasecmp(http.Method.c_str(), "GET") == 0) {
		ParseGETLine(&http, line);
		ParseGETHead(&http, line);
		free(line);
		pthread_mutex_lock(&gModuleListMutex);
		for(unsigned int i = 0; i< ModuleList.size(); i++)
			if (ModuleList[i].ModuleName == http.Uri) {
				ModuleList[i].func(&http, sock);
				break;
			}
		pthread_mutex_unlock(&gModuleListMutex);	
		close(sock);
	}
	else if( strcasecmp(http.Method.c_str(), "POST") == 0) {
		ParsePOSTLine(&http, line);
		ParsePOSTHead(&http, line);
		ParsePOSTQueryString(&http, line);
	}
	free(line);
	return NULL;
}

//添加模块接口
int CoverAddModule(char* name, ModuleFunc func) {
	Module module;
	module.ModuleName = name;
	module.func = func;	
	ModuleList.push_back(module);
	return 0;	
}

int CoverInit() {
	int sock = -1;
	if(readConfig("config.xml") < 0) {
		Log("ReadConfig Error");
		return -1;
	}
		
	if( (sock = initNetwork()) < 0) {
		Log("Iniatial NetWork Error");
		return -1;
	}
	return sock;
}

void CoverRun(int sock) {
	int client = -1;
	int *pclient = NULL;
	while(1) {
		client = accept( sock, NULL, NULL);
		pthread_t tid;
		if ( client != INVALID_SOCKET ) {
			pclient = (int*)malloc(sizeof(int));
			if(pclient == NULL) {
				Log("malloc error %s", strerror(errno));
				return;
			}
			*pclient = client;
			pthread_create(&tid, NULL, &handlerEntry, (void*)pclient);
		} else {
			Log("Accept Failed");
		}
	}		
}
#if 0
int main(int argc, char **argv) {
	int sock = CoverInit();
	if(sock == -1) {
		Log("Initial Server Error %s", strerror(errno));
		return -1;
	}
	CoverRun(sock);	
}
#endif
