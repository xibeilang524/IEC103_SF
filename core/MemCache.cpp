/*
 * MemCache.cpp
 *
 *  Created on: Jun 16, 2015
 *      Author: root
 */
#include <dlfcn.h>
#include <unistd.h>
#include "MemCache.h"
#include "Logger.h"
#include "Config.h"
#include "WarningLib.h"

#define REDIS_NAME "redis.so"

MemCache * MemCache::m_instance = NULL;

MemCache::MemCache():m_handle(NULL),m_redisAddr(""),m_redisPort(0) {
	Pfun_InitDB = NULL;
	Pfun_UnInitDB = NULL;
	Pfun_Connect = NULL;
	Pfun_DisConnect = NULL;
	Pfun_WritePointsValue_C = NULL;
	Pfun_WritePointsValue_V = NULL;
	Pfun_WritePointValue = NULL;
}

MemCache::~MemCache()
{
	Free();
}

MemCache * MemCache::GetInstance()
{
	if(NULL == m_instance)
	{
		m_instance = new MemCache();
	}
	return m_instance;
}

bool MemCache::Init(const std::string & redisAddr, uint16_t redisPort)
{
	m_redisAddr = redisAddr;
	m_redisPort = redisPort;
	char buf[512] = {0};
	getcwd(buf, sizeof(buf));
	std::string strRedisPath(buf);
	strRedisPath.append("/").append(REDIS_NAME);

	m_handle = dlopen(strRedisPath.c_str(), RTLD_LAZY); /*打开动态链接库*/
	if(m_handle == NULL)
	{
		ERROR("dlopen error:%s \n", dlerror());
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "redis.so dlopen error");
		 return false;
	}

	//获取函数指针
	if(!GetFunction(reinterpret_cast<Handle *>(&Pfun_InitDB), "InitDB"))
	{
		return false;
	}

	if(!GetFunction(reinterpret_cast<Handle *>(&Pfun_UnInitDB), "UnInitDB"))
	{
		return false;
	}

	if(!GetFunction(reinterpret_cast<Handle *>(&Pfun_Connect), "Connect"))
	{
		return false;
	}

	if(!GetFunction(reinterpret_cast<Handle *>(&Pfun_DisConnect), "DisConnect"))
	{
		return false;
	}

	if(!GetFunction(reinterpret_cast<Handle *>(&Pfun_WritePointsValue_C), "WritePointsValue_C"))
	{
		return false;
	}

	if(!GetFunction(reinterpret_cast<Handle *>(&Pfun_WritePointsValue_V), "WritePointsValue_V"))
	{
		return false;
	}

	if(!GetFunction(reinterpret_cast<Handle *>(&Pfun_WritePointValue), "WritePointValue"))
	{
		return false;
	}
	if(!GetFunction(reinterpret_cast<Handle *>(&Pfun_post), "post"))
	{
		return false;
	}
	if(!GetFunction(reinterpret_cast<Handle *>(&Pfun_CheckConnection), "check_connection"))
	{
		return false;
	}

	if(!Pfun_InitDB())
	{
		ERROR("redis init error \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "redis init error");
		return false;
	}

	if(!Pfun_Connect(0, m_redisAddr.c_str(), m_redisPort))
	{
		ERROR("redis Connect error \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "redis Connect error");
		return false;
	}

	return true;
}

bool MemCache::GetFunction(Handle * handle, const char* funcName)
{
	*handle = dlsym(m_handle,funcName);
	char * errorInfo = dlerror();
	if (errorInfo != NULL){
		ERROR("%s method get failed%s\n", funcName, errorInfo);
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, " redis dlsym error");
		return false;
	}
	return true;
}

bool MemCache::WritePointsValue_V(std::vector<Tag>& m_tag_ver)
{
	int count = 0;
	while(true)
	{
		if(!Pfun_CheckConnection(0))
		{
			if(3 == count)
			{
				ERROR("WritePointsValue_V error \n");
				WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, " WritePointsValue_V error");
				return false;
			}
			WARN("redis reconnect \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV0, "redis reconnect");
			sleep(3);
			Pfun_DisConnect();
			Pfun_Connect(0, m_redisAddr.c_str(), m_redisPort);
			++count;

			continue;
		}
		else
		{
			break;
		}
	}

	bool ret = true;
	if(!Pfun_WritePointsValue_C(&m_tag_ver[0], m_tag_ver.size()))
	{
		ret = false;
	}
	Post(Config::GetInstance()->GetRedisPostName(), Config::GetInstance()->GetRedisPostSize());
	return ret;
}

/*
* @Desc: 数据更新通知
* @Param: pCh通道名数组
* @Param: m_ch_size 通道个数
* @Return: 成功或失败
*/
void MemCache::Post( char *pCh,int m_ch_size)
{
	return Pfun_post(pCh, m_ch_size);
}

void MemCache::Free()
{
	if((Pfun_DisConnect != NULL)
			&&(Pfun_UnInitDB != NULL))
	{
		Pfun_DisConnect();
		Pfun_UnInitDB();
	}

}
