/* 
* Copyright (c) 2015，大唐先一
* All rights reserved. 
*  
* 文件名称：DBHelper.h
* 文件标识：内存数据操作接口
*  
* 历史版本：1.0 
* 作    者：高海清
* 初始化完成日期：2015年5月18日 
*/

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>

#ifdef WIN32

	#ifdef IDBTPYE_EXPORTS
	#define IDBTPYE __declspec(dllexport)
	#else
	#define IDBTPYE __declspec(dllimport)
	#endif
#else
	#define IDBTPYE
#endif


using namespace std;
//数据类型自定义
typedef std::vector<unsigned char> BYTES;
typedef std::string KEY;
typedef std::string STRING;
typedef std::vector<KEY> KEYS;
typedef std::vector<STRING> STRINGS;//多
typedef std::vector<std::pair<KEY, BYTES> > PAIRS;
typedef PAIRS::value_type PAIR;
typedef std::pair<int, BYTES> INT_VALUE;
//点信息
enum DATATYPE{TYPE_STRING=1,TYPE_INT32,TYPE_FLOAT,TYPE_LONG,TYPE_BOOL,TYPE_DOUBLE,TYPE_UNKOWN};
//连接模式
#define SUB_TYPE 1  //数据订阅模式
#define PUB_TYPE 0  //数据发布模式
#define DEFAULT_TAGNAME_LEN 64 
#define DEFAULT_DATA_LEN   128
#define DEFAULT_CHENEL_LEN  10
const std::string DEFAULT = "TESTNO";
#pragma pack(push,1)
struct Tag
{
	Tag(){
		dataType = TYPE_UNKOWN;
		memset(pName,0,sizeof(pName));
		memset(pdata,0,sizeof(pdata));
		m_dataSize = 0;//默认不用分配内存
		pResizeData = NULL;
	}
	~Tag(){
		if (pResizeData)
		{
			delete []pResizeData;
			pResizeData = NULL;
		}
	}
public:
	char dataType;
	char pName[DEFAULT_TAGNAME_LEN];
	char pdata[DEFAULT_DATA_LEN];
	char *pResizeData;
	int m_dataSize;
public:
	Tag& operator = (Tag &m_right)
	{
		dataType = m_right.dataType;
		memcpy(pName,m_right.pName,sizeof(m_right.pName));
		memcpy(pdata,m_right.pdata,sizeof(m_right.pdata));
		return *this;
	}
	void ZeroTag()
	{
		dataType = TYPE_UNKOWN;
		memset(pName,0,sizeof(pName));
		memset(pdata,0,sizeof(pdata));
		if (pResizeData)
		{
			delete []pResizeData;
			pResizeData = NULL;
		}
	}
	void CopyStrData(const char * pSrcData,int len)
	{
		if (len<0)
		{
			return;
		}
		if ( len > 0 && len < DEFAULT_DATA_LEN )
		{
			memcpy(pdata,pSrcData,len);
		}
		else
		{
			
			if (pResizeData == NULL)
			{
				pResizeData = new char[len+1];
				memset(pResizeData,0,len+1);
				m_dataSize = len;
				memcpy(pResizeData,pSrcData,len);
			}
			else
			{
				delete []pResizeData;
				pResizeData = new char[len+1];
				memset(pResizeData,0,len+1);
				m_dataSize = len;
				memcpy(pResizeData,pSrcData,len);
			}
		}
		return;

	}
};
#pragma pack(pop)



//
//读写内存数据库帮助类，封装了细节操作
//初始创建2015-5-4
class  IDB_Type
{
public:
	IDB_Type(){
		
	}
	virtual~IDB_Type(){}
public:
	//***************************************************************************
	//功能：连接内存数据库函数 OK
	//输入：服务器Ip ：host 服务器端口：port,ConnectTpye连接类型1:订阅，0：发布
	//输出：连接成功与否
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool Connect(int ConnectTpye,const char* host,unsigned short u_port  ) = 0;
	
	//***************************************************************************
	//功能：检查数据库是否连接正常 OK
	//输入：ConnectTpye类型，是订阅还是发布模式
	//输出：成功与否
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool check_connection(int ConnectTpye) = 0;


	//***************************************************************************
	//功能：断开内存数据库连接及释放资源函数 OK
	//输入：
	//输出：断开成功与否
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool DisConnect() = 0;


	//***************************************************************************
	//功能：写单点数据(key ,value)不建议这样写，点多的话效率很低 OK
	//输入：m_key点名字，value点值
	//输出：
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool WritePointValue(const char* m_key,const char* value) = 0;
	
	
	//***************************************************************************
	//功能：写单点数据(key ,value)不建议这样写，点多的话效率很低 OK
	//输入：m_key点名字，value点值
	//输出：
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool WritePointValue(Tag *m_tag) = 0;


	//***************************************************************************
	//功能：写多点数据(pTags)建议写 OK
	//输入：pTags，m_count点数
	//输出：成功与否
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool WritePointsValue(Tag * pTags,int m_count) = 0;


	//***************************************************************************
	//功能：写多点数据(pTags)建议写 OK
	//输入：pTags，m_count点数
	//输出：成功与否
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool WritePointsValue(std::vector<Tag>& m_tag_ver) = 0;


	//***************************************************************************
	//功能：读单点数据(key ,value)不建议这样写，点多的话效率很低
	//输入：m_key点名字，value点值
	//输出：成功与否
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool ReadPointValue(const char* m_key, char* value) = 0;
	


	//***************************************************************************
	//功能：读单点数据(tag)不建议这样写，点多的话效率很低 0K
	//输入：m_key点名字，value点值
	//输出：
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool ReadPointValue(Tag *m_tag) = 0;

	//***************************************************************************
	//功能：读多点点数据(tag) OK
	//输入：m_tag多点，tagNum需要读的点数
	//输出：m_count返回的点数
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool ReadPointsValue(Tag *m_tag,int tagNum ) = 0;

	//***************************************************************************
	//功能：测试是否点存在 OK
	//输入：name 点名，
	//输出：成功与否
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool ExistPoint(const char*  name) = 0;




	//***************************************************************************
	//功能：模拟信号通知机制,客户端写数据后调用，实时响应数据更新 OK
	//输入：需要通知的通道，可以通知多个通道pCh,m_ch_size通道个数
	//输出：
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual void post( char *pCh,int m_ch_size) = 0;


	//***************************************************************************
	//功能：等待通道通知 OK
	//输入：
	//输出：
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual bool wait(int timeout = 0) = 0;


	//***************************************************************************
	//功能：订阅设置通道的数据 OK
	//输入：pCh需要订阅的通道数组
	//输出：
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual void subscribe( char *pCh,int m_ch_size ) = 0;

	//***************************************************************************
	//功能：设置缓存点大小
	//输入：m_size预先设置容器缓存大小
	//输出：
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual void SetVecSize(int m_size) = 0;

	//***************************************************************************
	//功能：获取更新的数据点
	//输入：m_max_size取点最大值，m_tag初始化的数据
	//输出：
	//时间：[2015/5/18]
	//作者：GHQ
	//***************************************************************************
	virtual int GetUpdatePoints(Tag *m_tag,int m_max_size) = 0;

	//数据转换辅助函数
public:
virtual	void FloatToStr(float f_value,char* pData) = 0;
virtual void IntToStr(int f_value,char* pData) = 0;
virtual	void BoolToStr(bool b_value,char* pData) = 0;


public:
	std::string m_chenle;//通道
	std::vector< Tag > m_tag_vec;
};










typedef IDB_Type* IDB_PtrT;