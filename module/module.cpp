#include "module.h"

#define DEFAULT	"./static"

//返回数据
int Response(int fd, void *data, int length) {
	return send(fd, data, length, 0);		
}

//读取文件并返回
void ServeFile(HTTP *http, int sock, const char *fileName) {
	char uri[256];
	char buf[256];
	sprintf(uri, "%s%s", DEFAULT, fileName);
	printf("serve file %s\n", fileName);	
	FILE *fp = fopen(uri, "r");
	if (fp == NULL) {
		fprintf(stderr, "fopen error %s\n", strerror(errno));
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
	close(sock);
}

//主页模块
void Index(HTTP *http, int sock) {
	ServeFile(http, sock, http->Uri.c_str());		
}

//文章模块
void Article(HTTP *http, int sock) {
	//获取所有文章
	if(strcmp(http->Params["type"].c_str(), "0") == 0) {
		//string data = GetAllArticle();
		//Response(sock, data.c_str(), strlen(data.c_str()));
	}
	//获取根据文章ID获取文章信息
	if(strcmp(http->Params["type"].c_str(), "1") == 0) {
		string article_id = http->Params["articleid"];
		//string data = GetArticleById(article_id);
		//string comment = GetArticleCommentByArticleId(article_id)		
		//data.append(comment);
		//Response(sock, data.c_str(), strlen(data.c_str()));
	}
	close(sock);	
}

//登录模块
void Signin(HTTP *http, int sock) {
	ServeFile(http, sock, "/sigin.html");		
}

