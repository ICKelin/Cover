#ifndef MYSQLDB_H_
#define MYSQLDB_H_
#include <mysql.h>

typedef struct {
	MYSQL *mysql;
	//
	unsigned int err_no;
	const char *err_str;
}DBI;

int db_connect(DBI *dbi, const char *host, const char *user, 
				const char *password, const char *dbname, 
				unsigned int port);

void db_close(DBI *dbi);

void *db_exec_query(DBI *dbi, const char *sql, unsigned long length,
				unsigned int *num_fields, unsigned int *num_rows);
char **db_fetch_row(void *rs);
unsigned long *db_fetch_lengths(void *rs);
void db_end_query(void *rs);
unsigned long db_escape_string(DBI *dbi, char *to, const char *from, unsigned long length);

#endif /*MYSQLDB_H_*/
