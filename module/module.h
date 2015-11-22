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
#include "http.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

using namespace std;

typedef void(*ModuleFunc)(HTTP* phttp, int sock);

struct Module {
	string 	ModuleName;
	ModuleFunc func;
};

void ServeFile(const char*);

void Index(HTTP *phttp, int sock);

void Signin(HTTP *phttp, int sock);

void Article(HTTP *phttp, int sock);

