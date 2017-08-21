/*
 * Service.h
 *
 *  Created on: Jun 15, 2015
 *      Author: root
 */

#ifndef SERVICE_H_
#define SERVICE_H_

#include <string>
#include <vector>
#include <map>
#include <sys/time.h>

#include "IEC103Type.h"
#include "../core/Inet.h"
#include "../core/Config.h"
#include "../core/TcpNet.h"

extern const char * g_cmdName[];
extern const char * g_TcpCmdName[];
class Service {
public:
	Service();
	virtual ~Service();

	bool Init();

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
	virtual bool ParseFixedData(const char* pData, int32_t dataLen);

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
	 std::vector<char> CmdResetCon();

	/*
	 * @Desc: 构造复位帧计数位字符串
	 * @Return: 复位通信指令字符串
	 */
	 std::vector<char> CmdResetNum();

	/*
	 * @Desc: 指定控制域生成固定长度报文数据字符串
	 * @Param: code
	 * @Return: 固定长度报文数据字符串
	 */
	 std::vector<char> CmdFixedData(uint8_t code);


	/*
	 * @Desc: 构造召唤一级数据指令字符串
	 * @Return: 召唤一级数据指令字符串
	 */
	 std::vector<char> CmdGetDataLv1();

	/*
	 * @Desc: 构造召唤二级数据指令字符串
	 * @Return: 召唤二级数据指令字符串
	 */
	 std::vector<char> CmdGetDataLv2();

	/*
	 * @Desc: 对时
	 * @Return:
	 */
	 std::vector<char> CmdSetTimeStamp();

	/*
	 * @Desc: 事件告警复归
	 * @Return:
	 */
	 std::vector<char> CmdResetEventAlarm();

	/*
	 * @Desc: 事件告警复归
	 * @Return:
	 */
	 std::vector<char> CmdResetEventAlarm();

	/*
	 * @Desc: 定值下传
	 * @Return:
	 */
	 std::vector<char> CmdSettingDownload();

	/*
	 * @Desc: 定值修改
	 * @Return:
	 */
	 std::vector<char> CmdSettingModify();

	/*
	 * @Desc: 构造总召唤指令字符串
	 * @Return: 总召唤指令字符串
	 */
	 std::vector<char> CmdGetAll();

//通用分类服务使用部分
	/*
	 * @Desc: 构造通用分类读命令（读一个组所有条目的值）
	 * @Param: groupNum要读的组号
	 * @Return: 通用分类读命令字符串
	 */
	 std::vector<char> CmdGetGroupValue(uint8_t groupNo);

	/*
	 * @Desc: 构造通用分类读命令（读一个组所有条目的值）
	 * @Param: groupNum要读的组号
	 * @Return: 通用分类读命令字符串
	 */
	 std::vector<char> CmdGetEntryValue(uint8_t groupNo, uint8_t entryNo);

	/*
	 * @Desc: 生成较验码
	 * @Param: packet 开始地址
	 * @Param: length 开始地址
	 * @Return: 较验码
	 */
	 uint8_t SumCheck( const char* packet, int32_t length );

//解析客户端上送的ASDU数据
protected:
	/*
	 * @Desc: 解析ASDU44上送全遥信
	 * @Param: packet 开始地址
	 * @Param: length 开始地址
	 */
	 virtual void ParseASDU44(const char* packet, int32_t length);

	/*
	 * @Desc: 解析ASDU40上送变位遥信
	 * @Param: packet 开始地址
	 * @Param: length 开始地址
	 */
	virtual void ParseASDU40(const char* packet, int32_t length);

	/*
	 * @Desc: 解析ASDU44上送SOE
	 * @Param: packet 开始地址
	 * @Param: length 开始地址
	 */
	virtual void ParseASDU41(const char* packet, int32_t length);


	/*
	 * @Desc: 解析ASDU50遥测上送
	 * @Param: packet 开始地址
	 * @Param: length 开始地址
	 */
	virtual void ParseASDU50(const char* packet, int32_t length);



	//遥信传输过程：主站以ASDU7发总查询命令
	//           从站以ASDU42上送全遥信，以ASDU1上送变位遥信，以ASDU2上送事故报文
	/*
	 * @Desc: 解析ASDU42上送全遥信
	 * @Param: packet 开始地址
	 * @Param: length 开始地址
	 */
	virtual void ParseASDU42(const char* packet, int32_t length);

	/*
	 * @Desc: 解析ASDU1遥信变位上送
	 * @Param: packet 开始地址
	 * @Param: length 开始地址
	 */
	virtual void ParseASDU1(const char* packet, int32_t length);

	/*
	 * @Desc: 解析ASDU2事故报文上送
	 * @Param: packet 开始地址
	 * @Param: length 开始地址
	 */
	virtual void ParseASDU2(const char* packet, int32_t length);


	//遥测传输过程：主站以ASDU21发总召唤命令
	//           从站以ASDU10上送全遥测
	/*
	 * @Desc: 解析通用分类数据响应（读目录，读一个组的描述，读一个组的值）
	 * @Param: packet 开始地址
	 * @Param: length 开始地址
	 */
	virtual void ParseASDU10(const char* packet, int32_t length);
private:
	virtual void ParseASDU10AllValue(const char* packet, int32_t length);

protected:
	/*
	 * @Desc: 通过I03点地址获取对应的合局地址
	 * @Param: privateAddr 103点地址
	 * @Return: 全局地址
	 */
	std::string GetGlobalAddrByPrivateAddr(std::string privateAddr);

	float GetRateByPrivateAddr(std::string privateAddr);

	std::string ToHexString(const char* data, int32_t dataLen);
private:
	/*
	 * @Desc: 初始化103点表
	 * @Return: 成功或失败
	 */
	bool InitPointAddrMap();
	/*
	 * @Desc: 开始监听心跳，每一段时间往数据库更新一次
	 * @Return: 成功或失败
	 */
	bool StartBreakHeart();
    static void * ThreadFunc(void *arg);
<<<<<<< HEAD
protected:
	enum CMD_SEND
	{
		CMD_RESET_CON,
		CMD_RESET_NUM,
		CMD_RESET_TIMESTAMP,
		CMD_RESET_EVENT,
		CMD_GET_ALL,
		CMD_GET_DATA_LV1,
		CMD_GET_DATA_LV2,
		CMD_GENERAL_READ_YX_GROUP_VALUE,
		CMD_GENERAL_READ_YC_GROUP_VALUE,
	};
=======

>>>>>>> d5145ec17abff158ad4ef64531721a60fadcab11
protected:
	struct Point103
	{
		float       rate;
		std::string pubAddr;
	};

protected:
	Inet        * m_net;
	IEC103CodeS2C m_iec103CodeS2C; //103控制域主到从
	CMD_SEND      m_nextCmd;       //下一条主站应该发送的指令

	uint8_t       m_clientAddr;  //从站地址
	static uint16_t      m_heartInterval; //秒

	std::map<std::string, Point103> m_pointAddrMap;  //<103点地址， 61850地址> 103地址和全局地址映射
	uint16_t      m_maxReSendTimes;       //重发次数
	uint16_t      m_sendInterval;         //重新发送间隔
	char          m_datapacket[MAX_SIZE]; //读到的数据

	bool          m_isResetConEnd;  //复位通信
	bool          m_isResetNumEnd;  //复位帧计数位
	bool          m_isGetAllEnd;    //总召唤

	uint8_t       m_ycGroupNum;
	uint8_t       m_yxGroupNum;

	uint8_t       m_GroupNo;
	uint8_t		  m_EntryNo;
	uint8_t       m_timeStampAddr;
};

#endif /* SERVICE_H_ */
