/*
 * Service.cpp
 *
 *  Created on: Jun 15, 2015
 *      Author: root
 */

#include "Service.h"
#include <string.h>
#include <iostream>
#include <pthread.h>

#include "../core/Logger.h"
#include "../core/MemCache.h"
#include "../core/EncapMysql.h"
#include "../core/WarningLib.h"


const char * g_cmdName[]= {
		"CMD_RESET_CON",
		"CMD_RESET_NUM",
		"CMD_GET_ALL",
		"CMD_GET_DATA_LV1",
		"CMD_GET_DATA_LV2",
		"CMD_GENERAL_READ_YX_GROUP_VALUE",
		"CMD_GENERAL_READ_YC_GROUP_VALUE"};

const char * g_TcpCmdName[]= {
		"CMD_CONNECTION_CON",
		"CMD_CALL_DATA",
		"CMD_TEST1_DATA",
		"CMD_TEST2_DATA",
		"CMD_GET_GROUPDATA",

		"CMD_GET_DATA_LV1",
		"CMD_GET_GROUPDATA",
		"CMD_GENERAL_READ_YX_GROUP_VALUE",
		"CMD_GENERAL_READ_YC_GROUP_VALUE"};
uint16_t Service::m_heartInterval=60;

Service::Service():m_net(NULL)
{
	memset(m_dataBuffer, 0, sizeof(m_dataBuffer));
	m_maxReSendTimes = 3;
	m_sendInterval = 3;

	m_iec103CodeS2C.fcv = 0;
	m_iec103CodeS2C.fcb = 0;

	m_isResetConEnd = false;  //复位通信
	m_isResetNumEnd = false;  //复位帧计数位
	m_isGetAllEnd = false;    //总召唤

	m_ycGroupNum = 0x09;
	m_yxGroupNum = 0x00;

	m_nextCmd = CMD_GENERAL_READ_YX_GROUP_VALUE;
}

Service::~Service()
{
	if(NULL != m_net)
	{
		delete m_net;
	}
}

void Service::InitConfig()
{

}

bool Service::Init()
{
	//初始化配置参数
	InitConfig();

//	m_heartInterval = Config::GetInstance()->GetHeartInterval();
//
//	//初始化103点表
//	if(!InitPointAddrMap())
//	{
//		return false;
//	}
//
//	//定时更新心跳
//	if(!StartBreakHeart())
//	{
//		return false;
//	}

	return true;
}

void Service::Start()
{
	m_net = new TcpNet(Config::GetInstance()->GetIp(),Config::GetInstance()->GetTcpPort());

	std::cout<<"Connection TYPE：103 TcpNet，ipAddr＝"<< Config::GetInstance()->GetIp()<<" Port = "<<Config::GetInstance()->GetTcpPort()<<std::endl;

	if(!m_net->Connect())
	{
		ERROR( "tcp connection error  \n");
		return;
	}

	while(true)
	{
		 if(!Write())
		 {
			 break;
		 }
		 if(!Read())
		 {
			 break;
		 }
	}
}

bool Service::Write()
{
	std::vector<char> strCmd;
	std::string cmdName("");
	//首先连接，再启动总召换
	switch(m_nextCmd)
	{
	case CMD_RESET_CON:
		strCmd = CmdResetCon();
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
		strCmd = CmdGetGroupValue(m_yxGroupNum);
		break;
	case CMD_GENERAL_READ_YC_GROUP_VALUE:
		strCmd = CmdGetGroupValue(m_ycGroupNum);
		break;
	default:
		ERROR("cmd not defined \n" );
		return false;
	}
	DEBUG("Write[%s]:%s \n" ,g_cmdName[m_nextCmd], ToHexString(strCmd.data(), strCmd.size()).c_str());
	//TODO:测试时使用
	printf("write[%s]: ", g_cmdName[m_nextCmd]);
	printf("%s \n", ToHexString(strCmd.data(), strCmd.size()).c_str());

	//如果发送失败，就认为是已经断开连接了
	if(-1 == m_net->Write(strCmd.data(), strCmd.size()))
	{
		ERROR("send cmd %d error\n", m_nextCmd);
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "send data error");
		m_nextCmd = CMD_RESET_CON;
		//return false;
	}
	return true;
}

bool Service::Read()
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

		if(CMD_RESET_CON == m_nextCmd) //如果上一次指令是通信复位不做重发计数
		{
			return true;
		}
	}
	else
	{
		reSendTimes = 0;
		DEBUG("read[%s] size %d :%s \n", g_cmdName[m_nextCmd], readSize,ToHexString(m_dataBuffer, readSize).c_str());
//		if(START_68H == m_dataBuffer[0])
//			DEBUG("{code=%d, ASDU=%d, FUN=%d, INF=%d} \n",
//					(uint8_t)m_dataBuffer[4],
//					(uint8_t)m_dataBuffer[6],
//					(uint8_t)m_dataBuffer[10],
//					(uint8_t)m_dataBuffer[11]);
		//TODO:测试时使用
		printf("size: %d  read[%s] : ",readSize,g_cmdName[m_nextCmd]);
		printf("%s \n", ToHexString(m_dataBuffer, readSize).c_str());

		if(!ParseRecvData(m_dataBuffer, readSize))
		{
			WARN("recv invalied data: %s\n", ToHexString(m_dataBuffer, readSize).c_str());
		}
		return true;
	}

	//重发计数超过最大重发次数则进行复位通信
	if(++reSendTimes >= m_maxReSendTimes)
	{
		reSendTimes = 0;
	//	m_tcpSendCmd = CMD_CALL_DATA;
		m_isResetConEnd = false;
		m_isResetNumEnd = false;
		WARN("cmd send thress times , cmd resetcon send \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV1, "Link disconnect , cmd resetcon send");
	}

	return true;
}

/*
 * @Desc: 解析接收到的数据
 * @Param: pData　解析数据开始地址
 * @Param: dataLen　解析数据长度
 * @Return: 无
 */
bool Service::ParseRecvData(const char* pData, int32_t dataLen)
{
    uint8_t sumcheck = 0;
	if(START_10H == pData[0] && 5 == dataLen) //固定长度报文(固定五个字节)
	{
		sumcheck = SumCheck( pData+1, 2);     //较验数据
		if (sumcheck == (uint8_t)pData[3])
		{
			ParseFixedData(pData, dataLen);   // 处理定长数据
		}
		else        //数据错误
		{
			return false;
		}
	}
	else if(START_68H == pData[0] && dataLen >= 14) //可变长报文(最少14个字节)
	{
		sumcheck = SumCheck( pData+4, (uint8_t)pData[1] );     	//较验数据
		if (sumcheck == (uint8_t)pData[(uint8_t)pData[1]+4])
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
	}
	else
	{
		// 不应该出现此种情况
		ERROR("received invalid data \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV0, "received invalid data");
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
bool Service::ParseFixedData(const char* pData, int32_t dataLen)
{
	int acd = pData[2] & ACD_1;
	if(acd)
	{
		m_iec103CodeS2C.fcb = m_iec103CodeS2C.fcb ^ FCB_1;
		m_iec103CodeS2C.fcv = FCV_1;
		m_nextCmd = CMD_GET_DATA_LV1;
	}

	return true;
}

/*
 * @Desc: 处理接收到的可变长度报文
 * @Param: pData　解析数据开始地址
 * @Param: dataLen　解析数据长度
 * @Return: 解析成功或失败
 */
bool Service::ParseVariableData(const char* pData, int32_t dataLen)
{
	uint8_t asduType = pData[TYP_POSI]; // asdu类型

	switch(asduType)
	{
		case ASDU_1:
			ParseASDU1(pData, dataLen);
			break;
		case ASDU_2:
			ParseASDU2(pData, dataLen);
			break;
		case ASDU_10:
			ParseASDU10(pData, dataLen);
			break;
		case ASDU_40:
			ParseASDU40(pData, dataLen);
			break;
		case ASDU_41:
			ParseASDU41(pData, dataLen);
			break;
		case ASDU_42:
			ParseASDU42(pData, dataLen);
			break;
		case ASDU_44:
			ParseASDU44(pData, dataLen);
			break;
		case ASDU_50:
			ParseASDU50(pData, dataLen);
			break;
		default:
			break;
	}
	return true;
}

// 复位通信单元
std::vector<char> Service::CmdResetCon()
{
	m_iec103CodeS2C.fcb = 0; //复位时将桢记数位置0
	m_iec103CodeS2C.fcv = 0;
	uint8_t code = PRA_1|0|0|FNA_S2C_RESET_CON;
	return CmdFixedData(code);
}

/*
 * @Desc: 构造复位帧计数位字符串
 * @Return: 复位通信指令字符串
 */
std::vector<char> Service::CmdResetNum()
{
	m_iec103CodeS2C.fcb = 0; //复位时将桢记数位置0
	m_iec103CodeS2C.fcv = 0;
	uint8_t code = PRA_1|0|0|FNA_S2C_RESET_NUM;
	return CmdFixedData(code);
}

// 召唤一级数据
std::vector<char> Service::CmdGetDataLv1()
{
	uint8_t code = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_GETDATA_LV1;
	return CmdFixedData(code);
}

// 召唤二级数据
std::vector<char> Service::CmdGetDataLv2()
{
	uint8_t code = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_GETDATA_LV2;
	return CmdFixedData(code);
}

// 固定长度报文
std::vector<char> Service::CmdFixedData(uint8_t code)
{
	std::vector<char> buffer(5);
	buffer[0] = START_10H;
	buffer[1] = code;
	buffer[2] = m_clientAddr;
	buffer[3] = buffer[1]+buffer[2];
	buffer[4] = END_16H;
	return buffer;
}

// 总召唤
std::vector<char> Service::CmdGetAll()
{
	std::vector<char> buffer(15);
	static uint8_t scn = 1;
	if(255 == scn)
	{
		scn = 1;
	}
	buffer[0] = START_68H;
	buffer[1] = 0x09;
	buffer[2] = 0x09;
	buffer[3] = START_68H;
	buffer[4] = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_POSTDATA;
	buffer[5] = m_clientAddr;
	buffer[6] = ASDU7_GETALL;
	buffer[7] = 0x81;  //vsq
	buffer[8] = COT_S2C_GETALL_START;
	buffer[9] = m_clientAddr;
	buffer[10] = FUN_GETALL;  // FUN
	buffer[11] = 0x00;
	buffer[12] = scn++;
	buffer[13] = SumCheck( &(buffer[4]), (uint8_t)buffer[1] );
	buffer[14] = END_16H;
	return buffer;
}

/*
 * @Desc: 构造通用分类读命令（读一个组所有条目的值）
 * @Param: groupNum要读的组号
 * @Return: 通用分类读命令字符串
 */
std::vector<char> Service::CmdGetGroupValue(uint8_t groupNum)
{
	std::vector<char> buffer(19);
	buffer[0] = START_68H;
	buffer[1] = 0x0D;
	buffer[2] = 0x0D;
	buffer[3] = START_68H;
	buffer[4] = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_POSTDATA;
	buffer[5] = m_clientAddr;
	buffer[6] = ASDU21_GETGROUP;
	buffer[7] = 0x81;  //vsq
	buffer[8] = COT_S2C_GENERAL_READ;
	buffer[9] = m_clientAddr;
	buffer[10] = 0xFE;  // FUN
	buffer[11] = 0xF1;
	buffer[12] = 0x15;
	buffer[13] = 0x01;
	buffer[14] = groupNum;
	buffer[15] = 0;
	buffer[16] = 1;
	buffer[17] = SumCheck( &(buffer[4]), (uint8_t)buffer[1] );
	buffer[18] = END_16H;
	return buffer;
}

uint8_t Service::SumCheck( const char* buffer, int32_t count )
{
	uint8_t result = 0;
	for( int32_t i = 0; i < count; i++ )
	{
		result += buffer[i];
	}
	return result;
}

/*
 * @Desc: 解析ASDU44上送全遥信
 * @Param: buffer 开始地址
 * @Param: count 开始地址
 */
void Service::ParseASDU44(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[5];
	uint8_t vsq = buffer[7];
	uint8_t fun = buffer[10];
	uint8_t inf = buffer[11];
	uint8_t scdNum = vsq & 0x7F; // 信息元个数
    bool bValue = false;
    char addr[100] = {0}; //103地址
    std::vector<Tag> vecTag;
    for(int i = 0; i < scdNum; ++i)
    {
    	memset(addr, 0, sizeof(addr));
    	//信息元
		uint8_t pScd = buffer[12 + i*5];
		uint8_t qds = buffer[16 + i*5]; //品质描述字节
		if(qds & 0x80) //当前值无效
		{
			++inf;
			continue;
		}
		if(0x01 & pScd)
		{
			bValue = true;  //true为合，false为分
		}
		sprintf(addr, "%d_%d%03d", clientAddr, fun, inf); //构造103地址
		Tag tagTemp;
		tagTemp.ZeroTag();
		tagTemp.dataType = TYPE_BOOL;
		std::string strGlgAddr = GetGlobalAddrByPrivateAddr(addr);//公共点地址
		if(!strGlgAddr.empty())
		{
			uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
			memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
			sprintf(tagTemp.pdata, "%d", bValue);
			vecTag.push_back(tagTemp);
		}
		++inf;

    }
    if(!vecTag.empty())
    {
    	MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库
    }

}

/*
 * @Desc: 解析ASDU40上送变位遥信
 * @Param: buffer 开始地址
 * @Param: count 开始地址
 */
void Service::ParseASDU40(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[5];
	uint8_t fun = buffer[10];
	uint8_t inf = buffer[11];
	uint8_t siq = buffer[12]; //带品质描述的单点信息字节
	bool bValue = false;
	char addr[100] = {0}; //103地址
	std::vector<Tag> vecTag;
	if(siq & 0x80) //无效
	{
		return;
	}
	if(0x01 & siq)
	{
		bValue = true;  //true为合，false为分
	}

	sprintf(addr, "%d_%d%03d", clientAddr, fun, inf); //构造103地址
	Tag tagTemp;
	tagTemp.ZeroTag();
	tagTemp.dataType = TYPE_BOOL;
	std::string strGlgAddr = GetGlobalAddrByPrivateAddr(addr);//公共点地址
	if(!strGlgAddr.empty())
	{
		uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
		memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
		sprintf(tagTemp.pdata, "%d", bValue);
		vecTag.push_back(tagTemp);
	}
	if(!vecTag.empty())
	{
		MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库
	}
}

/*
 * @Desc: 解析ASDU44上送SOE
 * @Param: buffer 开始地址
 * @Param: count 开始地址
 * @Return: 较验码
 */
void Service::ParseASDU41(const char* buffer, int32_t count)
{
	ParseASDU40(buffer, count);
}

void Service::ParseASDU42(const char* buffer, int32_t count)
{
	uint8_t vsq = buffer[VSQ_POSI];
	uint8_t clientAddr = buffer[ADDR_POSI];
	uint8_t fun = buffer[FUN_POSI];;
	uint8_t inf = buffer[INF_POSI];
	uint8_t scdNum = vsq & 0x7F; 	// 信息元个数
    bool bValue = true;
    char addr[100] = {0}; 			 //103地址
    std::vector<Tag> vecTag;
    for(int i = 0; i < scdNum; ++i)
    {
    	memset(addr, 0, sizeof(addr));
    	//信息元 diq
		uint8_t diq = buffer[DPI_POSI + i]; //品质描述的双点信息

		if(diq & 0xfc) //当前值无效  diq为2位数组，值=0/3为无意义，值=1为分，值=2为合
		{
			continue;
		}

		if(0x01 & diq)
		{
			bValue = false;  //true为合，false为分
		}

		sprintf(addr, "%d_%d%03d", clientAddr, fun, inf); //构造103地址
		Tag tagTemp;
		tagTemp.ZeroTag();
		tagTemp.dataType = TYPE_BOOL;
		std::string strGlgAddr = GetGlobalAddrByPrivateAddr(addr);//公共点地址
		if(!strGlgAddr.empty())
		{
			uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
			memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
			sprintf(tagTemp.pdata, "%d", bValue);
			vecTag.push_back(tagTemp);
		}
    }
    if(!vecTag.empty())
    {
    	MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库
    }
}

/*
 * @Desc: 解析ASDU50遥测上送
 * @Param: buffer 开始地址
 * @Param: count 开始地址
 */
void Service::ParseASDU50(const char* buffer, int32_t count)
{
	uint8_t vsq = buffer[VSQ_POSI];
	uint8_t clientAddr = buffer[ADDR_POSI];
	uint8_t fun = buffer[FUN_POSI];
	uint8_t inf = buffer[INF_POSI];
	uint8_t scdNum = vsq & 0x7F; // 信息元个数
	float value = 0.0f;
	char addr[100] = {0}; //103地址
	std::vector<Tag> vecTag;
	for(int i = 0; i < scdNum; ++i)
	{
		memset(addr, 0, sizeof(addr));
		//信息元
		uint8_t qds = buffer[12 + i*2];
		uint16_t tmpValue = buffer[12 + i*2];
		if(qds & 0x02) //当前值无效
		{
			++inf;
			continue;
		}
		sprintf(addr, "%d_%d%03d", clientAddr, fun, inf); //构造103地址

		bool flag = (tmpValue & 0x8000) ? true: false; //是否为负数
		tmpValue = ~tmpValue;
		tmpValue &= 0xfff8; //品质位置0
		tmpValue &= 0x7ff8; //符号位置0
		tmpValue >>= 3;
		tmpValue += 1;
		value = (flag?(-tmpValue):tmpValue)*GetRateByPrivateAddr(addr);

		Tag tagTemp;
		tagTemp.ZeroTag();
		tagTemp.dataType = TYPE_FLOAT;
		std::string strGlgAddr = GetGlobalAddrByPrivateAddr(addr);//公共点地址
		if(!strGlgAddr.empty())
		{
			uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
			memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
			sprintf(tagTemp.pdata, "%.4f", value);
			vecTag.push_back(tagTemp);
		}
		++inf;

	}
	if(!vecTag.empty())
	{
		MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库
	}
}

/*
 * @Desc: 解析ASDU1遥信变位上送
 * @Param: buffer 开始地址
 * @Param: count 开始地址
 */
void Service::ParseASDU1(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[ADDR_POSI];
	uint8_t fun = buffer[FUN_POSI];
	uint8_t inf = buffer[INF_POSI];
	uint8_t dpi = buffer[DPI_POSI] & 0x03;
	bool bValue = false;
	char addr[100] = {0}; //103地址

	UnionTime4Byte unionTime4Byte;
	memcpy(unionTime4Byte.buf, buffer + ASDU_1_TIMESTAMP, 4);  //取四字节时间
	printf("%d:%d:%d\n",unionTime4Byte.stTime4Byte.hour,unionTime4Byte.stTime4Byte.min,unionTime4Byte.stTime4Byte.msec);

	std::vector<Tag> vecTag;
	if(2 == dpi)
	{
		bValue = true;  //true为合，false为分
	}

	sprintf(addr, "%d_%d%03d", clientAddr, fun, inf); //构造103地址
	Tag tagTemp;
	tagTemp.ZeroTag();
	tagTemp.dataType = TYPE_BOOL;
	std::string strGlgAddr = GetGlobalAddrByPrivateAddr(addr);//公共点地址
	if(!strGlgAddr.empty())
	{
		uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
		memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
		sprintf(tagTemp.pdata, "%d", bValue);
		vecTag.push_back(tagTemp);
		std::cout<<"save Mem database tagName ="<<tagTemp.pName<<" tagValue ="<<tagTemp.pdata<<std::endl;
		bool rs =MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库
        std::cout<<"save result ="<<rs<<endl;
	}
}

/*
 * @Desc: 解析ASDU2事故报文上送
 * @Param: buffer 开始地址
 * @Param: count 开始地址
 */
void Service::ParseASDU2(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[ADDR_POSI];
	uint8_t fun = buffer[FUN_POSI];
	uint8_t inf = buffer[INF_POSI];
	uint8_t dpi = buffer[DPI_POSI] & 0x03;

	UnionConvertShort unionConvert;
	memcpy(unionConvert.buf,buffer + RET_POSI, 2);
	short ret = unionConvert.value;
	memcpy(unionConvert.buf,buffer + FAN_POSI, 2);
	short fan = unionConvert.value;
	printf("ret = %d  fan = %d\n",ret,fan);

	bool bValue = false;
	char addr[100] = {0}; //103地址

	UnionTime4Byte unionTime4Byte;
	memcpy(unionTime4Byte.buf, buffer + ASDU_2_TIMESTAMP, 4);  //取四字节时间
	printf("%d:%d:%d\n",unionTime4Byte.stTime4Byte.hour,unionTime4Byte.stTime4Byte.min,unionTime4Byte.stTime4Byte.msec);
	std::vector<Tag> vecTag;
	if(2 == dpi)
	{
		bValue = true;  //true为合，false为分
	}

	sprintf(addr, "%d_%d%03d", clientAddr, fun, inf); //构造103地址
	Tag tagTemp;
	tagTemp.ZeroTag();
	tagTemp.dataType = TYPE_BOOL;
	std::string strGlgAddr = GetGlobalAddrByPrivateAddr(addr);//公共点地址
	if(!strGlgAddr.empty())
	{
		uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
		memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
		sprintf(tagTemp.pdata, "%d", bValue);
		vecTag.push_back(tagTemp);
		std::cout<<"save Mem database tagName ="<<tagTemp.pName<<" tagValue ="<<tagTemp.pdata<<std::endl;
		bool rs =MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库
		std::cout<<"save result ="<<rs<<endl;
	}
}

/*
 * @Desc: 通过I03点地址获取对应的合局地址
 * @Param: privateAddr 103点地址
 * @Return: 全局地址
 */
std::string Service::GetGlobalAddrByPrivateAddr(std::string privateAddr)
{
	std::string globalKey("");
	std::map<std::string, Point103>::const_iterator cit = m_pointAddrMap.find(privateAddr);
	if(cit != m_pointAddrMap.end())
	{
		globalKey = cit->second.pubAddr;
	}
	return globalKey;
}

float Service::GetRateByPrivateAddr(std::string privateAddr)
{
	float rate = 1.0f;
	std::map<std::string, Point103>::const_iterator cit = m_pointAddrMap.find(privateAddr);
	if(cit != m_pointAddrMap.end())
	{
		rate = cit->second.rate;
	}
	return rate;
}

std::string Service::ToHexString(const char* data, int32_t dataLen)
{
	uint16_t len =(dataLen+1)*3;
	char * hexStr = new char[len];
	memset(hexStr, 0, len);
	for(int i = 0; i < dataLen; ++i)
	{
		sprintf(hexStr + 3*i, "%02x ", 0xff & (uint8_t)data[i]);
	}
	std::string retStr = hexStr;
	delete [] hexStr;
	return retStr;
}

bool Service::InitPointAddrMap()
{
	m_pointAddrMap.clear();

	//从mysql数据库读取点表
	std::string strCmdTemp = "select NODE_INDEX,PUB_ADDR,RATE,TERMINAL from 103_node where IED_NAME in(";
	std::vector<std::string>::const_iterator cit = Config::GetInstance()->GetIedName().begin();
	for(; cit != Config::GetInstance()->GetIedName().end(); ++cit)
	{
		strCmdTemp+="\'";
		strCmdTemp+=*cit;
		strCmdTemp+="\'";
	}
	strCmdTemp += ")";
	char keyTemp[50] = {0};
	if(0 != EncapMysql::GetInstance()->SelectQuery(strCmdTemp.c_str()))
	{
		return false;
	}
	while (char** addr = EncapMysql::GetInstance()->FetchRow())
	{
		if((NULL == addr[0]) || (NULL == addr[1])
				||(NULL == addr[2]) || (NULL == addr[3]))
				{
					ERROR("cdt node table error \n");
					return false;
				}
		memset(keyTemp, 0, sizeof(keyTemp));
		sprintf(keyTemp, "%d_%s",atoi(addr[3]), addr[0] );
		Point103 point103;
		point103.pubAddr = addr[1];
		point103.rate = atof(addr[2]);
		m_pointAddrMap.insert(make_pair(keyTemp, point103));
	}

	if(0 == m_pointAddrMap.size())
	{
		ERROR("cmd[%s] get nothing \n", strCmdTemp.c_str());
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV1, "point table get noting");
		return false;
	}

	return true;
}

bool Service::StartBreakHeart()
{
	pthread_t tid = -1;
	if(0 != pthread_create(&tid, NULL, ThreadFunc, NULL))
	{
		ERROR("pthread_create error \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "pthread_create error");
		return false;
	}
	pthread_detach(tid);
	return true;
}

void * Service::ThreadFunc(void *arg)
{
	while(true)
	{
		//更新心跳时间到采集进程表
		char cmd[512] = {0};
		sprintf(cmd, "update collect_process set HEART_BEAT_TIME=%ld WHERE id=%d;", time(NULL), Config::GetInstance()->GetId());
		if(0 != EncapMysql::GetInstance()->ModifyQuery(cmd))
		{
			WARN("cmd[%s] run error \n",cmd);
		}
		sleep(m_heartInterval); //
	}

	return (void*)0;
}

/*
 * @Desc: 解析通用分类数据响应（读目录，读一个组的描述，读一个组的值）
 * @Param: buffer 开始地址
 * @Param: count 开始地址
 */
void Service::ParseASDU10(const char* buffer, int32_t count)
{
	uint8_t fun = buffer[10];
	uint8_t inf = buffer[11];

	if((0xFE == fun) && (0xF0 == inf)) //通用分类数据响应命令（装置响应的读目录）
	{

	}
	else if((0xFE == fun) && (0xF1 == inf)) //通用分类数据响应命令(读一个组的描述或值)
	{
		ParseASDU10AllValue(buffer, count);
	}
	else if((0xFE == fun) && (0xF4 == inf))     //ASDU10上送要测量
	{

	}
	else
	{
		WARN("unknown asdu10:%s", ToHexString(buffer, count).c_str());
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV1, "received unknown asdu10");
		return;
	}
}

void Service::ParseASDU10AllValue(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[5];
	uint8_t ngdNum = buffer[13] & 0x3F; // 返回的数据个数
	uint16_t index = 0;//数据的偏移量
	std::vector<Tag> vecTag;
	char addr[100] = {0};
	for(int i = 0; i < ngdNum; ++i)
	{
		memset(addr, 0, sizeof(addr));
		uint8_t gin_0 = buffer[14 + index];           // 组号
		uint8_t gin_1 = buffer[15 + index];           // 条目号
		uint8_t type = buffer[17 + index];            //数据类型
		uint8_t wideLen  = buffer[18 + index];        //数据每个元素宽度
		uint8_t valueNum  = buffer[19 + index] &0x7F; //数据元素数目
		uint8_t valueLen = wideLen * valueNum;        //数据占用多少字节
		char valueStr[20] = {0};
		memcpy(valueStr, &buffer[20 + index], valueLen);

		sprintf(addr, "%d_%d%03d", clientAddr, gin_0, gin_1); //构造103地址

		//不是遥信或者遥测数据
		if(m_ycGroupNum != gin_0 && m_yxGroupNum != gin_0)
		{
			break;
		}
		// 未定义的点不进行解析
		std::string strGlgAddr = GetGlobalAddrByPrivateAddr(addr);//公共点地址
//		if(strGlgAddr.empty())
//		{
//			continue;
//		}
//		if(0x01 != kod) //只处理数值不处理描述
//		{
//			continue;
//		}
		Tag tagTemp;
		tagTemp.ZeroTag();


		if(12 == type) //带品质描述值
		{
			tagTemp.dataType = TYPE_FLOAT;
			//信息元
			uint8_t qds = valueStr[0];
			uint16_t tmpValue = valueStr[0];
			float  value = 0.0f;
			if(qds & 0x02) //当前值无效
			{
				continue;
			}

			bool flag = (tmpValue & 0x8000) ? true: false; //是否为负数
			tmpValue = ~tmpValue;
			tmpValue &= 0xfff8; //品质位置0
			tmpValue &= 0x7ff8; //符号位置0
			tmpValue >>= 3;
			tmpValue += 1;
			value = (flag?(-tmpValue):tmpValue)*GetRateByPrivateAddr(addr);
			sprintf(tagTemp.pdata, "%.4f", value);
		}
		else if(9 == type)//双点信息
		{
			tagTemp.dataType = TYPE_BOOL;
			uint8_t dpi = valueStr[0] & 0x03;
			bool bValue = false;
			if(2 == dpi)
			{
				bValue = true;  //true为合，false为分
			}
			sprintf(tagTemp.pdata, "%d", bValue);
		}
		else if(7 == type) //R32.23，IEEE标准754短实数
		{
			tagTemp.dataType = TYPE_FLOAT;
			UnionConvertFloat convertUnion;
			memcpy(convertUnion.buf,valueStr,4);
			sprintf(tagTemp.pdata, "%.4f", convertUnion.value);
		}
		else if(3 == type)// 无符号整数
		{
			tagTemp.dataType = TYPE_LONG;
			UnionConvertUint convertUnion;
			memcpy(convertUnion.buf,valueStr,4);
			sprintf(tagTemp.pdata, "%u", convertUnion.value);
		}
		else if(10 == type)//单点信息
		{
			tagTemp.dataType = TYPE_BOOL;
			uint8_t siq = valueStr[0];
			bool bValue = false;
			if(0x01 & siq)
			{
				bValue = true;  //true为合，false为分
			}
			sprintf(tagTemp.pdata, "%d", bValue);
		}
		else
		{
			break;
		}

//		uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
//		memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
//		vecTag.push_back(tagTemp);
//		MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库

		if(index > count - 14 - 2) //不应该发生这种情况
		{
			WARN("data parse error:%s", ToHexString(buffer, count).c_str());
			return;
		}

	}

}
