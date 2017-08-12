/*
 * Warning.h
 *
 *  Created on: Jul 30, 2015
 *      Author: root
 */

#ifndef WARNINGLIB_H_
#define WARNINGLIB_H_

#include <string>
#include <string.h>
#include <stdint.h>
#include <mysql/mysql.h>

struct WarnInfo
{
	int32_t processIndex;
	int32_t warnLv;
	int32_t warnClass;
	int32_t warnType;
	int32_t warnReason;
	const char* warnDesc;
};

enum WarningLevlel
{
	WARNING_LV0 = 66,  // 告警
	WARNING_LV1 = 77, // 次要
	WARNING_LV2 = 88,  //　主要
	WARNING_LV3 = 99  //　紧急
};

class WarningLib {
public:
	static WarningLib * GetInstance();
	/*
	 * @Desc: 初始化链接
	 * @Param: szDbIp 要链接的mysql数据库IP地址
	 * @Param: szUser 要链接的mysql数据库用户名
	 * @Param: szPassword 要链接的mysql数据库密码
	 * @Param: szDb 要链接的mysql数据库名
	 * @Param: port 要链接的mysql数据库端口号
	 * @Return:成功或失败
	 */
	bool Init(const char* szDbIp, const char* szUser,
		const char* szPassword,const char* szDb, uint32_t port);

	/*
	 * @Desc: 启动告警链接，调用前需调用Init
	 * @Return:成功或失败
	 */
	bool Start();

	/*
	 * @Desc: 将告警数据写入数据库
	 * @Param: warnInfo 要写入的告警信息
	 * @Return:成功或失败
	 */
	bool WriteWarn(const WarnInfo * warnInfo );

	bool WriteWarn2(WarningLevlel warninglv, const char* errStr);

	/*
	 * @Desc: 停止告警链接
	 * @Return:成功或失败
	 */
	bool Stop();
private:
	typedef void * Handle;
	bool GetFunction(Handle & handle, const char* funcName);
private:
	bool (*pInit)(const char* szDbIp, const char* szUser,
		        const char* szPassword,const char* szDb, uint32_t port, MYSQL * sqlConnection);
	bool (*pStart)();

	bool (*pStop)();

	bool (*pWriteWarn)(const WarnInfo * warnInfo);
private:
	WarningLib();
	virtual ~WarningLib();
private:
	void *m_handle;
	static WarningLib * m_instance;
};

#endif /* WARNINGLIB_H_ */
