/*
 * Config.h
 *
 *  Created on: Jun 17, 2015
 *      Author: root
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>
#include <stdint.h>
#include <sstream>
#include <map>
#include <vector>

#define COMMENT_CHAR '#'

using namespace std;

class Config
{
public:
	static Config * GetInstance();
	bool Init(const std::string & configName);
	bool InitMysqlParams();
public:
	enum IEC103_RUN_TYPE
	{
		RS232_RUN = 1,
		RS485_CLIENT_RUN,
		RS485_SERVER_RUN
	};
public:
	std::string GetSerialPortPath()
	{
		return m_serialPortPath;
	}
	int32_t GetSpeed()
	{
		return m_speed;
	}
	int32_t GetDatabits()
	{
		return m_databits;
	}
	int32_t GetStopbits()
	{
		return m_stopbits;
	}
	uint16_t GetParity()
	{
		return m_parity;
	}
	const std::vector<uint16_t> & GetClientAddr()
	{
		return m_clientAddr;
	}
	uint16_t GetNetType()
	{
		return m_netType;
	}
	std::string GetIp()
	{
		return m_ip;
	}
	uint16_t GetTcpPort()
	{
		return m_tcpPort;
	}
	uint16_t GetUdpPort()
	{
		return m_udpPort;
	}
	uint16_t GetOutTime()
	{
		return m_outTime;
	}
	std::string GetMysqlIp()
	{
		return m_mysqlIp;
	}
	uint16_t GetMysqlPort()
	{
		return m_mysqlPort;
	}
	std::string GetMysqlUser()
	{
		return m_mysqlUser;
	}
	std::string GetMysqlPwd()
	{
		return m_mysqlPwd;
	}
	std::string GetMysqlDbname()
	{
		return m_mysqlDbname;
	}
	std::string GetRedisIp()
	{
		return m_redisIp;
	}
	uint16_t GetRedisPort()
	{
		return m_redisPort;
	}
	uint16_t GetId()
	{
		return m_id;
	}
	int32_t GetLogLv()
	{
		return m_logLv;
	}
	uint16_t GetHeartInterval()
	{
		return m_heartInterval;
	}
	const std::vector<uint16_t> & GetIedId()
	{
		return m_iedId;;
	}
	const std::vector<std::string> & GetIedName()
	{
		return m_iedName;
	}
	char* GetRedisPostName()
	{
		return m_redisPostName;
	}
	uint16_t GetRedisPostSize()
	{
		return m_redisPostSize;
	}
	IEC103_RUN_TYPE GetIec103RunType()
	{
		return m_iec103RunType;
	}

private:
	bool ReadConfig(const std::string & configName);
	bool IsSpace(char c);
	bool IsCommentChar(char c);
	void Trim(string & str);
	bool AnalyseLine(const string & line, string & key, string & value);
	template<typename T>
	bool GetValueByKey(const std::string & key, T &value);
	bool GetStrValueByKey(const std::string & key, std::string &value);
	std::vector<std::string> Split(std::string str,std::string pattern);
private:
	Config();
	virtual ~Config();
	template<typename T>
	bool String2Int(const char * strValue, T & value)
	{
		if(NULL == strValue)
		{
			return false;
		}
		stringstream ss;
		ss << strValue;
		ss >> value;
		if(ss.fail())
		{
			return false;
		}
		return true;
	}
private:
	std::string m_configName;
	std::map<std::string, std::string> m_params; //配置参数键值对
	static Config * m_instance;

	std::string m_serialPortPath;
	int32_t     m_speed;
	int32_t     m_databits;
	int32_t     m_stopbits;
	uint16_t        m_parity;
	uint16_t     m_netType;
	std::string  m_ip;
	uint16_t     m_tcpPort;
	uint16_t     m_udpPort;
	std::vector<uint16_t> m_clientAddr;

	uint16_t     m_outTime;
	std::string m_mysqlIp;
	std::string m_mysqlUser;
	std::string m_mysqlPwd;
	uint16_t    m_mysqlPort;
	std::string m_mysqlDbname;
	std::string m_redisIp;
	uint16_t    m_redisPort;
	uint16_t    m_id; // 进程表主键ID，进程编号
	int32_t     m_logLv;
	uint16_t m_heartInterval;
	std::vector<uint16_t>    m_iedId;
	std::vector<std::string> m_iedName;
	char m_redisPostName[1000];
	uint16_t    m_redisPostSize;
	IEC103_RUN_TYPE m_iec103RunType;
};

template<typename T>
bool Config::GetValueByKey(const std::string & key, T &value)
{
	std::map<std::string, std::string>::const_iterator cit = m_params.find(key);
	if(cit != m_params.end())
	{
		std::string strValue = cit->second;
		stringstream ss(strValue);
		ss >> value;
		return true;
	}
	return false;
}



#endif /* CONFIG_H_ */
