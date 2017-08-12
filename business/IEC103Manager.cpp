/*
 * IEC103Manager.cpp
 *
 *  Created on: Jun 19, 2015
 *      Author: root
 */

#include "IEC103Manager.h"


IEC103Manager::IEC103Manager():m_service(NULL)
{

}

IEC103Manager::~IEC103Manager()
{
	delete m_service;
}

bool IEC103Manager::Init()
{
	//从配置文件初始化配置参数 mysql连接的ip usr pwd
	if(!Config::GetInstance()->Init(CONFIGPATHNAME))
	{
		std::cout << "init config error" << std::endl;
		return false;
	}

	//初始化日志文件
	InitLogging("",(LogLevel)Config::GetInstance()->GetLogLv(),LOGPATH);

	signal(SIGTERM, SignalHandle);

	//初始化mysql
	std::string strMysqlIp = Config::GetInstance()->GetMysqlIp();
	std::string strMysqlUser = Config::GetInstance()->GetMysqlUser();
	std::string strMysqlPwd = Config::GetInstance()->GetMysqlPwd();
	std::string strMysqlDbname = Config::GetInstance()->GetMysqlDbname();
	uint16_t   mysqlPort = Config::GetInstance()->GetMysqlPort();
	if(0 != EncapMysql::GetInstance()->Connect(strMysqlIp.c_str(), strMysqlUser.c_str(), strMysqlPwd.c_str(), strMysqlDbname.c_str(), mysqlPort))
	{
		std::cout << "EncapMysql error" << std::endl;
		return false;
	}

	//初始化告警库
//	if(!WarningLib::GetInstance()->Init(strMysqlIp.c_str(), strMysqlUser.c_str(), strMysqlPwd.c_str(), strMysqlDbname.c_str(), mysqlPort))
//	{
//		return false;
//	}
//	if(!WarningLib::GetInstance()->Start())
//	{
//		return false;
//	}

	//从mysql中读取配置参数初始化本地配置参数
//	if(!Config::GetInstance()->InitMysqlParams())
//	{
//		std::cout << "Config read d eata rror" << std::endl;
//		return false;
//	}

	//初始化内存数据库
//	std::string redisAddr = Config::GetInstance()->GetRedisIp();
//	uint16_t    redisPort = Config::GetInstance()->GetRedisPort();
//	if(!MemCache::GetInstance()->Init(redisAddr, redisPort))
//	{
//		std::cout << "memcache error" << std::endl;
//		return false;
//	}

	//初始化服务
	uint16_t netType = Config::GetInstance()->GetNetType();
	std::string strNetType("");
	if(1 == netType)
	{
		if(Config::RS485_CLIENT_RUN == Config::GetInstance()->GetIec103RunType())
		{
			m_service = new Serial485ClientService();
		}
		else
		{
			m_service = new SerialService();
		}

		strNetType = "serial";
	}
	else if(2 == netType)
	{
		//网络通信
		strNetType = "Internet";
		//m_service = new TcpService();
		m_service = new Service();
		//return false;
	}
	else
	{
		ERROR("invalid netType \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "invalid netType");
		return false;
	}

	INFO("IEC103 Start with %s \n",strNetType.c_str());

	if(!m_service->Init())
	{
		ERROR("IEC103 init failed \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "IEC103 init failed");
		return false;
	}

	//更新pid到采集进程表
//	char cmd[512] = {0};
//	sprintf(cmd, "update collect_process set PROCESS_ID=%d WHERE id=%d;", getpid(), Config::GetInstance()->GetId());
//	if(0 != EncapMysql::GetInstance()->ModifyQuery(cmd))
//	{
//		std::cout << "update Process Error "<<  getpid()<< "  "<< Config::GetInstance()->GetId()<<std::endl;
//		return false;
//	}

	return true;
}

void IEC103Manager::Start()
{
	m_service->Start();  //启动服务
}

void IEC103Manager::Stop()
{
	WarningLib::GetInstance()->Stop();
}

void IEC103Manager::SignalHandle(int sigNo)
{
	INFO("received SIGTERM IEC103 pid[%d] exit\n", getpid() );
	WarningLib::GetInstance()->Stop();
	exit(0);
}
