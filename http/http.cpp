
#include "http.h"

int GetRawData(int fd, char *line, int length) {
	if(recv(fd, line, length, 0) != -1)
		return 0;
	return -1;
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
		fprintf(stderr, "malloc error %s\n", strerror(errno));
		return;
	}
	snprintf(buf, npos - begin +1, "%s", begin);
	if(strcmp(buf, "/") == 0) {
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
		if(strcmp(buf, "/") == 0)
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

//解析POST的请求行
void ParsePOSTLine(HTTP *http, char* line) {
	ParseGETLine(http, line);
}

//解析POST的请求头
void ParsePOSTHead(HTTP *http, char *line) {
	ParseGETHead(http, line);
}

//
void ParsePOSTQueryString(HTTP *http, char *line) {
	char *pos = strstr(line ,"\r\n\r\n");
	if(pos != NULL) {
		pos = pos + strlen("\r\n\r\n");
		//
		while(*pos) {
			char key[256];
			char value[256];
			int i = 0;
			while(*pos && *pos != '=')
				key[i++] = *pos++;
			i = 0;
			pos++;
			while(*pos && *pos != '&')
				value[i++] = *pos++;
			pos++;
			http->Params.insert(pair<string, string>(key, value));
		}
	}
}

//获取请求的参数
string GetParameter(HTTP *http, string Parameter) {
	return http->Params[Parameter];
}

//获取http头信息
string GetHead(HTTP *http, string head) {
	return http->Heads[head];
}
