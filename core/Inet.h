/*
 * Inet.h
 * Des:通讯接口类
 *  Created on: Jun 14, 2015
 *      Author: root
 */

#ifndef INET_H_
#define INET_H_

#include <stdint.h>

class Inet {
public:
	Inet();
	/*
	 * @Desc: 进行通讯连接
	 * @Return: 连接成功或失败
	 */
	virtual bool    Connect() = 0;
	/*
	 * @Desc: 读取数据
	 * @Param: 读取数据存放地址
	 * @Param: 读取数据的最大长度
	 * @Return: 读取的数据长度
	 */
	virtual int32_t Read(char * pRecvData, uint32_t dataMaxLen) = 0;
	/*
	 * @Desc: 发送数据
	 * @Param: 发送数据存放地址
	 * @Param: 发送的数据的长度
	 * @Return: 发送的数据长度
	 */
	virtual int32_t Write(const char  * pSendData, uint32_t dataLen) = 0;
	/*
	 * @Desc: 断开通讯连接
	 * @Return: 成功或失败
	 */
	virtual bool    DisConnect() = 0;
	virtual ~Inet();
protected:
	int32_t m_netFd;
};

#endif /* INET_H_ */
