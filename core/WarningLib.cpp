/*
 * WarningLib.cpp
 *
 *  Created on: Jul 30, 2015
 *      Author: root
 */

#include "WarningLib.h"
#include <dlfcn.h>
#include <unistd.h>
#include "Logger.h"
#include "Config.h"

WarningLib * WarningLib::m_instance = NULL;


WarningLib::WarningLib()
{

}

WarningLib::~WarningLib() {

}

WarningLib * WarningLib::GetInstance()
{
	if(NULL == m_instance)
	{
		m_instance = new WarningLib();
	}
	return m_instance;
}

/*
 * @Desc: 初始化链接
 * @Param: szDbIp 要链接的mysql数据库IP地址
 * @Param: szUser 要链接的mysql数据库用户名
 * @Param: szPassword 要链接的mysql数据库密码
 * @Param: szDb 要链接的mysql数据库名
 * @Param: port 要链接的mysql数据库端口号
 * @Return:成功或失败
 */
bool WarningLib::Init(const char* szDbIp, const char* szUser,
	const char* szPassword,const char* szDb, uint32_t port)
{
	char buf[512] = {0};
	getcwd(buf, sizeof(buf));
	std::string strRedisPath(buf);
	strRedisPath.append("/").append("libWarning.so");

	m_handle = dlopen(strRedisPath.c_str(), RTLD_LAZY); /*打开动态链接库*/
	if(m_handle == NULL)
	{
		ERROR("dlopen error:%s \n", dlerror());
		return false;
	}

	if(!GetFunction(reinterpret_cast<void* &>(pInit), "Init"))
	{
		return false;
	}
	if(!GetFunction(reinterpret_cast<void* &>( pStart), "Start"))
	{
		return false;
	}
	if(!GetFunction(reinterpret_cast<void* &>( pStop), "Stop"))
	{
		return false;
	}
	if(!GetFunction(reinterpret_cast<void* &>( pWriteWarn), "WriteWarn"))
	{
		return false;
	}
	if(!pInit(szDbIp, szUser, szPassword, szDb, port, NULL))
	{
		ERROR("libwarning init error \n");
		return false;
	}
	return true;
}

bool WarningLib::GetFunction(Handle & handle, const char* funcName)
{
	handle = dlsym(m_handle,funcName);
	char * errorInfo = dlerror();
	if (errorInfo != NULL){
		ERROR("%s method get failed%s\n", funcName, errorInfo);
		return false;
	}
	return true;
}

/*
 * @Desc: 启动告警链接，调用前需调用Init
 * @Return:成功或失败
 */
bool WarningLib::Start()
{
	if(!pStart())
	{
		return false;
	}
	return true;
}

/*
 * @Desc: 将告警数据写入数据库
 * @Param: warnInfo 要写入的告警信息
 * @Return:成功或失败
 */
bool WarningLib::WriteWarn(const WarnInfo * warnInfo )
{
	if(!pWriteWarn(warnInfo))
	{
		return false;
	}
	return true;
}

bool WarningLib::WriteWarn2(WarningLevlel warninglv, const char* errStr)
{
	WarnInfo warnInfo;
	warnInfo.processIndex = Config::GetInstance()->GetId();
	warnInfo.warnLv = warninglv;
	warnInfo.warnClass = 0;
	warnInfo.warnReason = 0;
	warnInfo.warnType = 0;
	warnInfo.warnDesc = errStr;
	WriteWarn(&warnInfo);
	return true;
}

/*
 * @Desc: 停止告警链接
 * @Return:成功或失败
 */
bool WarningLib::Stop()
{
	if(!pStop())
	{
		return false;
	}
	return true;
}
