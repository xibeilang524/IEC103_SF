/*
 * TcpNet.cpp
 *
 *  Created on: Jun 14, 2015
 *      Author: root
 */

#include "TcpNet.h"

#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "WarningLib.h"
#include "Logger.h"


TcpNet::TcpNet(const std::string & serverAddr, uint16_t serverPort):Inet()
{
	m_serverAddrStr = serverAddr;

	m_serverAddr.sin_family = AF_INET;
	m_serverAddr.sin_port = htons(serverPort);
	m_serverAddr.sin_addr.s_addr = inet_addr(serverAddr.c_str());
}

TcpNet::~TcpNet()
{

}

/*
 * @Desc: 进行通讯连接
 * @Return: 连接成功或失败
 */
bool TcpNet::Connect()
{

	//创建socket
	if((m_netFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		ERROR(":%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "socket failed");
		return false;
	}

	//tcp连接服务端
	if(connect(m_netFd, (struct sockaddr *)&m_serverAddr, sizeof(m_serverAddr)) < 0)
	{
		ERROR("tcp connect failed:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "tcp connect failed");
		return false;
	}
	int flags=fcntl(m_netFd,F_GETFL,0);        //获取建立的sockfd的当前状态（非阻塞）
	fcntl(m_netFd,F_SETFL,flags|O_NONBLOCK);   //将当前sockfd设置为非阻塞
	return true;
}

/*
 * @Desc: 读取数据
 * @Param: 读取数据存放地址
 * @Param: 读取数据的最大长度
 * @Return: 读取的数据长度
 */
int32_t TcpNet::Read(char * pRecvData, uint32_t dataMaxLen)
{
	int32_t readLen = 0;
	int32_t readAllLen = 0;

	struct timeval tv; /* 声明一个时间变量来保存时间 */
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	fd_set watchset;
	FD_ZERO(&watchset);
	int32_t rcd = select(m_netFd + 1, &watchset, NULL, NULL, &tv);

	while((readLen = recv(m_netFd, pRecvData, dataMaxLen - readAllLen, 0)) > 0)
	{
		if(-1 == readLen)
		{
			ERROR("read failed:%s \n", strerror(errno));
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "tcp read failed");
			return -1;
		}
		readAllLen += readLen;
	}
	return readAllLen;
}

/*
 * @Desc: 发送数据
 * @Param: 发送数据存放地址
 * @Param: 发送的数据的长度
 * @Return: 发送的数据长度
 */
int32_t TcpNet::Write(const char * pSendData, uint32_t dataLen)
{
	int32_t writeLen = 0;
	if(-1 == (writeLen = send(m_netFd, pSendData, dataLen, 0)))
	{
		ERROR("write failed:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "tcp write failed");
		DisConnect();
		Connect();
		return -1;
	}
	return writeLen;
}

/*
 * @Desc: 断开通讯连接
 * @Return: 成功或失败
 */
bool TcpNet::DisConnect()
{
	close(m_netFd);
	m_netFd = -1;
	return true;
}
