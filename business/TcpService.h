/*
 * TcpService.h
 *
 *  Created on: Apr 18, 2016
 *      Author: root
 */

#ifndef TCPSERVICE_H_
#define TCPSERVICE_H_

#include "Service.h"
#include "IEC103Type.h"
#include <iostream>
#include <stdint.h>

using namespace std;




#define CONFIRM_07H  0x07
#define CONFIRM_0BH  0x0B


class TcpService: public Service {
public:
	uint32_t GetSendID();
	uint32_t GetRecvID();
	TcpService();
	virtual ~TcpService();
protected:
	uint32_t m_TcpSendID ;
	uint32_t m_TcpRecvID;
protected:
	//com param
	enum TCP_CMD_SEND
	{
		CMD_CONNECTION_CON,
		CMD_CALL_DATA,
		CMD_TEST1_DATA,
		CMD_TEST2_DATA,
		CMD_GET_GROUPDATA,

		CMD_GET_DATA_LV2,
		CMD_GENERAL_READ_YX_GROUP_VALUE,
		CMD_GENERAL_READ_YC_GROUP_VALUE,
	};
	std::string m_tcpip;
	uint16_t m_tcpport;
	TCP_CMD_SEND m_tcpSendCmd;


	virtual void Start();
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
	 * @Desc: 从配置文件初始化具体类型网络需要的配置
	 * @Return: 无
	 */
	virtual void InitConfig();
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
	//virtual bool ParseFixedData(const char* pData, int32_t dataLen);

	/*
	 * @Desc: 处理接收到的可变长度报文
	 * @Param: pData　解析数据开始地址
	 * @Param: dataLen　解析数据长度
	 * @Return: 解析成功或失败
	 */
	virtual bool ParseVariableData(const char* pData, int32_t dataLen);

	//主动发往客户端指令
protected:
	/*
	 * @Desc: 构造复位通信指令字符串
	 * @Return: 复位通信指令字符串
	 */
	std::vector<char> CmdConnection();
	/*
	 * @Desc: 测试指令
	 * @Return: 测试指令字符串
	 */
	std::vector<char> CmdGetTest1();
	std::vector<char> CmdGetTest2();
	/*
	 * @Desc: 构造总召唤指令字符串
	 * @Return: 总召唤指令字符串
	 */
	std::vector<char> CmdGetAll();
	/*
	 * @Desc:获取发送序号
	 * @Return: 获取发送序号
	 */


//	std::vector<char> CmdResetNum();

	/*
	 * @Desc: 指定控制域生成固定长度报文数据字符串
	 * @Param: code
	 * @Return: 固定长度报文数据字符串
	 */
	//virtual std::vector<char> CmdFixedData(uint8_t code);

	/*
	 * @Desc: 构造召唤一级数据指令字符串
	 * @Return: 召唤一级数据指令字符串
	 */
	//virtual std::vector<char> CmdGetDataLv1();

	/*
	 * @Desc: 构造召唤二级数据指令字符串
	 * @Return: 召唤二级数据指令字符串
	 */
	//virtual std::vector<char> CmdGetDataLv2();

	/*
	 * @Desc: 构造总召唤指令字符串
	 * @Return: 总召唤指令字符串
	 */
	//virtual std::vector<char> CmdGetAll();

	//通用分类服务使用部分
	/*
	 * @Desc: 构造通用分类读命令（读一个组所有条目的值）
	 * @Param: groupNum要读的组号
	 * @Return: 通用分类读命令字符串
	 */
	 std::vector<char> CmdGetGroupData(uint8_t groupNum);

	/*
	 * @Desc: 生成较验码
	 * @Param: buffer 开始地址
	 * @Param: count 开始地址
	 * @Return: 较验码
	 */
	//virtual uint8_t SumCheck( const char* buffer, int32_t count );

	//解析客户端上送的ASDU数据
protected:
	/*
	 * @Desc: 解析ASDU1遥信变位上送
	 * @Param: buffer 开始地址
	 * @Param: count 开始地址
	 */
	virtual void ParseASDU1(const char* buffer, int32_t count);
	/*
	 * @Desc: 解析通用分类数据响应（读目录，读一个组的描述，读一个组的值）
	 * @Param: buffer 开始地址
	 * @Param: count 开始地址
	 */
	virtual void ParseASDU10(const char* buffer, int32_t count);
	/*
	 * @Desc: 解析ASDU42上送全遥信
	 * @Param: buffer 开始地址
	 * @Param: count 开始地址
	 */
	void ParseASDU42(const char* buffer, int32_t count);
	/*
	 * @Desc: 解析ASDU40上送变位遥信
	 * @Param: buffer 开始地址
	 * @Param: count 开始地址
	 */
	 void ParseASDU40(const char* buffer, int32_t count);
	/*
	 * @Desc: 解析ASDU44上送SOE
	 * @Param: buffer 开始地址
	 * @Param: count 开始地址
	 */
	virtual void ParseASDU44(const char* buffer, int32_t count);
	virtual void ParseASDU41(const char* buffer, int32_t count);
	/*
	 * @Desc: 解析ASDU50遥测上送
	 * @Param: buffer 开始地址
	 * @Param: count 开始地址
	 */
	virtual void ParseASDU50(const char* buffer, int32_t count);


	virtual void ParseASDU10AllValue(const char* buffer, int32_t count);
private:
	//virtual void ParseASDU10AllValue(const char* buffer, int32_t count);

};

#endif /* TCPSERVICE_H_ */
