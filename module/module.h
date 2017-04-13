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
#include "http.h"

using namespace std;

typedef void(*ModuleFunc)(HTTP* phttp, int sock);

struct Module {
	string 	ModuleName;
	ModuleFunc func;
};
