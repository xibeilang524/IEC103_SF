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
	memset(m_datapacket, 0, sizeof(m_datapacket));
	m_maxReSendTimes = 3;
	m_sendInterval = 3;

	m_iec103CodeS2C.fcv = FCV_1;
	m_iec103CodeS2C.fcb = 0;

	m_isResetConEnd = false;  //复位通信
	m_isResetNumEnd = false;  //复位帧计数位
	m_isGetAllEnd = false;    //总召唤

	m_ycGroupNum = 0x09;
	m_yxGroupNum = 0x60;
	m_timeStampAddr = 0xff;
	m_clientAddr = 0x32;

	m_nextCmd = CMD_RESET_EVENT;
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
	case CMD_RESET_TIMESTAMP:
		strCmd = CmdGetTimeStamp();
		break;
	case CMD_RESET_EVENT:
		strCmd = CmdResetEventAlarm();
		break;
	case CMD_SETTING_DOWNLOAD:
		strCmd = CmdSettingDownload();
		break;
	case CMD_SETTING_MODIFY:
		strCmd = CmdSettingModify();
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
	case CMD_GET_SINGLE_SETTING_VALUE:
		strCmd = CmdGetEntryValue(m_GroupNo,m_EntryNo);
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
		printf("read[%s]: ",g_cmdName[m_nextCmd]);
		printf("%s \n", ToHexString(m_datapacket, strlen(m_datapacket)).c_str());
	}
	else if(0 == readSize)//未读到数据
	{
		printf("read[%s]: no data\n",g_cmdName[m_nextCmd]);
		WARN("read[%s]: no data\n",g_cmdName[m_nextCmd]);
		if(CMD_RESET_CON == m_nextCmd) //如果上一次指令是通信复位不做重发计数
		{
			return true;
		}
		reSendTimes++;
	}
	else
	{
		reSendTimes = 0;
		DEBUG("read[%s] size %d :%s \n", g_cmdName[m_nextCmd], readSize,ToHexString(m_datapacket, readSize).c_str());

		//TODO:测试时使用
		printf("size: %d  read[%s] : ",readSize,g_cmdName[m_nextCmd]);
		printf("%s \n", ToHexString(m_datapacket, readSize).c_str());

		if(!ParseRecvData(m_datapacket, readSize))
		{
			WARN("recv invalied data: %s\n", ToHexString(m_datapacket, readSize).c_str());
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
		m_iec103CodeS2C.fcb = m_iec103CodeS2C.fcb ^ FCB_1;   //把FCB位取反
		m_iec103CodeS2C.fcv = FCV_1;
		m_nextCmd = CMD_GET_DATA_LV1;
	}
	else
	{
		m_iec103CodeS2C.fcv = (m_iec103CodeS2C.fcv | FCV_1) ^ FCV_1;  //把FCV位置0
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
	std::vector<char> packet(5);
	packet[0] = START_10H;
	packet[1] = code;
	packet[2] = m_clientAddr;
	packet[3] = packet[1]+packet[2];
	packet[4] = END_16H;
	return packet;
}


// 对时
std::vector<char> Service::CmdGetTimeStamp()
{
	std::vector<char> packet(21);

	struct timeval cur_t;
	gettimeofday(&cur_t,NULL);
	struct tm *p;
	p = localtime(&cur_t.tv_sec);
	stTime7Byte timeByte;
	timeByte.year = p->tm_year + 1900;
	timeByte.month = p->tm_mon + 1;
	timeByte.day = p->tm_mday;
	timeByte.hour = p->tm_hour;
	timeByte.min = p->tm_min;
	timeByte.msec = cur_t.tv_usec/1000;
	//printf("time_now:%d%d%d%d%d%d.%ld\n", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, cur_t.tv_usec/1000);

	packet[0] = START_68H;
	packet[1] = 0x0f;
	packet[2] = 0x0f;
	packet[3] = START_68H;
	packet[4] = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_POSTDATA_4;
	packet[5] = m_timeStampAddr;
	packet[6] = ASDU6_TIMESTAMP;
	packet[7] = 0x81;  //vsq
	packet[8] = COT_S2C_TIMESTAMP;
	packet[9] = m_timeStampAddr;   //ASDU_ADDR   0xFF=广播方式    装置地址=点对点方式
	packet[10] = FUN_GLB;
	packet[11] = 0x00;
	packet[12] = ((char*)(&timeByte))[0];
	packet[13] = ((char*)(&timeByte))[1];
	packet[14] = ((char*)(&timeByte))[2];
	packet[15] = ((char*)(&timeByte))[3];
	packet[16] = ((char*)(&timeByte))[4];
	packet[17] = ((char*)(&timeByte))[5];
	packet[18] = ((char*)(&timeByte))[6];
	packet[19] = SumCheck( &(packet[4]), (uint8_t)packet[1] );
	packet[20] = END_16H;
	return packet;
}


// 事件告警复归
std::vector<char> Service::CmdResetEventAlarm()
{
	std::vector<char> packet(16);

	packet[0] = START_68H;
	packet[1] = 0x0a;
	packet[2] = 0x0a;
	packet[3] = START_68H;
	packet[4] = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_POSTDATA_3;
	packet[5] = m_clientAddr;
	packet[6] = ASDU20_RESET;
	packet[7] = 0x81;                  //vsq
	packet[8] = COT_S2C_COMMON_ORDER;  //一般命令
	packet[9] = m_clientAddr;
	packet[10] = FUN_GLB;              //FUN
	packet[11] = INF_RESET_ORDER;      //复归命令
	packet[12] = 0x02;                 //DCO=1 跳 =2合  0，3未用
	packet[13] = 0x00;                 //如果时点对点发送，从站响应报文附加信息SIN和此值相同
	packet[14] = SumCheck( &(packet[4]), (uint8_t)packet[1] );
	packet[15] = END_16H;
	return packet;
}


// 定值下载
std::vector<char> Service::CmdSettingDownload()
{
	std::vector<char> packet(26);

	stDataUnit dataUnit;
	dataUnit.groupNo = 1;
	dataUnit.entryNo = 3;
	dataUnit.datatype = 1;
	dataUnit.kod = 1;
	dataUnit.datatype = 7;
	dataUnit.datasize = 4;
	dataUnit.number = 1;

	packet[0] = START_68H;
	packet[1] = 0x14;
	packet[2] = 0x14;
	packet[3] = START_68H;
	packet[4] = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_POSTDATA_3;
	packet[5] = m_clientAddr;
	packet[6] = ASDU10_SETTING;
	packet[7] = 0x81;                    //vsq
	packet[8] = COT_S2C_GENERAL_WRITE;   //通用分类写命令
	packet[9] = m_clientAddr;
	packet[10] = FUN_GEN;                //FUN
	packet[11] = INF_CONFIRM_WRITE;      //INF
	packet[12] = 0x03;                   //RII
	packet[13] = 0x01;                   //通用分类标识符数目

	packet[14] = ((char*)&dataUnit)[0];
	packet[15] = ((char*)&dataUnit)[1];
	packet[16] = ((char*)&dataUnit)[2];
	packet[17] = ((char*)&dataUnit)[3];
	packet[18] = ((char*)&dataUnit)[4];
	packet[19] = ((char*)&dataUnit)[5];

	packet[20] = 0x00;
	packet[21] = 0x00;
	packet[22] = 0xa0;
	packet[23] = 0x41;

	packet[24] = SumCheck( &(packet[4]), (uint8_t)packet[1] );
	packet[25] = END_16H;
	return packet;
}


// 定值修改
std::vector<char> Service::CmdSettingModify()
{
	std::vector<char> packet(26);

	stDataUnit dataUnit;
	dataUnit.groupNo = 1;
	dataUnit.entryNo = 3;
	dataUnit.datatype = 1;
	dataUnit.kod = 1;
	dataUnit.datatype = 7;
	dataUnit.datasize = 4;
	dataUnit.number = 1;

	packet[0] = START_68H;
	packet[1] = 0x14;
	packet[2] = 0x14;
	packet[3] = START_68H;
	packet[4] = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_POSTDATA_3;
	packet[5] = m_clientAddr;
	packet[6] = ASDU10_SETTING;
	packet[7] = 0x81;                    //vsq
	packet[8] = COT_S2C_GENERAL_WRITE;   //通用分类写命令
	packet[9] = m_clientAddr;
	packet[10] = FUN_GEN;                //FUN
	packet[11] = INF_EXEC_WRITE;         //INF
	packet[12] = 0x04;                   //RII
	packet[13] = 0x01;                   //通用分类标识符数目

	packet[14] = ((char*)&dataUnit)[0];
	packet[15] = ((char*)&dataUnit)[1];
	packet[16] = ((char*)&dataUnit)[2];
	packet[17] = ((char*)&dataUnit)[3];
	packet[18] = ((char*)&dataUnit)[4];
	packet[19] = ((char*)&dataUnit)[5];

	packet[20] = 0x00;
	packet[21] = 0x00;
	packet[22] = 0xa0;
	packet[23] = 0x41;

	packet[24] = SumCheck( &(packet[4]), (uint8_t)packet[1] );
	packet[25] = END_16H;
	return packet;
}


// 总召唤
std::vector<char> Service::CmdGetAll()
{
	std::vector<char> packet(15);
	static uint8_t scn = 1;
	if(255 == scn)
	{
		scn = 1;
	}
	packet[0] = START_68H;
	packet[1] = 0x09;
	packet[2] = 0x09;
	packet[3] = START_68H;
	packet[4] = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_POSTDATA_3;
	packet[5] = m_clientAddr;
	packet[6] = ASDU7_GETALL;
	packet[7] = 0x81;                  //vsq
	packet[8] = COT_S2C_GETALL_START;  //总召唤
	packet[9] = m_clientAddr;
	packet[10] = FUN_GLB;  // FUN
	packet[11] = 0x00;
	packet[12] = scn++;
	packet[13] = SumCheck( &(packet[4]), (uint8_t)packet[1] );
	packet[14] = END_16H;
	return packet;
}

/*
 * @Desc: 构造通用分类读命令（读一个组所有条目的值）
 * @Param: groupNum要读的组号
 * @Return: 通用分类读命令字符串
 */
std::vector<char> Service::CmdGetGroupValue(uint8_t groupNo)
{
	std::vector<char> packet(19);
	packet[0] = START_68H;
	packet[1] = 0x0D;
	packet[2] = 0x0D;
	packet[3] = START_68H;
	packet[4] = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_POSTDATA_3;
	packet[5] = m_clientAddr;
	packet[6] = ASDU21_GETGROUP;
	packet[7] = 0x81;  //vsq
	packet[8] = COT_S2C_GENERAL_READ;
	packet[9] = m_clientAddr;
	packet[10] = FUN_GEN;            //通用分类服务功能类型
	packet[11] = INF_READ_GROUP;     //读一个组的全部条目的值或者属性
	packet[12] = 0x15;               //返回信息标识符RII
	packet[13] = 0x01;               //通用分类标识数目
	packet[14] = groupNo;
	packet[15] = 0;
	packet[16] = KOD_1;
	packet[17] = SumCheck( &(packet[4]), (uint8_t)packet[1] );
	packet[18] = END_16H;
	return packet;
}

std::vector<char> Service::CmdGetEntryValue(uint8_t groupNo, uint8_t entryNo)
{
	std::vector<char> packet(19);
	packet[0] = START_68H;
	packet[1] = 0x0D;
	packet[2] = 0x0D;
	packet[3] = START_68H;
	packet[4] = PRA_1|m_iec103CodeS2C.fcb|m_iec103CodeS2C.fcv|FNA_S2C_POSTDATA_3;
	packet[5] = m_clientAddr;
	packet[6] = ASDU21_GETGROUP;
	packet[7] = 0x81;  //vsq
	packet[8] = COT_S2C_GENERAL_READ;
	packet[9] = m_clientAddr;
	packet[10] = FUN_GEN;            //通用分类服务功能类型
	packet[11] = INF_READ_EBTRY;     //读一个组的全部条目的值或者属性
	packet[12] = 0x15;               //返回信息标识符RII
	packet[13] = 0x01;               //通用分类标识数目
	packet[14] = groupNo;
	packet[15] = entryNo;
	packet[16] = KOD_1;
	packet[17] = SumCheck( &(packet[4]), (uint8_t)packet[1] );
	packet[18] = END_16H;
	return packet;
}
uint8_t Service::SumCheck( const char* packet, int32_t length )
{
	uint8_t result = 0;
	for( int32_t i = 0; i < length; i++ )
	{
		result += packet[i];
	}
	return result;
}

/*
 * @Desc: 解析ASDU44上送全遥信
 * @Param: packet 开始地址
 * @Param: length 开始地址
 */
void Service::ParseASDU44(const char* packet, int32_t length)
{
	uint8_t clientAddr = packet[5];
	uint8_t vsq = packet[7];
	uint8_t fun = packet[10];
	uint8_t inf = packet[11];
	uint8_t scdNum = vsq & 0x7F; // 信息元个数
    bool bValue = false;
    char addr[100] = {0}; //103地址
    std::vector<Tag> vecTag;
    for(int i = 0; i < scdNum; ++i)
    {
    	memset(addr, 0, sizeof(addr));
    	//信息元
		uint8_t pScd = packet[12 + i*5];
		uint8_t qds = packet[16 + i*5]; //品质描述字节
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
 * @Param: packet 开始地址
 * @Param: length 开始地址
 */
void Service::ParseASDU40(const char* packet, int32_t length)
{
	uint8_t clientAddr = packet[5];
	uint8_t fun = packet[10];
	uint8_t inf = packet[11];
	uint8_t siq = packet[12]; //带品质描述的单点信息字节
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
 * @Param: packet 开始地址
 * @Param: length 开始地址
 * @Return: 较验码
 */
void Service::ParseASDU41(const char* packet, int32_t length)
{
	ParseASDU40(packet, length);
}

void Service::ParseASDU42(const char* packet, int32_t length)
{
	uint8_t vsq = packet[VSQ_POSI];
	uint8_t clientAddr = packet[ADDR_POSI];
	uint8_t fun = packet[FUN_POSI];;
	uint8_t inf = packet[INF_POSI];
	uint8_t dpiNum = vsq & 0x7F; 	// 信息元个数

    std::vector<Tag> vecTag;
    for(int i = 0; i < dpiNum; ++i)
    {
		uint8_t dpi = packet[DPI_POSI + i]; //品质描述的双点信息

		if(!((dpi == 0x01) || (dpi == 0x02))) //当前值无效  dpi为2位数组，值=0/3为无意义，值=1为分，值=2为合
		{
			continue;
		}

		char addr[100] = {0}; 			 //103地址
		sprintf(addr, "%d_%d%03d", clientAddr, fun, inf); //构造103地址

		Tag tagTemp;
		tagTemp.ZeroTag();
		tagTemp.dataType = TYPE_BOOL;
		std::string strGlgAddr = GetGlobalAddrByPrivateAddr(addr);//公共点地址
		if(!strGlgAddr.empty())
		{
			uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
			memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
			sprintf(tagTemp.pdata, "%d", (dpi - 1) & 0x01);
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
 * @Param: packet 开始地址
 * @Param: length 开始地址
 */
void Service::ParseASDU50(const char* packet, int32_t length)
{
	uint8_t vsq = packet[VSQ_POSI];
	uint8_t clientAddr = packet[ADDR_POSI];
	uint8_t fun = packet[FUN_POSI];
	uint8_t inf = packet[INF_POSI];
	uint8_t scdNum = vsq & 0x7F; // 信息元个数
	float value = 0.0f;
	char addr[100] = {0}; //103地址
	std::vector<Tag> vecTag;
	for(int i = 0; i < scdNum; ++i)
	{
		memset(addr, 0, sizeof(addr));
		//信息元
		uint8_t qds = packet[12 + i*2];
		uint16_t tmpValue = packet[12 + i*2];
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
 * @Param: packet 开始地址
 * @Param: length 开始地址
 */
void Service::ParseASDU1(const char* packet, int32_t length)
{
	uint8_t code = packet[CODE_POSI];
	uint8_t clientAddr = packet[ADDR_POSI];
	uint8_t fun = packet[FUN_POSI];
	uint8_t inf = packet[INF_POSI];


	char addr[100] = {0}; //103地址
	std::vector<Tag> vecTag;
	stTime7Byte Time4Byte;
	memcpy(&Time4Byte, packet + ASDU_1_TIMESTAMP, 4);  //取四字节时间
	printf("%d:%d:%d\n",Time4Byte.hour,Time4Byte.min,Time4Byte.msec);

	if(code & ACD_1)
	{
		m_iec103CodeS2C.fcb = m_iec103CodeS2C.fcb ^ FCB_1;
		m_iec103CodeS2C.fcv = FCV_1;
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
		sprintf(tagTemp.pdata, "%d", (packet[DPI_POSI] - 1) & 0x01);  //双点描述 dpi=1为分，=2为和   0和3无意义
		vecTag.push_back(tagTemp);
		std::cout<<"save Mem database tagName ="<<tagTemp.pName<<" tagValue ="<<tagTemp.pdata<<std::endl;
		bool rs =MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库
        std::cout<<"save result ="<<rs<<endl;
	}
}

/*
 * @Desc: 解析ASDU2事故报文上送
 * @Param: packet 开始地址
 * @Param: length 开始地址
 */
void Service::ParseASDU2(const char* packet, int32_t length)
{
	uint8_t code = packet[CODE_POSI];
	uint8_t clientAddr = packet[ADDR_POSI];
	uint8_t fun = packet[FUN_POSI];
	uint8_t inf = packet[INF_POSI];
	uint8_t dpi = packet[DPI_POSI] & 0x03;

	UnionConvert2Byte unionConvert;
	memcpy(unionConvert.buf,packet + RET_POSI, 2);
	short ret = unionConvert.value;
	memcpy(unionConvert.buf,packet + FAN_POSI, 2);
	short fan = unionConvert.value;
	printf("ret = %d  fan = %d\n",ret,fan);
	bool bValue = false;
	char addr[100] = {0}; //103地址

	if(code & ACD_1)
	{
		m_iec103CodeS2C.fcb = m_iec103CodeS2C.fcb ^ FCB_1;
		m_iec103CodeS2C.fcv = FCV_1;
	}

	stTime7Byte Time4Byte;
	memcpy(&Time4Byte, packet + ASDU_2_TIMESTAMP, 4);  //取四字节时间
	printf("%d:%d:%d\n",Time4Byte.hour,Time4Byte.min,Time4Byte.msec);
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
 * @Param: packet 开始地址
 * @Param: length 开始地址
 */
void Service::ParseASDU10(const char* packet, int32_t length)
{
	uint8_t fun = packet[FUN_POSI];
	uint8_t inf = packet[INF_POSI];

	if((FUN_GEN == fun) && (0xF0 == inf)) //通用分类数据响应命令（装置响应的读目录）
	{

	}
	else if((FUN_GEN == fun) && (INF_READ_GROUP == inf)) //通用分类数据响应命令(读一个组的描述或值)
	{
		ParseASDU10AllValue(packet, length);
	}
	else if((FUN_GEN == fun) && (INF_READ_EBTRY == inf)) //通用分类数据响应命令(读一个条目的描述或值)
	{
		ParseASDU10AllValue(packet, length);
	}
	else if((FUN_GEN == fun) && (INF_CONFIRM_WRITE == inf)) //带确认的写条目响应
	{
		ParseASDU10AllValue(packet, length);
	}
	else if((FUN_GEN == fun) && (INF_EXEC_WRITE == inf))    //带执行的写条目响应
	{
		uint8_t rii = packet[RII_POSI];
	}
	else
	{
		WARN("unknown asdu10:%s", ToHexString(packet, length).c_str());
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV1, "received unknown asdu10");
		return;
	}
}

void Service::ParseASDU10AllValue(const char* packet, int32_t length)
{
	uint8_t code = packet[CODE_POSI];
	uint8_t clientAddr = packet[ADDR_POSI];
	uint8_t ngdNum = packet[NGD_POSI] & 0x3F; // 返回的数据个数
	uint16_t offset = 0;                      //数据的偏移量
	std::vector<Tag> vecTag;

	if(code & ACD_1)
	{
		m_iec103CodeS2C.fcb = m_iec103CodeS2C.fcb ^ FCB_1;
		m_iec103CodeS2C.fcv = FCV_1;
	}

	for(int i = 0; i < ngdNum; ++i)
	{
		stDataUnit dataUnit;
		memcpy(&dataUnit, packet + GROUP_POSI + offset, sizeof(stDataUnit));

		char addr[100] = {0};
		memset(addr, 0, sizeof(addr));
		sprintf(addr, "%d_%d%03d", clientAddr, dataUnit.groupNo, dataUnit.entryNo); //构造103地址

		//不是遥信或者遥测数据
		if(m_ycGroupNum != dataUnit.groupNo && m_yxGroupNum != dataUnit.groupNo)
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

		UnionConvert4Byte convertUnion4Byte;
		UnionConvert2Byte convertUnion2Byte;
		switch(dataUnit.datatype)
		{
		case 3:                                  //2字节无符号整形
			tagTemp.dataType = TYPE_LONG;
			memcpy(convertUnion2Byte.buf, packet + GID_POSI + offset, dataUnit.datasize);
			sprintf(tagTemp.pdata, "%u", convertUnion2Byte.uValue);
			break;

		case 7:                                   //短实数
			tagTemp.dataType = TYPE_FLOAT;
			memcpy(convertUnion4Byte.buf, packet + GID_POSI + offset, dataUnit.datasize);
			sprintf(tagTemp.pdata, "%.4f", convertUnion4Byte.fValue);
			break;

		case 9:                                   //双点描述 dpi=1为分，=2为和   0和3无意义
			tagTemp.dataType = TYPE_BOOL;
			if(!(((packet + GID_POSI + offset)[0] == 0x01) || ((packet + GID_POSI + offset)[0] == 0x02)))
			{
				break;
			}
			sprintf(tagTemp.pdata, "%d", ((packet + GID_POSI + offset)[0] - 1) & 0x01);
			break;

		case 10:                               //单点描述 siq=0为分，=1为和
			tagTemp.dataType = TYPE_BOOL;
			if(!(((packet + GID_POSI + offset)[0] == 0x00) || ((packet + GID_POSI + offset)[0] == 0x01)))
			{
				break;
			}
			sprintf(tagTemp.pdata, "%d", (packet + GID_POSI + offset)[0] & 0x01);
			break;

		case 12:                           //带品质描述词的被测值
			tagTemp.dataType = TYPE_FLOAT;
			//信息元
//			uint8_t qds = valueStr[0];
//			uint16_t tmpValue = valueStr[0];
//			float  value = 0.0f;
//			if(qds & 0x02) //当前值无效
//			{
//				continue;
//			}
//
//			bool flag = (tmpValue & 0x8000) ? true: false; //是否为负数
//			tmpValue = ~tmpValue;
//			tmpValue &= 0xfff8; //品质位置0
//			tmpValue &= 0x7ff8; //符号位置0
//			tmpValue >>= 3;
//			tmpValue += 1;
//			value = (flag?(-tmpValue):tmpValue)*GetRateByPrivateAddr(addr);
//			sprintf(tagTemp.pdata, "%.4f", value);
			break;
		default:
			break;
		}

//		uint16_t addrLen = (strGlgAddr.length() > sizeof(tagTemp.pName))? sizeof(tagTemp.pName): strGlgAddr.length();
//		memcpy(tagTemp.pName, strGlgAddr.c_str(), addrLen);
//		vecTag.push_back(tagTemp);
//		MemCache::GetInstance()->WritePointsValue_V(vecTag); //更新点到内存数据库

		offset = offset + dataUnit.datasize + 6;  //通用分类数据单元占6个字节
	}
}



