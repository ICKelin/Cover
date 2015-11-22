#ifndef __PRINTLOG__H__
#define __PRINTLOG__H__

#define ELOG __FILE__,__LINE__

//日志级别格式字符串
const char* const priorities[] = {
	"FATAL", 
	"ALERT",
	"CRIT",
	"ERROR",
	"WARN",
	"NOTICE",
	"INFO",
	"DEBUG",
	"TRACE",
	"NOTSET",
	"UNKNOWN"
};

//日志级别枚举变量
typedef enum {
	PRIORITY_FATAL	= 000,
	PRIORITY_ALERT	= 100, 
	PRIORITY_CRIT	= 200, 
	PRIORITY_ERROR	= 300, 
	PRIORITY_WARN	= 400, 
	PRIORITY_NOTICE	= 500, 
	PRIORITY_INFO	= 600, 
	PRIORITY_DEBUG	= 700,
	PRIORITY_TRACE	= 800,
	PRIORITY_NOTSET	= 900,
	PRIORITY_UNKNOWN	= 1000
} priority_level;

/********************************************************************************
* initlog:	初始化日志配置														*
* 参数说明:	-	progname:	运行的程序名，main()函数中可将argv[0]传入			*
*							windows下调用传入NULL即可							*
*				priority:	日志级别，打印该级别及以下的日志					*
*							使用priority_level中定义的数值						*
*				logpath:	日志文件存放路径，如果为NULL则使用当前目录			*
*				logopt:		日志文件产生规则，0固定名称及备份，1每天一个文件名	*
*				logsize:	命名规则为0时，当文件大于该值时备份，单位为K		*
********************************************************************************/
int initlogs(const char *progname, int priority, const char *logpath,
			int logopt, long logsize);

/********************************************************************************
* initlog:	初始化日志配置														*
* 参数说明:	-	progname:	运行的程序名，main()函数中可将argv[0]传入			*
*							windows下调用传入NULL即可							*
*				spriority:	日志级别，打印该级别及以下的日志					*
*							使用priorities中定义的字符串常量					*
*				logpath:	日志文件存放路径，如果为NULL则使用当前目录			*
*				logopt:		日志文件产生规则，0固定名称及备份，1每天一个文件名	*
*				logsize:	命名规则为0时，当文件大于该值时备份，单位为K		*
********************************************************************************/
int initlog(const char *progname, const char *spriority, const char *logpath,
			int logopt, long logsize);

/********************************************************************************
* initlog:	释放log资源													*
* 参数说明:	-															*
********************************************************************************/
int freelog();

/********************************************************************************
* initlog:	初始化日志配置														*
* 参数说明:	-	filename:	当前代码所在文件名									*
*				line:		当前代码所在行数									*
*				priority:	日志的级别，用枚举变量priority_level里的数值传入	*
*				format:		同printf一样的格式，传入字符串格式和参数			*
* 其他说明:	filename和line两个参数可以用已定义的宏定义ELOG传入					*
********************************************************************************/
int _printlog(const char * filename, const int line, int priority, const char *format, ...);

#endif //__PRINTLOG__H__
