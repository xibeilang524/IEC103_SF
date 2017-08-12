/*
 * Logger.h
 *
 *  Created on: 2013Äê9ÔÂ10ÈÕ
 *      Author: Administrator
 */

#ifndef LOGGER_H_
#define LOGGER_H_

//#include "ILogger.h"
#include "Mutex.h"
#include<fstream>
#include<string>

using namespace std;

const int MaxFilePathLen = 1024;
const char PathSplitChar = '/';

enum LogLevel{
    /// <summary>
    /// 调试
    /// </summary>
    DEBUG = 0,
    /// <summary>
    /// 普通日志
    /// </summary>
    INFO = 1,
    /// <summary>
    /// 警告
    /// </summary>
    WARN = 2,
    /// <summary>
    /// 错误
    /// </summary>
    ERROR = 3,
    /// <summary>
    /// 崩溃
    /// </summary>
    FATAL = 4,
    /// <summary>
    /// 超出错误级别
    /// </summary>
    OFF = 5

};


class ILogger {
public:
    //
    virtual ~ILogger(){}
#define ABSTRACT_LOG_FUNC(name) \
    virtual void name(string msg)=0; \
    virtual void name(const char* fmt,...)=0;

    ABSTRACT_LOG_FUNC(Debug)
    ABSTRACT_LOG_FUNC(Info)
    ABSTRACT_LOG_FUNC(Warn)
    ABSTRACT_LOG_FUNC(Error)
    ABSTRACT_LOG_FUNC(Fatal)

#undef ABSTRACT_LOG_FUNC
#define ABSTRACT_LOG_FUNC_X(name) \
    virtual void name(LogLevel lv,string msg)=0; \
    virtual void name(LogLevel lv,const char* fmt,...)=0;\
    virtual void name(const char* file,int line,LogLevel lv,string msg)=0;\
    virtual void name(const char* file,int line,LogLevel lv,const char* fmt,...)=0;
    ABSTRACT_LOG_FUNC_X(Log)

#undef LOG_FUNC_X
};


class Logger: public ILogger {
    std::string logPath;
    std::string logPrefix;
    std::fstream logFile;
    LogLevel level;
    Mutex mutex;

    Logger(LogLevel level,char * folder,char * prefix);

public:
    static Logger& GetInstance();
    static Logger* GetInstancePtr();
    virtual ~Logger();
    inline fstream & stream(){return logFile;}

#define DECLARE_LOG_FUNC(name) \
    virtual void name(string msg); \
    virtual void name(const char* fmt,...);

#define DECLARE_LOG_FUNC_X(name) \
    virtual void name(LogLevel lv,string msg); \
    virtual void name(LogLevel lv,const char* fmt,...);\
    virtual void name(const char* file,int line,LogLevel lv,string msg);\
    virtual void name(const char* file,int line,LogLevel lv,const char* fmt,...);

    DECLARE_LOG_FUNC(Debug)
    DECLARE_LOG_FUNC(Info)
    DECLARE_LOG_FUNC(Warn)
    DECLARE_LOG_FUNC(Error)
    DECLARE_LOG_FUNC(Fatal)

    DECLARE_LOG_FUNC_X(Log)

#undef DECLARE_LOG_FUNC_X
#undef DECLARE_LOG_FUNC

};

class LogMessage {
    Logger* logger;
    static Mutex mutex;
public:
    LogMessage(const char* file, int line,LogLevel lv);
    ostream& stream(){return logger->stream();}
    virtual ~LogMessage();
};


void InitLogging(const char* filename,LogLevel minlevel,const char* destFolder);
void CloseLogging();

//#define LOG(level) LogMessage(__FILE__, __LINE__,level).stream()
#define DEBUG(format, ...) Logger::GetInstance().Log(__FILE__, __LINE__,DEBUG,format, ##__VA_ARGS__)
#define INFO(format, ...) Logger::GetInstance().Log(__FILE__, __LINE__,INFO,format, ##__VA_ARGS__)
#define WARN(format, ...) Logger::GetInstance().Log(__FILE__, __LINE__,WARN,format, ##__VA_ARGS__)
#define ERROR(format, ...) Logger::GetInstance().Log(__FILE__, __LINE__,ERROR,format, ##__VA_ARGS__)
//#define DEBUG() LOG(DEBUG)
//#define INFO() LOG(INFO)
//#define WARN() LOG(WARN)
//#define ERROR() LOG(ERROR)
#define LOG_IF(severity, condition) \
    !(condition) ? (void) 0 : LOG(severity)
#define LOG_ASSERT(condition)  \
    LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition
#define CHECK(condition)  \
    LOG_IF(FATAL, !(condition)) \
    << "Check failed: " #condition " "

#endif /* LOGGER_H_ */
