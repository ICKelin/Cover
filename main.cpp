#include "cover.h"

void Index(HTTP *http, int sock) {
	_printlog(__FILE__, __LINE__, PRIORITY_INFO, "Serving Index");
	ServeFile(http, sock, "index.html");
}

int main(int argc, char **argv) {
	if (ReadConfig("config.xml") < 0) {
		_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "ReadConfig Error");
		return -1;
	}
	AddModule("/", Index);
	AddModule("/index.html", Index);
	ListenAndServe();
	_printlog(__FILE__, __LINE__, PRIORITY_ERROR, "Run Error");
}
