#include "cover.h"

using namespace std;

string GetChildContent(TiXmlElement *pTiElement, const char *pszElement);
int InitNetwork();
void Run(int sock);

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
	int ret = ParseHttp(sock, &http);
	ModuleFunc func;
	int flag = 0;
	if (ret == 0) {
		pthread_mutex_lock(&gModuleListMutex);
		int i = 0;
		for (i = 0; i < ModuleList.size(); i++) {
			if(ModuleList[i].ModuleName == http.Uri) {
				//ModuleList[i].func(&http, sock);
				func = ModuleList[i].func;
				break;
			}
		}
		if(i == ModuleList.size()) {
			flag = 1;
		}
		pthread_mutex_unlock(&gModuleListMutex);
		if (flag) {
			ServeFile(&http, sock, http.Uri.c_str());
		} else {
			func(&http, sock);
		}
	}
	close(sock);
	_printlog(__FILE__, __LINE__, PRIORITY_INFO, "Release connection %d", sock);
	// 404
	return NULL;
}

int Init(){
	int sock = -1;
	
	if( (sock = InitNetwork()) < 0) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "Iniatial NetWork Error");
		return -1;
	}
	return sock;
}

void Run(int sock) {
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
void AddModule(string pattern, ModuleFunc func) {
	Module module;
	module.ModuleName = pattern;
	module.func = func;
	ModuleList.push_back(module);
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
