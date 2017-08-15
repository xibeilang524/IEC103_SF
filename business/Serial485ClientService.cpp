/*
 * Serial485ClientService.cpp
 *
 *  Created on: Jul 14, 2015
 *      Author: root
 */

#include "Serial485ClientService.h"
#include <iostream>
#include <unistd.h>
#include <string.h>

#include "../core/SerialNet.h"
#include "../core/Logger.h"
#include "../core/WarningLib.h"

Serial485ClientService::Serial485ClientService():SerialService()
{

}

Serial485ClientService::~Serial485ClientService()
{
}

void Serial485ClientService::Start()
{
	m_net = new SerialNet(m_portPath, m_speed, m_databits, m_stopbits, m_parity, m_outTime);

	//连接
	if(!m_net->Connect())
	{
		ERROR( "serial port connect failed \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "serial port connect failed");
		return;
	}

	while(true)
	{
		 usleep(50*1000);
		 if(!Read())
		 {
			 break;
		 }
	}

}


bool Serial485ClientService::Read()
{
	memset(m_datapacket, 0, sizeof(m_datapacket));
	static uint16_t reSendTimes = 0; //同一指令从发次数（不包括复位通信）
	int readSize = m_net->Read(m_datapacket, sizeof(m_datapacket));
	if(-1 == readSize)
	{
		return false;
	}
	else if(-2 == readSize) //读到无效的数据重发
	{
	}
	else if(0 == readSize)//未读到数据
	{
	}
	else
	{
		reSendTimes = 0;
		DEBUG("read[%s]:%s \n", g_cmdName[m_nextCmd], ToHexString(m_datapacket, readSize).c_str());
		if(START_68H == m_datapacket[0])
		DEBUG("{code=%d, ASDU=%d, FUN=%d, INF=%d} \n", (uint8_t)m_datapacket[4], (uint8_t)m_datapacket[6], (uint8_t)m_datapacket[10], (uint8_t)m_datapacket[11]);
		//TODO:测试时使用
		printf("read[%s]: ",g_cmdName[m_nextCmd]);
		printf("%s \n", ToHexString(m_datapacket, readSize).c_str());
		if(START_68H == m_datapacket[0])
		printf("{code=%d, ASDU=%d, FUN=%d, INF=%d} \n", (uint8_t)m_datapacket[4], (uint8_t)m_datapacket[6], (uint8_t)m_datapacket[10], (uint8_t)m_datapacket[11]);
		if(!ParseRecvData(m_datapacket, readSize))
		{
			WARN("recv invalied data: %s\n", ToHexString(m_datapacket, readSize).c_str());
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV0, "recv invalied data");
		}
		return true;

	}

	return true;
}

