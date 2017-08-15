/*
 * TcpService.cpp
 *
 *  Created on: Apr 18, 2016
 *      Author: root
 */

#include "TcpService.h"
#include "../core/TcpNet.h"
#include "../core/SerialNet.h"
#include "../core/Logger.h"
#include "../core/WarningLib.h"
#include "../core/MemCache.h"
#include "../core/EncapMysql.h"


TcpService::TcpService() {
	// TODO Auto-generated constructor stub
	//m_tcpSendCmd = CMD_CONNECTION_CON;
	m_tcpSendCmd = CMD_CALL_DATA;
	m_TcpSendID = 0;
	m_TcpRecvID = 0;
}

TcpService::~TcpService() {
	// TODO Auto-generated destructor stub
}

void TcpService::Start()
{
	m_net = new TcpNet(m_tcpip,m_tcpport);

	//连接
	std::cout<<"Connection TYPE：103 TcpNet，ipAddr＝"<< m_tcpip<<" Port = "<<m_tcpport<<std::endl;

	if(!m_net->Connect())
	{
		ERROR( "tcp connection error  \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "tcp connection error");
		//return;
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

// send data
bool TcpService::Write()
{
	std::vector<char> strCmd;
	std::string cmdName("");
	//首先连接，再启动总召换
	switch(m_tcpSendCmd)
	{
		case CMD_CONNECTION_CON:
			strCmd = CmdConnection();
			break;
		case CMD_CALL_DATA:
			strCmd = Service::CmdGetAll();
			break;
		case CMD_GET_GROUPDATA:
			strCmd = CmdGetGroupData(1);
			break;
		case CMD_TEST1_DATA:
			strCmd = CmdGetTest1();
			break;
		case CMD_TEST2_DATA:
			strCmd = CmdGetTest2();
			break;
		case CMD_GENERAL_READ_YC_GROUP_VALUE:
			strCmd = CmdGetGroupValue(0);//TODO 遥测组号为0，需要确认不同厂家是否一致
			break;
		default:
			ERROR("cmd not defined \n" );
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "cmd not defined");
			return false;
	}
	DEBUG("Write[%s]:%s \n" ,g_TcpCmdName[m_tcpSendCmd], ToHexString(strCmd.data(), strCmd.size()).c_str());
	//TODO:测试时使用
	printf("write[%s]: ", g_TcpCmdName[m_tcpSendCmd]);
	printf("%s \n", ToHexString(strCmd.data(), strCmd.size()).c_str());

	//如果发送失败，就认为是已经断开连接了
	if(-1 == m_net->Write(strCmd.data(), strCmd.size()))
	{
		ERROR("send cmd %d error\n", m_tcpSendCmd);
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "send data error");
		m_tcpSendCmd = CMD_CONNECTION_CON;
		//return false;
	}
	return true;
}

bool TcpService::Read()
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
		//TODO:测试时使用
		printf("read[%s]: ",g_TcpCmdName[m_tcpSendCmd]);
		printf("%s \n", ToHexString(m_datapacket, strlen(m_datapacket)).c_str());
	}
	else if(0 == readSize)//未读到数据
	{
		printf("read[%s]: no data\n",g_TcpCmdName[m_tcpSendCmd]);
		WARN("read[%s]: no data\n",g_TcpCmdName[m_tcpSendCmd]);
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
		DEBUG("read[%s]:%s \n", g_TcpCmdName[m_tcpSendCmd], ToHexString(m_datapacket, readSize).c_str());
		if(START_68H == m_datapacket[0])
			DEBUG("{code=%d, ASDU=%d, FUN=%d, INF=%d} \n", (uint8_t)m_datapacket[4], (uint8_t)m_datapacket[6], (uint8_t)m_datapacket[10], (uint8_t)m_datapacket[11]);
		//TODO:测试时使用
		printf("read[%s]: ",g_TcpCmdName[m_tcpSendCmd]);
		printf("%s \n", ToHexString(m_datapacket, readSize).c_str());

		if(START_68H == m_datapacket[0])
		printf("{code=%d, ASDU=%d, FUN=%d, INF=%d} \n",
				(uint8_t)m_datapacket[4],
				(uint8_t)m_datapacket[TYP_POSI],
				(uint8_t)m_datapacket[FUN_POSI],
				(uint8_t)m_datapacket[INF_POSI]);
		if(!ParseRecvData(m_datapacket, readSize))
		{
			WARN("recv invalied data: %s\n", ToHexString(m_datapacket, readSize).c_str());
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV0, "recv invalied data");
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
	//sleep(m_sendInterval);

	return true;
}
bool TcpService::ParseRecvData(const char* pData, int32_t dataLen)
{
    if(dataLen < 5)
    {
    	return false;
    }
	if(START_68H == pData[0] && 7==dataLen ) //确认是否生效报文,固定报文
	{
		//连接
		if(CONFIRM_0BH == pData[3])
		{
			m_tcpSendCmd = CMD_CALL_DATA;
			return true;
		}
		//测试
		if(0x43 == pData[3] || 0x83 == pData[3])
		{
			if(0x43 == pData[3])
				m_tcpSendCmd = CMD_TEST1_DATA;
			else
				m_tcpSendCmd = CMD_TEST2_DATA;
			return true;
		}

	}
	//接收的序号(可变长度中与S帧，取接收的序号)
	m_TcpRecvID = pData[5]>>1 + pData[6]*127;
	if(7 == dataLen)
		return true;
	//长度大于7的时候，计算发送序号
	m_TcpSendID = pData[3] / 2 + pData[4]*127 +1;
	std::cout<<m_TcpSendID<<std::endl;
	if(CMD_GET_GROUPDATA == m_tcpSendCmd )
		m_tcpSendCmd = CMD_TEST2_DATA;
	if(START_68H == pData[0] && 10<dataLen && 0x0a==pData[9]) //总召唤结束
	{
		m_tcpSendCmd = CMD_GET_GROUPDATA;
		return true ;
	}else if(START_68H == pData[0] && 7==dataLen) //固定报文长度
	{
		m_tcpSendCmd = CMD_TEST2_DATA;
		return true;
	}else if(!ParseVariableData(pData, dataLen)) // 处理可变长数据
	{
		return false;
	}
	return true;
}
//进行连接的指令
std::vector<char> TcpService::CmdConnection()
{
	std::vector<char> buffer(7);
	buffer[0] = START_68H;
	buffer[1] = 0x00;
	buffer[2] = 0x04;
	buffer[3] = CONFIRM_07H;
	buffer[4] = 0x00;
	buffer[5] = 0x00;
	buffer[6] = 0x00;
	return buffer;
}
//测试指令1
std::vector<char> TcpService::CmdGetTest1()
{
	std::vector<char> buffer(7);

	buffer[0] = START_68H;
	buffer[1] = 0x00;
	buffer[2] = 0x04;
	buffer[3] = 0x83;
	buffer[4] = 0x00;
	buffer[5] = 0x00;
	buffer[6] = 0x00;

	return buffer;
}
//测试指令2
std::vector<char> TcpService::CmdGetTest2()
{
	std::vector<char> buffer(7);

	buffer[0] = START_68H;
	buffer[1] = 0x00;
	buffer[2] = 0x04;
	buffer[3] = 0x43;
	buffer[4] = 0x00;
	buffer[5] = 0x00;
	buffer[6] = 0x00;

	return buffer;
}
//总召唤指令
std::vector<char> TcpService::CmdGetAll()
{
	std::vector<char> buffer(15);
	static uint8_t scn = 1;
	if(255 == scn)
	{
		scn = 1;
	}

	buffer[0] = START_68H;
	buffer[1] = 0x00;
	buffer[2] = 12;
	buffer[3] = (GetSendID() % 127)<<1;
	buffer[4] = GetSendID() /127;
	buffer[5] = (GetRecvID() % 127)<<1;
	buffer[6] = GetRecvID() /127;
	buffer[7] = ASDU7_GETALL;   //类型，总召换
	buffer[8] = 0x81;  			//vsq
	buffer[9] = 0x09;   		//COT传送	原因
	buffer[10] = m_clientAddr;  //ASDU ADDR
	buffer[11] = m_clientAddr;  //ASDU ADDR
	buffer[12] = FUN_GLB;
	buffer[13] = 0x00 ;
	buffer[14] = scn++;   		//scn

	return buffer;
}
std::vector<char> TcpService::CmdGetGroupData(uint8_t groupNum)
{
	std::vector<char> buffer(20);
	buffer[0] = START_68H;
	buffer[1] = 0x00;   //h  len
	buffer[2] = 0x00;   //l  len

	buffer[3] = (GetSendID() % 127)<<1;;
	buffer[4] = GetSendID() /127;
	buffer[5] = (GetRecvID() % 127)<<1;
	buffer[6] = GetRecvID() / 127;

	buffer[7] = ASDU21_GETGROUP;
	buffer[8] = 0x81;				//vsq
	buffer[9] = COT_S2C_GENERAL_READ;
	buffer[10] = 0x00;
	buffer[11] = m_clientAddr;
	buffer[12] = 0xFE;  // FUN
	buffer[13] = 0xF1;
	buffer[14] = 0x15;
	buffer[15] = 0x01;
	buffer[16] = groupNum;
	buffer[17] = 0;
	buffer[18] = 1;
	buffer[19] = 0x01;

	return buffer;
}


// 处理可变长数据
bool TcpService::ParseVariableData(const char* pData, int32_t dataLen)
{
	uint8_t asduType = pData[TYP_POSI]; // asdu类型

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
		case 42:
			ParseASDU42(pData, dataLen);
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
//tcp ASDU1修改完
void TcpService::ParseASDU1(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[ADDR_POSI];
	uint8_t fun = buffer[FUN_POSI];
	uint8_t inf = buffer[INF_POSI];
	uint8_t dpi = buffer[DPI_POSI] & 0x03;

	bool bValue = false;
	char addr[100] = {0}; //103地址
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
//tcp ASDU10修改完
void TcpService::ParseASDU10(const char* buffer, int32_t count)
{
	uint8_t fun = buffer[FUN_POSI];
	uint8_t inf = buffer[INF_POSI];

	if((0xFE == fun) && (0xF0 == inf)) //通用分类数据响应命令（装置响应的读目录）
	{

	}
	else if((0xFE == fun) && (0xF1 == inf)) //通用分类数据响应命令(读一个组的描述或值)
	{
		ParseASDU10AllValue(buffer, count);
	}
	else
	{
		WARN("unknown asdu10:%s", ToHexString(buffer, count).c_str());
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV1, "received unknown asdu10");
		return;
	}
}
//tcp ASDU10修改完
void TcpService::ParseASDU10AllValue(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[ADDR_POSI];
	uint8_t ngdNum = buffer[15] & 0x3F; // 返回的数据个数
	uint16_t index = 0;//数据的偏移量
	std::vector<Tag> vecTag;
	char addr[100] = {0};
	for(int i = 0; i < ngdNum; ++i)
	{
		memset(addr, 0, sizeof(addr));
		uint8_t gin_0 = buffer[16 + index];     // 组号
		uint8_t gin_1 = buffer[17 + index];     // 条目号
		//uint8_t kod   = buffer[16 + index];
		uint8_t type = buffer[19 + index];      //数据类型
		uint8_t wideLen  = buffer[20 + index];  //数据每个元素宽度
		uint8_t valueNum  = buffer[21 + index] &0x7F; //数据元素数目
		uint8_t valueLen = wideLen * valueNum;        //数据占用多少字节
		char valueStr[20] = {0};
		memcpy(valueStr, &buffer[22 + index], valueLen);
		index += (6 + valueLen);
		sprintf(addr, "%d_%d%03d", clientAddr, gin_0, gin_1); //构造103地址
		//不是遥信或者遥测数据
		if(m_ycGroupNum != gin_0 || m_yxGroupNum != gin_0)
		{
			break;
		}
		// 未定义的点不进行解析
		std::string strGlgAddr = GetGlobalAddrByPrivateAddr(addr);//公共点地址
		if(strGlgAddr.empty())
		{
			continue;
		}

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
			float ftempValue = 0.0f;
			ftempValue = *((float*)valueStr[0]) * GetRateByPrivateAddr(addr);
			sprintf(tagTemp.pdata, "%.4f", ftempValue);
		}
		else if(3 == type)// 无符号整数
		{
			tagTemp.dataType = TYPE_LONG;
			uint64_t  tempValue = *((uint64_t*)valueStr[0]);
			sprintf(tagTemp.pdata, "%ld", tempValue);
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

		uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
		memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
		vecTag.push_back(tagTemp);
		MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库

		if(index > count - 14 - 2) //不应该发生这种情况
		{
			WARN("data parse error:%s", ToHexString(buffer, count).c_str());
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV1, "data parse error");
			return;
		}
	}
}
void TcpService::ParseASDU40(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[ADDR_POSI];
	uint8_t fun = buffer[FUN_POSI];
	uint8_t inf = buffer[INF_POSI];
	uint8_t siq = buffer[14]; //带品质描述的单点信息字节
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
void TcpService::ParseASDU41(const char* buffer, int32_t count)
{
	ParseASDU40(buffer, count);
}
void TcpService::ParseASDU42(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[11];
	uint8_t vsq = buffer[8];
	uint8_t fun = 0 ;
	uint8_t inf = buffer[11];
	uint8_t scdNum = vsq & 0x7F; 	// 信息元个数
    bool bValue = false;
    char addr[100] = {0}; 			//103地址
    std::vector<Tag> vecTag;
    for(int i = 0; i < scdNum; ++i)
    {
    	memset(addr, 0, sizeof(addr));
    	//信息元，占三个字节，fun inf diq
    	fun = buffer[12 + i*3];
    	inf = buffer[13 + i*3];
		uint8_t pScd = buffer[14 + i*3]; //品质描述的双点信息

		if(pScd & 0x80) //当前值无效
		{
			continue;
		}
		if(0x2==(0x02 & pScd))
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
    }
    if(!vecTag.empty())
    {
    	MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库
    }

}
void TcpService::ParseASDU44(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[ADDR_POSI];
	uint8_t fun = buffer[FUN_POSI];
	uint8_t inf = buffer[INF_POSI];
	uint8_t vsq = buffer[VSQ_POSI];
	uint8_t scdNum = vsq & 0x7F; // 信息元个数

    bool bValue = false;
    char addr[100] = {0}; //103地址
    std::vector<Tag> vecTag;

    for(int i = 0; i < scdNum; ++i)
    {
    	memset(addr, 0, sizeof(addr));
    	//信息元
		uint8_t pScd = buffer[14 + i*5];
		uint8_t qds = buffer[18 + i*5]; //品质描述字节
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

void TcpService::ParseASDU50(const char* buffer, int32_t count)
{
	uint8_t clientAddr = buffer[5];
	uint8_t vsq = buffer[7];
	uint8_t fun = buffer[10];
	uint8_t inf = buffer[11];
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

void TcpService::InitConfig()
{
	m_tcpip = Config::GetInstance()->GetIp();
	m_tcpport = Config::GetInstance()->GetTcpPort();
}
uint32_t TcpService::GetSendID()
{
	return m_TcpSendID;
}
uint32_t TcpService::GetRecvID()
{
	return m_TcpRecvID;
}
