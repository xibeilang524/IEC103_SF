/*
 * MemCache.h
 *
 *  Created on: Jun 16, 2015
 *      Author: root
 */

#ifndef MEMCACHE_H_
#define MEMCACHE_H_

#include <string>
#include <string.h>
#include <vector>
#include <stdint.h>

#include "DBHelper.h"

class MemCache {
public:
	static MemCache * GetInstance();
	/*
	* @Desc: 初始化，读取so，连接内存数据库
	* @Return: 成功或失败
	*/
	bool Init(const std::string & redisAddr, uint16_t redisPort);
	/*
	* @Desc: 释放内存数据库
	* @Return: 成功或失败
	*/
	void Free();
	/*
	* @Desc: 更新点到内存数据库
	* @Param: m_tag_ver要更新的点集合
	* @Return: 成功或失败
	*/
	bool WritePointsValue_V(std::vector<Tag>& m_tag_ver);
	/*
	* @Desc: 数据更新通知
	* @Param: pCh通道名数组
	* @Param: m_ch_size 通道个数
	* @Return: 成功或失败
	*/
	void Post( char *pCh,int m_ch_size);
	typedef void * Handle;
private:
	MemCache();
	virtual ~MemCache();
private:
	bool GetFunction(Handle * handle, const char* funcName);
private:
	//初始化
	bool (*Pfun_InitDB)();
	//卸载数据库
	bool (*Pfun_UnInitDB)();
	//连接内存数据库函数
	bool (*Pfun_Connect)(int ConnectTpye,const char* host,unsigned short u_port);
	//断开数据库
	bool (*Pfun_DisConnect)();
	//写点数据
	bool (*Pfun_WritePointsValue_C)(Tag * pTags,int m_count);
	//写多点容器
	bool (*Pfun_WritePointsValue_V)(std::vector<Tag>& m_tag_ver);
	//写单点
	bool (*Pfun_WritePointValue)(Tag *m_tag);
	//更新通知
	void (*Pfun_post)(char *pch, int m_ch_size);
	bool (*Pfun_CheckConnection)(int ConnectTpye);
private:
	static MemCache * m_instance;
	void *m_handle;
	std::string m_redisAddr;
	uint16_t m_redisPort;
};

#endif /* MEMCACHE_H_ */
