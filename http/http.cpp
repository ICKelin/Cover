#include "http.h"

int GetRawData(int fd, char *line, int length);
void GetMethod(HTTP *http, const char *line);
void ParseGETLine(HTTP *http, char *line);
void ParseGETHead(HTTP *http, char *line);
void GetRequestParameter(HTTP *http, char *line);

static void LogRequest(HTTP *http) {
	_printlog(__FILE__, __LINE__, PRIORITY_INFO,
					"FD: %d %s %s", http->fd,  http->Method.c_str(), http->Uri.c_str());
	string buf;
	for(map<string, string>::iterator it = http->Heads.begin(); it != http->Heads.end(); it++) {
		buf = buf + " " + it->first + " " + it->second;
	}
	_printlog(__FILE__, __LINE__, PRIORITY_INFO, " %s ", buf.c_str());
	buf = "";
	for(map<string, string>::iterator it = http->Params.begin(); it != http->Params.end(); it++) {
		buf = buf + " " + it->first + " " + it->second;
	}
	_printlog(__FILE__, __LINE__, PRIORITY_INFO, " %s ", buf.c_str());
}

int ParseHttp(int sock, HTTP *http) {
	char data[1024*4];
	http->fd = sock;
	GetRawData(sock, data, 1024*4);
	GetMethod(http, data);
	ParseGETHead(http, data);
	ParseGETLine(http, data);
	LogRequest(http);
	if (http->Method == "GET") {
		ParseGETHead(http, data);
		ParseGETLine(http, data);
	}
}

int GetRawData(int fd, char *line, int length) {
	if(recv(fd, line, length, 0) <0)
		return -1;
	return 0;
}

void GetMethod(HTTP *http, const char *line) {
	if(strncasecmp(line, "GET ", 4) == 0)
		http->Method = "GET";
	else if(strncasecmp(line, "POST", 4) == 0)
		http->Method  = "POST";
	else
		http->Method = "UnImplement";
}

//解析GET的请求行
void ParseGETLine(HTTP *http, char *line) {
	char *begin = line + strlen(http->Method.c_str()) + 1;
	char *npos = begin;
	while(*npos != ' ')
		npos++;
	char *buf = (char*)malloc(512);
	if(buf == NULL) {
		_printlog(__FILE__, __LINE__, PRIORITY_FATAL, "malloc error %s\n", strerror(errno));
		return;
	}
	snprintf(buf, npos - begin +1, "%s", begin);
	if(strcmp(buf, "/") == 0 || strcmp(buf, "") == 0) {
		http->Uri = "/index.html";
		free(buf);
	}
	else {
		//解析参数
		npos = begin;
		while(*npos != '?' && *npos != ' ') {
			npos++;
		}
		snprintf(buf, npos - begin +1, "%s", begin);
		if(strcmp(buf, "/") == 0 || strcmp(buf, "") == 0)
			http->Uri = "/index.html";
		else
			http->Uri.assign(buf);
		//参数
		if( *npos == '?') {
			while(*npos != ' ') {
				char key[50], value[50];
				begin = npos + 1;
				npos = begin;
				while(*npos != '=' && *npos != '\n') {
					npos++;	
				}
				snprintf(key, npos - begin + 1, "%s", begin);
			
				begin = npos + strlen("=");
				npos = begin;
				while(*npos != '&' && *npos != ' ') {
					npos++;	
				}
				snprintf(value, npos - begin + 1, "%s", begin);
				http->Params.insert(pair<string, string>(key, value));
			}
		}
		free(buf);
	}
}

//解析GET的请求头
void ParseGETHead(HTTP *http, char *line) {
	char *pos = line;
	char key[512];
	char value[512];
	int i = 0;
	//跳过前面两行
	while(*pos++ != '\n');
	while(*pos++ != '\n');

	
	while(*pos && *pos != '\r'){
		memset(key, 0, sizeof(key));
		memset(value, 0, sizeof(value));
		i = 0;
		while(*pos && *pos != ':'){
			key[i++] = *pos++;
		}
		pos++;
		i = 0;
		while(*pos && *pos != '\n') {
			value[i++] = *pos++;
		}
		pos++;
		http->Heads.insert(pair<string, string>(key, value));
	}
}

//获取请求的参数
string GetParameter(HTTP *http, string Parameter) {
	return http->Params[Parameter];
}

////////////////////////////////////////////////////////////////////////////////
// Reasonse Area
////////////////////////////////////////////////////////////////////////////////

#define DEFAULT  "./static/"

//返回数据
int Response(int fd, void *data, int length) {

		return send(fd, data, length, 0);
}

//读取文件并返回
void ServeFile(HTTP *http, int sock, const char *fileName) {
        char uri[256];
        char buf[256];
        sprintf(uri, "%s%s", DEFAULT, fileName);
        _printlog(__FILE__, __LINE__, PRIORITY_INFO, "serve file %s\n", fileName);
        FILE *fp = fopen(uri, "r");
        if (fp == NULL) {
                _printlog(__FILE__, __LINE__, PRIORITY_ERROR, "fopen error %s\n", strerror(errno));
                return;
        }
        strcpy(buf, "HTTP/1.1 200 OK\r\n");
        strcat(buf, "server:ICKelin\r\n");
        strcat(buf, "Content-Type: text/html\r\n");
        strcat(buf, "\r\n");
        send(sock, buf, strlen(buf), 0);
        while(fgets(buf, 256, fp) != NULL)
                send(sock, buf, strlen(buf), 0);
        fclose(fp);
}

