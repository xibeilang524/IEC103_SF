/*
 * SerialService.cpp
 *
 *  Created on: Jun 15, 2015
 *      Author: root
 */

#include "SerialService.h"
#include <iostream>
#include <unistd.h>
#include <string.h>

#include "../core/TcpNet.h"
#include "../core/SerialNet.h"
#include "../core/Logger.h"
#include "../core/WarningLib.h"

SerialService::SerialService():Service()
{
	memset(m_dataBuffer, 0, sizeof(m_dataBuffer));

	m_iec103CodeS2C.fcv = 0;
	m_iec103CodeS2C.fcb = 0;

	m_nextCmd = CMD_RESET_NUM;
}

SerialService::~SerialService()
{

}

void SerialService::Start()
{
	uint16_t netType = Config::GetInstance()->GetNetType();
	switch(netType)
	{
	case 1:
		m_net = new SerialNet(m_portPath, m_speed, m_databits, m_stopbits, m_parity, m_outTime);
		break;
	case 2:
		m_net = new TcpNet(m_tcpip,m_tcpport);
		break;
	default:
		std::cout<<"Start Error :Unknown Type ="<<netType<<std::endl;
		return ;
		break;
	}


	//连接
	if(!m_net->Connect())
	{
		ERROR( "serial port connect failed \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "serial port connect failed");
		return;
	}

	while(true)
	{
		//usleep(50*1000);
		 if(!Write())
		 {

			 break;
		 }
		 //usleep(50*1000);
		 if(!Read())
		 {
			 break;
		 }
	}

}
//com send data
bool SerialService::Write()
{
	std::vector<char> strCmd;
	m_iec103CodeS2C.fcv = FCV_1;
	std::string cmdName("");
	switch(m_nextCmd)
	{
		case CMD_RESET_CON:
			strCmd = CmdResetCon();
			break;
		case CMD_RESET_NUM:
			strCmd = CmdResetNum();
			break;
		case CMD_GET_ALL:
			strCmd = CmdGetAll();
			break;
		case CMD_GET_DATA_LV1:
			strCmd = CmdGetDataLv1();
			break;
		case CMD_GET_DATA_LV2:
			strCmd = CmdGetDataLv2();
			break;
		case CMD_GENERAL_READ_YX_GROUP_VALUE:
			strCmd = CmdGetGroupValue(2); //TODO 遥信组号为2，需要确认不同厂家是否一致
			break;
		case CMD_GENERAL_READ_YC_GROUP_VALUE:
			strCmd = CmdGetGroupValue(0);//TODO 遥测组号为0，需要确认不同厂家是否一致
			break;
		default:
			ERROR("cmd not defined \n" );
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "cmd not defined");
			return false;
	}
	DEBUG("Write[%s]:%s \n", g_cmdName[m_nextCmd], ToHexString(strCmd.data(), strCmd.size()).c_str());
	//TODO:测试时使用
	printf("write[%s]: ", g_cmdName[m_nextCmd]);
	printf("%s \n", ToHexString(strCmd.data(), strCmd.size()).c_str());

	if(-1 == m_net->Write(strCmd.data(), strCmd.size()))
	{
		ERROR("send cmd %d error\n", m_nextCmd);
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "send data error");
		return false;
	}
	return true;
}

bool SerialService::Read()
{
	memset(m_dataBuffer, 0, sizeof(m_dataBuffer));
	static uint16_t reSendTimes = 0; //同一指令从发次数（不包括复位通信）
	int readSize = m_net->Read(m_dataBuffer, sizeof(m_dataBuffer));
	if(-1 == readSize)
	{
		return false;
	}
	else if(-2 == readSize) //读到无效的数据重发
	{
		//TODO:测试时使用
		printf("read[%s]: ",g_cmdName[m_nextCmd]);
		printf("%s \n", ToHexString(m_dataBuffer, strlen(m_dataBuffer)).c_str());
	}
	else if(0 == readSize)//未读到数据
	{
		printf("read[%s]: no data\n",g_cmdName[m_nextCmd]);
		WARN("read[%s]: no data\n",g_cmdName[m_nextCmd]);
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV0, "receive no data");

		if(CMD_RESET_CON == m_nextCmd) //如果上一次指令是通信复位不做重发计数
		{
			//sleep(1);
			return true;
		}
	}
	else
	{
		reSendTimes = 0;
		DEBUG("read[%s]:%s \n", g_cmdName[m_nextCmd], ToHexString(m_dataBuffer, readSize).c_str());
		if(START_68H == m_dataBuffer[0])
		DEBUG("{code=%d, ASDU=%d, FUN=%d, INF=%d} \n", (uint8_t)m_dataBuffer[4], (uint8_t)m_dataBuffer[6], (uint8_t)m_dataBuffer[10], (uint8_t)m_dataBuffer[11]);
		//TODO:测试时使用
		printf("read[%s]: ",g_cmdName[m_nextCmd]);
		printf("%s \n", ToHexString(m_dataBuffer, readSize).c_str());
		if(START_68H == m_dataBuffer[0])
		printf("{code=%d, ASDU=%d, FUN=%d, INF=%d} \n", (uint8_t)m_dataBuffer[4], (uint8_t)m_dataBuffer[6], (uint8_t)m_dataBuffer[10], (uint8_t)m_dataBuffer[11]);
		if(!ParseRecvData(m_dataBuffer, readSize))
		{
			WARN("recv invalied data: %s\n", ToHexString(m_dataBuffer, readSize).c_str());
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV0, "recv invalied data");
		}
		return true;

	}

	//重发计数超过最大重发次数则进行复位通信
	if(++reSendTimes >= m_maxReSendTimes)
	{
		reSendTimes = 0;
		m_nextCmd = CMD_RESET_CON;
		m_isResetConEnd = false;
		m_isResetNumEnd = false;
		WARN("cmd send thress times , cmd resetcon send \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV1, "Link disconnect , cmd resetcon send");
	}
	//sleep(m_sendInterval);

	return true;
}

/*
 * @Desc: 解析接收到的数据
 * @Param: pData　解析数据开始地址
 * @Param: dataLen　解析数据长度
 * @Return: 无
 */
bool SerialService::ParseRecvData(const char* pData, int32_t dataLen)
{
    if(dataLen < 5)
    {
    	return false;
    }
	if(START_10H == pData[0]) //固定长度报文(固定五个字节)
	{
		ParseFixedData(pData, dataLen); // 处理定长数据
	}
	else if(START_68H == pData[0]) //可变长报文(最少14个字节)
	{
		if(!ParseVariableData(pData, dataLen)) // 处理可变长数据
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}

/*
 * @Desc: 处理接收到的固定长度报文
 * @Param: pData　解析数据开始地址
 * @Param: dataLen　解析数据长度
 * @Return: 解析成功或失败
 */
bool SerialService::ParseFixedData(const char* pData, int32_t dataLen)
{
	m_iec103CodeS2C.fcb ^= FCB_1;

	//通信复位的确认处理
	if(CMD_RESET_CON == m_nextCmd
			|| CMD_RESET_NUM == m_nextCmd)
	{
		m_nextCmd = CMD_GET_DATA_LV1;
		return true;
	}

	//判断是否有一级数据上送,有则继续召唤一级数据，没有则召唤二级数据
	if ( (pData[1]&ACD_1) == ACD_1)
	{
		m_nextCmd = CMD_GET_DATA_LV1;
	}
	else
	{
		m_nextCmd = CMD_GET_DATA_LV2;
	}

	if(CMD_GET_DATA_LV2 == m_nextCmd
			&& (pData[1] & 0x0F) == 9) //从站没有所召唤的数据
	{
		sleep(1);
	}
	return true;
}

/*
 * @Desc: 处理接收到的可变长度报文
 * @Param: pData　解析数据开始地址
 * @Param: dataLen　解析数据长度
 * @Return: 解析成功或失败
 */
bool SerialService::ParseVariableData(const char* pData, int32_t dataLen)
{
	m_iec103CodeS2C.fcb ^= FCB_1;
	uint8_t asduType = pData[6]; // asdu类型

	if((CMD_GET_DATA_LV1 == m_nextCmd)
			&& (5 == asduType)) //复位通信或复位帧计数位一级数据召唤
	{
		if((pData[4]&ACD_1) != ACD_1 )
		{
			if(!m_isResetConEnd)
			{
				m_isResetConEnd = true; //复位通信单元完成
				m_nextCmd = CMD_RESET_NUM; //复位帧计数位
				return true;
			}

			if(!m_isResetNumEnd)
			{
				m_isResetNumEnd = true; //复位帧计数位完成
				m_nextCmd = CMD_GET_ALL; //复位帧计数位
				return true;
			}
		}
		return true;
	}

	//判断是否有一级数据上送,有则继续召唤一级数据，没有则召唤二级数据
	if ( (pData[4]&ACD_1) == ACD_1 )
	{
		m_nextCmd = CMD_GET_DATA_LV1;
	}
	else
	{
		m_nextCmd = CMD_GET_DATA_LV2;
	}

	switch(asduType)
	{
		case 1:
			ParseASDU1(pData, dataLen);
			break;
		case 10:
			ParseASDU10(pData, dataLen);
			break;
		case 40:
			ParseASDU40(pData, dataLen);
			break;
		case 41:
			ParseASDU41(pData, dataLen);
			break;
		case 44:
			ParseASDU44(pData, dataLen);
			break;
		case 50:
			ParseASDU50(pData, dataLen);
			break;
		default:
			break;
	}
	return true;
}

/*
 * @Desc: 从配置文件初始化具体类型网络需要的配置
 * @Return: 无
 */
void SerialService::InitConfig()
{

	m_portPath = Config::GetInstance()->GetSerialPortPath();
	m_speed = Config::GetInstance()->GetSpeed();
	m_databits = Config::GetInstance()->GetDatabits();
	m_stopbits = Config::GetInstance()->GetStopbits();
	m_parity = Config::GetInstance()->GetParity();

	m_outTime = Config::GetInstance()->GetOutTime();

	m_clientAddr = static_cast<uint8_t>(Config::GetInstance()->GetClientAddr()[0]);

	m_tcpip = Config::GetInstance()->GetIp();
	m_tcpport = Config::GetInstance()->GetTcpPort();
}

