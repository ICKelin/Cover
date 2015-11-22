#include <stdio.h>
#include "mysqldb.h"

int db_connect(DBI *dbi, const char *host, const char *user, 
				const char *password, const char *dbname, 
				unsigned int port)
{
	dbi->mysql = mysql_init(NULL);
	if (!dbi->mysql)
		return -1;

	char opt_reconn = 1; 
	mysql_options(dbi->mysql, MYSQL_OPT_RECONNECT, &opt_reconn);
	mysql_options(dbi->mysql, MYSQL_SET_CHARSET_NAME, "gbk");
	
	if (!mysql_real_connect(dbi->mysql, host, user, password, 
			dbname, port, NULL, 0)) {
		dbi->err_no = mysql_errno(dbi->mysql);
		dbi->err_str = mysql_error(dbi->mysql);
		//
		mysql_close(dbi->mysql);
		return -1;
	}
	mysql_autocommit(dbi->mysql, 1);
	return 0;
}

void db_close(DBI *dbi)
{
	if (dbi->mysql) {
		mysql_close(dbi->mysql);
		dbi->mysql = NULL;
	}
}

void *db_exec_query(DBI *dbi, const char *sql, unsigned long length, 
					unsigned int *num_fields, unsigned int *num_rows)
{
	MYSQL_RES *rs;
	
	if (mysql_real_query(dbi->mysql, sql, length) != 0) {
		dbi->err_no = mysql_errno(dbi->mysql);
		dbi->err_str = mysql_error(dbi->mysql);
		return NULL;
	}
	
	rs = mysql_store_result(dbi->mysql);
	if (!rs) {
		dbi->err_no = mysql_errno(dbi->mysql);
		dbi->err_str = mysql_error(dbi->mysql);
		return NULL;
	}
	
	if (num_fields)
		*num_fields = mysql_num_fields(rs);
	
	if (num_rows)
		*num_rows = (unsigned int)mysql_num_rows(rs);
	
	return rs;
}

char **db_fetch_row(void *rs) 
{
	MYSQL_ROW row = NULL;
	if (rs) {
		row = mysql_fetch_row((MYSQL_RES *)rs);
	}
	return row;
}

unsigned long *db_fetch_lengths(void *rs)
{
	unsigned long *lengths = NULL;
	
	if (rs) {
		lengths = mysql_fetch_lengths((MYSQL_RES *)rs);
	}
	//
	return lengths;
}

void db_end_query(void *rs)
{
	if (rs) {
		mysql_free_result((MYSQL_RES *)rs);
	}
}

unsigned long db_escape_string(DBI *dbi, char *to, const char *from, 
							   unsigned long length)
{
	return mysql_real_escape_string(dbi->mysql, to, from, length);
}

