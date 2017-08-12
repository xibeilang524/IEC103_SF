/*
 * SerialService.h
 *
 *  Created on: Jun 15, 2015
 *      Author: root
 */

#ifndef SERIALSERVICE_H_
#define SERIALSERVICE_H_

#include <string>

#include "IEC103Type.h"
#include "Service.h"

#define MAX_SIZE 1024

class SerialNet;
class SerialService : public Service
{
public:
	SerialService();
	virtual ~SerialService();
	virtual void Start();

protected:
	/*
	 * @Desc: 读取从站上送的数据
	 * @Return:　成功或失败
	 */
	virtual bool Read();

	/*
	 * @Desc: 主站发送指令
	 * @Return:　成功或失败
	 */
	virtual bool Write();

	/*
	 * @Desc: 解析接收到的数据
	 * @Param: pData　解析数据开始地址
	 * @Param: dataLen　解析数据长度
	 * @Return: 解析成功或失败
	 */
	virtual bool ParseRecvData(const char* pData, int32_t dataLen);

	/*
	 * @Desc: 处理接收到的固定长度报文
	 * @Param: pData　解析数据开始地址
	 * @Param: dataLen　解析数据长度
	 * @Return: 解析成功或失败
	 */
	virtual bool ParseFixedData(const char* pData, int32_t dataLen);

	/*
	 * @Desc: 处理接收到的可变长度报文
	 * @Param: pData　解析数据开始地址
	 * @Param: dataLen　解析数据长度
	 * @Return: 解析成功或失败
	 */
	virtual bool ParseVariableData(const char* pData, int32_t dataLen);

	/*
	 * @Desc: 从配置文件初始化具体类型网络需要的配置
	 * @Return: 无
	 */
	virtual void InitConfig();
protected:
	//串口参数从配置文件读取
	std::string  m_portPath;
	int32_t m_speed;
	int32_t m_databits;
	int32_t m_stopbits;
	uint16_t    m_parity;
	int32_t m_outTime;

	//com param
	std::string m_tcpip;
	uint16_t m_tcpport;

};

#endif /* SERIALSERVICE_H_ */
