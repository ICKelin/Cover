// printlog.cpp: implementation of the printlog.h
//
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <time.h>
#else
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>
#endif

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "printlog.h"

#define LOG_MAXLEVEL	10

static char m_logfile[256];					//日志文件路径
static char m_logpath[256];					//日志文件所在目录
static char m_progname[256];				//程序名称
static long m_npid;							//进程pid
static int	m_npriority = PRIORITY_INFO;	//日志打印的最高级别
static int  m_nlogopt = 0;					//日志文件产生规则，0固定名称及备份，1每天一个文件名
static long m_nlogsize  = 1024 * 1024;		//命名规则为0时，当文件大于该值时备份

static FILE *m_file_log = NULL;

#ifdef _WIN32
static CRITICAL_SECTION critsec;	//critical section object
#else
static pthread_mutex_t loglock;
#endif

static int mystricmp(const char *s1, const char *s2)
{   
	int i;   
	for(i = 0; s1[i]!='\0' && s2[i]!='\0'; i++)
	{   
		if(s1[i] != s2[i])
		{
			if((s1[i] >= 'a' && s1[i] <= 'z') && (s2[i] >= 'A' && s2[i] <= 'Z'))
			{
				if(s1[i] != (s2[i] + ('a' - 'A')))
					return s1[i]-s2[i];
			}
			else if((s2[i] >= 'a' && s2[i] <= 'z') && (s1[i] >= 'A' && s1[i] <= 'Z'))
			{
				if(s2[i] != (s1[i] + ('a' - 'A')))
					return s1[i]-s2[i];
			}
			else
				return s1[i]-s2[i];
		}
	}
	if(s1[i] == '\0' && s2[i] != '\0' )
		return 0 - s2[i];
	else if(s1[i] != '\0' && s2[i] == '\0')
		return s1[i];   
	return 0;
}

static const char *priority_to_string(int priority)
{
	priority /= 100;
	if(priority < 0 || priority > LOG_MAXLEVEL)
		priority = LOG_MAXLEVEL;
	return priorities[priority];
}

static int my_localtime(const time_t *clock, struct tm *result)
{
	int iret = 0;

#ifdef _WIN32
	memcpy(result, localtime(clock), sizeof(struct tm));
#else
	if(localtime_r(clock, result) == NULL)
		iret = -1;
#endif
	return iret;
}

static int getcurdatetime(char* cur_datetime)
{
	time_t     ltime;
	struct tm  tm_string;
	int        ich;
	
	if (time(&ltime) == -1) {
		printf("GetCurrentDay()::time() is error!\n");
		return -1;
	}
	
	my_localtime(&ltime, &tm_string);
	ich = strftime(cur_datetime, 20, "%Y-%m-%d %H:%M:%S", &tm_string);
	if (ich < 0) {
		printf("GetCurrentDay()::strftime() is error!\n");
		return -1;
	}
	
	return 0;
}

/*static int writetofile(char *filename, void *result, int buflen)
{
	int fd;
#ifdef _WIN32
	if ((fd = _open(filename, _O_RDWR | _O_APPEND | _O_CREAT, _S_IREAD | _S_IWRITE)) < 0)	
#else
	if ((fd = open(filename, O_RDWR | O_APPEND | O_CREAT, S_IREAD | S_IWRITE | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)	
#endif
	{
		return -1;
	}
	write(fd, result, buflen);
	close(fd);
	return 0;
}*/

/*static int writetofile(const char *filename, const void *result, int buflen)
{
	int nlen = 0;
	FILE *file = NULL;
	if((file = fopen(filename, "a+")) == NULL)
		return -1;

	nlen = fwrite(result, 1, buflen, file);
	if (nlen != buflen)
	{
		//wprintf(L"fwrite failed!\n");
		return -2;
	}

	fflush(file);
	fclose(file);

	return 0;
}*/

static int writelogfile(const void *result, int buflen)
{
	int nlen = 0;

	if(NULL == m_file_log)
	{
		if((m_file_log = fopen(m_logfile, "a+")) == NULL)
			return -1;
	}

	nlen = fwrite(result, 1, buflen, m_file_log);
	if (nlen != buflen)
	{
		//wprintf(L"fwrite failed!\n");
		fclose(m_file_log);
		return -2;
	}

	fflush(m_file_log);

	return 0;
}

static int getshortdatetime(char* cur_datetime)
{
	time_t     ltime;
	struct tm  tm_string;
	int        ich;
	
	if (time(&ltime) == -1)
	{
		printf("GetCurrentDay()::time() is error!\n");
		return -1;
	}
	
	my_localtime(&ltime, &tm_string);
	ich = strftime(cur_datetime, 15, "%Y%m%d%H%M%S", &tm_string);
	if (ich < 0) {
		printf("GetCurrentDay()::strftime() is error!\n");
		return -1;
	}
	
	return 0;
}

static int getshortdate(char* cur_date)
{
	time_t     ltime;
	struct tm  tm_string;
	int        ich;
	
	if (time(&ltime) == -1)
	{
		printf("GetCurrentDay()::time() is error!\n");
		return -1;
	}
	
	my_localtime(&ltime, &tm_string);
	ich = strftime(cur_date, 9, "%Y%m%d", &tm_string);
	if (ich < 0) {
		printf("GetCurrentDay()::strftime() is error!\n");
		return -1;
	}
	
	return 0;
}

int initlogs(const char *progname, int priority, const char *logpath, int logopt, long logsize)
{
	char path[256];
	char *pdest;
	int  nlen;

	if(priority < 0 || priority > (LOG_MAXLEVEL * 100))
		priority = PRIORITY_INFO;
	m_npriority = (priority / 100) * 100;

	m_nlogopt = logopt == 1 ? 1 : 0;
	if(logsize > 0)
		m_nlogsize = logsize * 1024;
	
	memset(path, 0, sizeof(path));
	memset(m_logfile, 0, sizeof(m_logfile));

#ifdef _WIN32
	GetModuleFileName(NULL, path, MAX_PATH);
	
	strcpy(m_logpath, path);
	(strrchr(m_logpath,'\\'))[1] = 0;
	(strrchr(path,'.'))[0] = 0;
	pdest = strrchr(path,'\\');
	strcpy(m_progname, pdest);
	if((logpath != NULL) && (access(logpath, 02) == 0))
	{
		strcpy(m_logpath, logpath);
	}
	nlen = strlen(m_logpath);
	if(m_logpath[nlen - 1] != '\\')
		strcat(m_logpath, "\\");

	sprintf(m_logfile, "%s%s.log", m_logpath, m_progname);
	
	m_npid = GetCurrentProcessId();
	
	InitializeCriticalSection(&critsec);
#else
	
	if((progname == NULL) || (strlen(progname) == 0))
	{
		strcpy(m_progname, "log");
	}
	else
	{
		strcpy(path, progname);
		pdest = strrchr(path, '/');
		if(pdest != NULL)
		{
			strcpy(m_progname, pdest + 1);
			strncpy(m_logpath, path, pdest - path);
		}
		else
		{
			strcpy(m_progname, path);
			strcpy(m_logpath, "./");
		}
	}
	if((logpath != NULL) && (access(logpath, 02) == 0))
	{
		strcpy(m_logpath, logpath);
	}
	nlen = strlen(m_logpath);
	if(m_logpath[nlen - 1] != '/')
		strcat(m_logpath, "/");

	sprintf(m_logfile, "%s/%s.log", m_logpath, m_progname);
	
	m_npid = getpid();
	
	pthread_mutex_init(&loglock, NULL);
#endif
	
	return 0;
}

int initlog(const char *progname, const char *spriority, const char *logpath, int logopt, long logsize)
{
	int i = 0, nlevel = PRIORITY_INFO / 100;

	for(i = 0; i<= LOG_MAXLEVEL; i++)
	{
		if(mystricmp(spriority, priorities[i]) == 0)
		{
			nlevel = i * 100;
			break;
		}
	}
	return initlogs(progname, nlevel, logpath, logopt, logsize);
}

int freelog()
{
	if(NULL != m_file_log)
	{
		fclose(m_file_log);
	}
#ifdef _WIN32
	DeleteCriticalSection(&critsec);
#else
	pthread_mutex_destroy(&loglock);
#endif
	return 0;
}

int _printlog(const char * filename, const int line, int priority, const char *format, ...)
{
	char smsg[512 - 64],logmsg[512];
	char datetime[20];
	static char lastdate[20];
	va_list args;

	if(priority > m_npriority)
		return 0;
	
	memset(datetime, 0, sizeof(datetime));

#ifdef _WIN32
	EnterCriticalSection(&critsec);
#else
	pthread_mutex_lock(&loglock);
#endif
	
	if(m_nlogopt == 0)
	{
		//固定文件名，备份超额文件
		struct stat fstat;
		if(stat(m_logfile, &fstat) >= 0)
		{
			if(fstat.st_size >= m_nlogsize)//文件大于1M移动文件
			{
				char tmpfile[256];
				char datetime[15];
				if(NULL != m_file_log)
				{
					fclose(m_file_log);
					m_file_log = NULL;
				}
				memset(tmpfile, 0, sizeof(tmpfile));
				memset(datetime, 0, sizeof(datetime));
				strcpy(tmpfile, m_logfile);
				(strrchr(tmpfile,'.'))[1] = 0;
				getshortdatetime(datetime);
				strcat(tmpfile, datetime);
				strcat(tmpfile, ".log");
				rename(m_logfile, tmpfile);
			}
		}
		else
		{
			if(NULL != m_file_log)
			{
				fclose(m_file_log);
				m_file_log = NULL;
			}
		}
	}
	else
	{
		//每天一个文件名
		getshortdate(datetime);
		sprintf(m_logfile, "%s%s%s.log", m_logpath, m_progname, datetime);
		if(strcmp(lastdate, datetime) != 0)
		{
			if(NULL != m_file_log)
			{
				fclose(m_file_log);
				m_file_log = NULL;
			}
			strcpy(lastdate, datetime);
		}
	}
	
	memset(smsg, 0, sizeof(smsg));
	va_start(args, format);
#ifdef _WIN32
	_vsnprintf(smsg, sizeof(smsg) - 1, format, args);
#else
	vsnprintf(smsg, sizeof(smsg) - 1, format, args);
#endif
	va_end(args);
	
	memset(datetime, 0, sizeof(datetime));
	getcurdatetime(datetime);

/*#ifdef _DEBUG

#ifdef _WIN32
	char *pdest = NULL;
	pdest = strrchr(filename, '\\');
	if(!pdest)
		pdest = (char*)filename;
	else
		pdest++;

	_snprintf(logmsg, sizeof(logmsg), "%s %-6.6s %-8ld %-18.18s%-6d - %s\n",
			datetime,
			priority_to_string(priority),
			m_npid,
			pdest, line,
			smsg);
#else
	snprintf(logmsg, sizeof(logmsg), "%s %-6.6s %-8ld %-18.18s%-6d - %s\n",
			datetime,
			priority_to_string(priority), 
			m_npid,
			filename, line,
			smsg);
#endif

	writelogfile(logmsg, strlen(logmsg));
	printf(logmsg);

#else*/


#ifdef _WIN32
	_snprintf(logmsg, sizeof(logmsg), "%s %-6.6s %-8ld - %s\n",
			datetime,
			priority_to_string(priority), 
			m_npid,
			smsg);
#else
	snprintf(logmsg, sizeof(logmsg), "%s %-6.6s %-8ld - %s\n",
			datetime,
			priority_to_string(priority), 
			m_npid,
			smsg);
#endif
	writelogfile(logmsg, strlen(logmsg));
	printf(logmsg);
//#endif

	
#ifdef _WIN32
	LeaveCriticalSection(&critsec);
#else
	pthread_mutex_unlock(&loglock);
#endif
	
	return 0;	
}


