/*
 * Config.cpp
 *
 *  Created on: Jun 17, 2015
 *      Author: root
 */

#include <fstream>
#include <iostream>
#include <string.h>
#include "Config.h"
#include "Logger.h"
#include "EncapMysql.h"
#include "WarningLib.h"

Config * Config::m_instance = NULL;

Config::Config():m_serialPortPath(""),
				 m_speed(0),
				 m_databits(0),
				 m_stopbits(0),
				 m_parity(2),
				 m_netType(2),
				 m_ip(""),
				 m_tcpPort(6000),
				 m_udpPort(6001),
				 m_outTime(3),
				 m_mysqlIp(""),
				 m_mysqlUser(""),
				 m_mysqlPwd(""),
			     m_mysqlPort(0),
				 m_mysqlDbname(""),
				 m_redisIp(""),
				 m_redisPort(0),
				 m_id(1),
				 m_logLv(1),
				 m_heartInterval(60),
				m_redisPostSize(0),
				m_iec103RunType(RS232_RUN)
{
	memset(m_redisPostName, 0, sizeof(m_redisPostName));
	m_clientAddr.clear();
	m_iedId.clear();
	m_iedName.clear();
}

Config * Config::GetInstance()
{
	if(NULL == m_instance)
	{
		m_instance = new Config();
	}
	return m_instance;
}

Config::~Config()
{

}

bool Config::Init(const std::string & configName)
{
	std::string strTempValue("");
	std::vector<std::string> tempValueVec;
	if(!ReadConfig(configName))
	{
		return false;
	}

	if(!GetStrValueByKey("serialPath", m_serialPortPath))
	{
	}

	if(!GetValueByKey("speed", m_speed))
	{
	}

	if(!GetValueByKey("databits", m_databits))
	{
	}

	if(!GetValueByKey("stopbits", m_stopbits))
	{
	}

	if(!GetValueByKey("parity", m_parity))
	{
	}

	if(!GetValueByKey("netType", m_netType))
	{
	}

	if(!GetStrValueByKey("ip", m_ip))
	{
	}

	if(!GetValueByKey("tcpPort", m_tcpPort))
	{
	}

	if(!GetValueByKey("udpPort", m_udpPort))
	{
	}

	if(!GetValueByKey("outTime", m_outTime))
	{
	}

	if(!GetStrValueByKey("mysql_ip", m_mysqlIp))
	{
	}

	if(!GetValueByKey("mysql_port", m_mysqlPort))
	{
	}

	if(!GetStrValueByKey("mysql_user", m_mysqlUser))
	{
	}

	if(!GetStrValueByKey("mysql_pwd", m_mysqlPwd))
	{
	}

	if(!GetStrValueByKey("mysql_dbname", m_mysqlDbname))
	{
	}

	if(!GetStrValueByKey("redis_ip", m_redisIp))
	{
	}

	if(!GetValueByKey("redis_port", m_redisPort))
	{
	}

	if(!GetValueByKey("logLv", m_logLv))
	{
	}

	if(!GetValueByKey("heart_beat_time", m_heartInterval))
	{

	}

	if(!GetValueByKey("ied_id", strTempValue))
	{
		ERROR("no ied_id \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "no ied_id");
		return false;
	}
	else
	{
		tempValueVec=Split(strTempValue, ",");
		std::vector<std::string>::const_iterator cit = tempValueVec.begin();
		uint16_t iedId= 0 ;
		for(; cit != tempValueVec.end(); ++cit)
		{
			iedId = 0;
			if(String2Int((*cit).c_str(), iedId))
			{
				m_iedId.push_back(iedId);
			}
		}
	}
	strTempValue = "";
	tempValueVec.clear();
	if(!GetValueByKey("ied_name", strTempValue))
	{
		ERROR("no ied_name \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "no ied_name");
		return false;
	}
	else
	{
		tempValueVec=Split(strTempValue, ",");
		std::vector<std::string>::const_iterator cit = tempValueVec.begin();
		for(; cit != tempValueVec.end(); ++cit)
		{
			m_iedName.push_back(*cit);
		}
	}

	if((m_iedName.size() != m_iedId.size())
			|| m_iedName.empty()
			|| m_iedId.empty())
	{
		ERROR("ied_name or ied_id error \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "ied_name or ied_id error");
		return false;
	}

	return true;
}


bool Config::IsSpace(char c)
{
    if (' ' == c || '\t' == c)
        return true;
    return false;
}

bool Config::IsCommentChar(char c)
{
    switch(c) {
    case COMMENT_CHAR:
        return true;
    default:
        return false;
    }
}

void Config::Trim(string & str)
{
    if (str.empty()) {
        return;
    }
    uint32_t i, start_pos, end_pos;
    for (i = 0; i < str.size(); ++i) {
        if (!IsSpace(str[i])) {
            break;
        }
    }
    if (i == str.size()) { // 全部是空白字符串
        str = "";
        return;
    }

    start_pos = i;

    for (i = str.size() - 1; i >= 0; --i) {
        if (!IsSpace(str[i])) {
            break;
        }
    }
    end_pos = i;

    str = str.substr(start_pos, end_pos - start_pos + 1);
}

bool Config::AnalyseLine(const string & line, string & key, string & value)
{
    if (line.empty())
        return false;
    int start_pos = 0, end_pos = line.size() - 1, pos;
    if ((pos = line.find(COMMENT_CHAR)) != -1) {
        if (0 == pos) {  // 行的第一个字符就是注释字符
            return false;
        }
        end_pos = pos - 1;
    }
    string new_line = line.substr(start_pos, start_pos + 1 - end_pos);  // 预处理，删除注释部分

    if ((pos = new_line.find('=')) == -1)
        return true;  // 没有=号

    key = new_line.substr(0, pos);
    value = new_line.substr(pos + 1, end_pos + 1- (pos + 1));

    Trim(key);
    if (key.empty()) {
        return false;
    }
    Trim(value);
    return true;
}
//
bool Config::ReadConfig(const string & filename)
{
	m_params.clear();
    ifstream infile(filename.c_str());
    if (!infile) {
    	ERROR("file %s open error \n", filename.c_str());
    	//WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "config file open error");
        return false;
    }
    string line, key, value;
    while (getline(infile, line)) {
        if (AnalyseLine(line, key, value)) {
        	m_params[key] = value;
        	//m_params.insert(key,value);
        }
    }
    infile.close();
    return true;
}


//find  m_params  key
bool Config::GetStrValueByKey(const std::string & key, std::string &value)
{
	std::map<std::string, std::string>::const_iterator cit = m_params.find(key);
	if(cit != m_params.end())
	{
		value = cit->second;
		return true;
	}
	return false;
}

bool Config::InitMysqlParams()
{
	char cmd[512] = {0};
	sprintf(cmd, "select ID FROM ied_process where IED_NAME=\'%s\'",m_iedName[0].c_str());
	//获取通过iedname获取进程编号
	if(0 != EncapMysql::GetInstance()->SelectQuery(cmd))
	{
		ERROR("iedname [%s]no find \n",m_iedName[0].c_str());
		return false;
	}
	if(char** value = EncapMysql::GetInstance()->FetchRow())
	{
		if(!String2Int(value[0], m_id))
		{
			ERROR("convert heartInterval error \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV1, "convert heartInterval error");
			return false;
		}
	}
	else
	{
		WARN("heartInterval get null set to 1 min \n");
	}

	//从点表中获取从站地址
	memset(cmd, 0, sizeof(cmd));
	std::string strCmdTemp = "select DISTINCT TERMINAL from 103_node where IED_NAME in (";
	std::vector<std::string>::const_iterator cit = m_iedName.begin();
	for(; cit != m_iedName.end(); ++cit)
	{
		strCmdTemp+="\'";
		strCmdTemp+=*cit;
		strCmdTemp+="\'";
	}
	strCmdTemp += ")";
	if(0 != EncapMysql::GetInstance()->SelectQuery(strCmdTemp.c_str()))
	{
		return false;
	}
	uint16_t clientAddr = 0;
	while (char** addr = EncapMysql::GetInstance()->FetchRow())
	{
		clientAddr = 0;
		if(!String2Int(addr[0], clientAddr))
			{
				ERROR("convert clientAddr error \n");
				WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "convert clientAddr error");
				return false;
			}
		m_clientAddr.push_back(clientAddr);
	}
	if(m_clientAddr.empty())
	{
		ERROR("cmd[%s] return nothing \n ", strCmdTemp.c_str());
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "clientAddr is null ");
		return false;
	}

	//从通信表中获取配置数据
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "select BAUD_RATE, DATA_BIT, STOP_BIT, CHECK_BIT, SERIAL_PORT_NAME, IP, PORT, REMARK, COMM_MODE "\
			"FROM communication where IED_ID=%d", m_iedId[0]);
	if(0 != EncapMysql::GetInstance()->SelectQuery(cmd))
	{
		return false;
	}

	if(char** value = EncapMysql::GetInstance()->FetchRow())
	{
		if(!String2Int(value[0], m_speed))
		{
			ERROR("convert speed error \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "convert speed error");
			return false;
		}
		if(!String2Int(value[1], m_databits))
		{
			ERROR("convert databits error \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "convert databits error");
			return false;
		}
		if(!String2Int(value[2], m_stopbits))
		{
			ERROR("convert stopbits error \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "convert stopbits error");
			return false;
		}
		//uint16_t parity;
		if(!String2Int(value[3], m_parity))
		{
			ERROR("convert parity error \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "convert parity error");
			return false;
		}

		if(NULL == value[4])
		{
			ERROR("convert serialPortPath error \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "convert serialPortPath error");
			return false;
		}
		m_serialPortPath = value[4];
		if(NULL == value[5])
		{
			ERROR("convert ip error \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "convert ip error");
			return false;
		}
		m_ip=value[5];
		if(!String2Int(value[6], m_tcpPort))
		{
			ERROR("convert tcpPort error \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "convert tcpPort error");
			return false;
		}
		std::string strRedisPostName(value[7]);
		vector<std::string> vecName;
		char strNameTemp[1000] = {0};
		vecName = Split(strRedisPostName, ",");

		for(uint32_t i = 0; i < vecName.size(); ++i)
		{
			uint16_t len = 0;
			len = ((vecName[i].length() > 10)?10:vecName[i].length());
			memcpy(strNameTemp + 10*i, vecName[i].c_str(), len);
		}
		memcpy(m_redisPostName, strNameTemp, sizeof(m_redisPostName));
		m_redisPostSize = vecName.size();

		if(!String2Int(value[8], m_netType))
		{
			ERROR("convert netType error \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "convert netType error");
			return false;
		}
	}
	else
	{
		ERROR("select communication error \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "select communication error");
		return false;
	}
	return true;
}

//字符串分割函数
std::vector<std::string> Config::Split(std::string str,std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str+=pattern;//扩展字符串以方便操作
	uint32_t size=str.size();
	for(uint32_t i=0; i<size; i++)
	{
		pos=str.find(pattern,i);
		if(pos<size)
		{
			std::string s=str.substr(i,pos-i);
			result.push_back(s);
			i=pos+pattern.size()-1;
		}
	}
	return result;
}






