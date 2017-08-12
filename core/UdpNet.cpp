/*
 * UdpNet.cpp
 *
 *  Created on: Jun 14, 2015
 *      Author: root
 */

#include "UdpNet.h"

#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Logger.h"
#include "WarningLib.h"


UdpNet::UdpNet(const std::string & serverAddr, uint16_t serverPort, uint16_t localPort):Inet()
{
	m_serverAddrStr = serverAddr;

	m_serverAddr.sin_family = AF_INET;
	m_serverAddr.sin_port = htons(serverPort);
	m_serverAddr.sin_addr.s_addr = inet_addr(serverAddr.c_str());

	m_localAddr.sin_family = AF_INET;
	m_localAddr.sin_port = htons(localPort);
	m_localAddr.sin_addr.s_addr = INADDR_ANY;
}

UdpNet::~UdpNet()
{

}

/*
 * @Desc: 进行通讯连接
 * @Return: 连接成功或失败
 */
bool UdpNet::Connect()
{

	//创建socket
	if((m_netFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		ERROR("socket failed:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "udp socket failed");
		return false;
	}
	//绑定端口
	if(-1 == bind(AF_INET, (sockaddr *)&m_localAddr, sizeof(m_localAddr)))
	{
		ERROR("bind failed:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "udp bind failed");
		return false;
	}

	return true;
}

/*
 * @Desc: 读取数据
 * @Param: 读取数据存放地址
 * @Param: 读取数据的最大长度
 * @Return: 读取的数据长度
 */
int32_t UdpNet::Read(char * pRecvData, uint32_t dataMaxLen)
{
	sockaddr_in serverAddr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int32_t readLen = 0;
	readLen = recvfrom(m_netFd,pRecvData,dataMaxLen,0,(struct sockaddr*)&serverAddr,&addr_len);
	if(-1 == readLen)
	{
		ERROR("read failed:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "udp read failed");
	}

	return readLen;
}

/*
 * @Desc: 发送数据
 * @Param: 发送数据存放地址
 * @Param: 发送的数据的长度
 * @Return: 发送的数据长度
 */
int32_t UdpNet::Write(const char * pSendData, uint32_t dataLen)
{
	int32_t writeLen = 0;
	if(-1 == (writeLen = sendto(m_netFd, pSendData, dataLen, 0, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr))))
	{
		ERROR("write failed:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "udp write failed");
		return -1;
	}
	return writeLen;
}

/*
 * @Desc: 断开通讯连接
 * @Return: 成功或失败
 */
bool UdpNet::DisConnect()
{
	close(m_netFd);
	m_netFd = -1;
	return true;
}
