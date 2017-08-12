/*
 * SerialNet.h
 *
 *  Created on: Jun 14, 2015
 *      Author: root
 */

#ifndef SERIALNET_H_
#define SERIALNET_H_

#include <string>
#include "Inet.h"

class SerialNet : public Inet
{
public:
	/*
	 * @Desc: 初始化
	 * @Param: portPath 串口路径
	 * @Param: speed 波特率
	 * @Param: databits 数据位的个数 取值 为 7 或者8
	 * @Param: stopbits 停止位使用格式 取值为 1 或者2
	 * @Param: parity 奇偶较验位 取值为N(清除),E(偶较验),O(奇较验)
	 * @Param: outTime 超时时间 单位秒
	 */
	SerialNet(const std::string & portPath, int32_t speed = 9600, int32_t databits = 8, int32_t stopbits = 1, char parity = 'N', int32_t outTime=1);

	/*
	 * @Desc: 进行通讯连接
	 * @Return: 连接成功或失败
	 */
	virtual bool Connect();

	/*
	 * @Desc: 读取数据
	 * @Param: pRecvData 读取数据存放地址
	 * @Param: dataMaxLen 读取数据的最大长度
	 * @Return: 读取的数据长度
	 */
	virtual int32_t Read(char * pRecvData, uint32_t dataMaxLen);

	/*
	 * @Desc: 发送数据
	 * @Param: pSendData 发送数据存放地址
	 * @Param: dataLen 发送的数据的长度
	 * @Return: 发送的数据长度
	 */
	virtual int32_t Write(const char * pSendData, uint32_t dataLen);

	/*
	 * @Desc: 断开通讯连接
	 * @Return: 成功或失败
	 */
	virtual bool DisConnect();

	/*
	 * @Desc: 生成较验码
	 * @Param: buffer 开始地址
	 * @Param: count 开始地址
	 * @Return: 较验码
	 */
	uint8_t SumCheck( const char* buffer, int32_t count );

	std::string ToHexString(const char* data, int32_t dataLen);

	virtual ~SerialNet();
	typedef void * Handle;
private:

	/*
	 * @Desc:设置波特率
	 * @Return: 设置成功或失败
	 */
	bool SetSpeed();

	/*
	 * @Desc:设置数据位、停止位和校验位
	 * @Return: 设置成功或失败
	 */
	bool SetParity();
private:
	bool GetFunction(Handle & handle, const char* funcName);
private:
	//初始化
	bool (*Pfun_InitSerial)(const char* m_Name,int m_badb,short m_dataSize,short m_parity_,short m_stop_bit,short m_buf_size,short m_timeOut,short m_read_Mode);
	//连接
	bool (*Pfun_Connect)();
	//断开
	bool (*Pfun_Broken)();
	//接收
	int (*Pfun_Recv)(unsigned char *pData,int len);
	//发送数据
	int (*Pfun_Send)(unsigned char *pData,int len);
	//卸载
	bool (*Pfun_UnInitSerial)();


private:
	std::string m_SerialPath;
	int32_t m_speed;
	int32_t m_databits; //数据位的个数
	int32_t m_stopbits; //停止位使用格式
	uint16_t 	m_parity;  //奇偶较验位
	int32_t m_outTime; //超时时间
	void *m_handle;

};


#endif /* SERIALNET_H_ */
