/*
 * SerialNet.cpp
 *
 *  Created on: Jun 14, 2015
 *      Author: root
 */

#include "SerialNet.h"

#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <dlfcn.h>
#include "Logger.h"
#include "../business/IEC103Type.h"
#include "WarningLib.h"

#define SERIAL_SO_NAME "Serial.so"

SerialNet::SerialNet(const std::string & portPath, int32_t speed, int32_t databits, int32_t stopbits, char parity, int32_t outTime)
		:Inet(),
		m_SerialPath(portPath),
		m_speed(speed),
		m_databits(databits),
		m_stopbits(stopbits),
		m_parity(parity),
		m_outTime(outTime)
{
	m_handle = NULL;
	Pfun_InitSerial = NULL;
	Pfun_Connect = NULL;
	Pfun_Broken = NULL;
	Pfun_Recv = NULL;
	Pfun_Send = NULL;
	Pfun_UnInitSerial = NULL;
}

SerialNet::~SerialNet()
{
	if(-1 != m_netFd)
	{
		close(m_netFd);
	}
	if((Pfun_Broken != NULL)
			&&(Pfun_UnInitSerial != NULL))
	{
		Pfun_Broken();
		Pfun_UnInitSerial();
	}

}

/*
 * @Desc: 进行通讯连接
 * @Return: 连接成功或失败
 */
bool SerialNet::Connect()
{
	char buf[512] = {0};
	getcwd(buf, sizeof(buf));
	std::string strRedisPath(buf);
	strRedisPath.append("/").append(SERIAL_SO_NAME);

	m_handle = dlopen(strRedisPath.c_str(),RTLD_LAZY ); /*打开动态链接库*/
	if(m_handle == NULL)
	{
		ERROR("dlopen error:%s \n", dlerror());
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "serial dlopen error");
		 return false;
	}

	//获取函数指针
	if(!GetFunction(reinterpret_cast<Handle &>(Pfun_InitSerial), "InitSerial"))
	{
		return false;
	}
	//获取函数指针
	if(!GetFunction(reinterpret_cast<Handle &>(Pfun_Connect), "Connect"))
	{
		return false;
	}
	//获取函数指针
	if(!GetFunction(reinterpret_cast<Handle &>(Pfun_Broken), "Broken"))
	{
		return false;
	}
	//获取函数指针
	if(!GetFunction(reinterpret_cast<Handle &>(Pfun_Recv), "Recv"))
	{
		return false;
	}
	//获取函数指针
	if(!GetFunction(reinterpret_cast<Handle &>(Pfun_Send), "Send"))
	{
		return false;
	}
	//获取函数指针
	if(!GetFunction(reinterpret_cast<Handle &>(Pfun_UnInitSerial), "UnInitSerial"))
	{
		return false;
	}
	if(!Pfun_InitSerial(m_SerialPath.c_str(), m_speed, m_databits, m_parity, m_stopbits,MAX_SIZE, 100,4))
	{
		char szMsg[128];
		sprintf(szMsg,"InitSerial failed comName=%s speed=%d m_databits=%d parity=%d \n",
				m_SerialPath.c_str(),
				m_speed,
				m_databits,
				m_parity);

		ERROR(szMsg);
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "InitSerial failed");
		return false;
	}
	if(!Pfun_Connect())
	{
		ERROR("Connect failed \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "Connect failed");
		return false;
	}
//	//打开串口
//	if(-1 == (m_netFd = open(m_SerialPath.c_str(), O_RDWR | O_NOCTTY)))
//	{
//		ERROR("serial[%s]open failed\n", m_SerialPath.c_str());
//		return false;
//	}

	//设置为阻塞模式
//	if(fcntl(m_netFd, F_SETFL, 0)<0)
//	{
//		ERROR("fcntl failed\n");
//		return false;
//	}
//	//设置波特率
//	if(!SetSpeed())
//	{
//		ERROR("set speed failed \n");
//		return false;
//	}
//	// 设置数据位 停止位 奇偶较验 超时时间
//	if(!SetParity())
//	{
//		return false;
//	}
	return true;
}

bool SerialNet::GetFunction(Handle & handle, const char* funcName)
{
	handle = dlsym(m_handle,funcName);
	char * errorInfo = dlerror();
	if (errorInfo != NULL){
		ERROR("%s method get failed%s\n", funcName, errorInfo);
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "serial.so dlsys error");
		return false;
	}
	return true;
}

/*
 * @Desc: 读取数据
 * @Param: pRecvData 读取数据存放地址
 * @Param: dataMaxLen 读取数据的最大长度
 * @Return: 读取的数据长度 错误的数据返回－2
 */
int32_t SerialNet::Read(char * pRecvData, uint32_t dataMaxLen)
{
	int32_t readLen = 0;
	int32_t readAllLen = 0;
	uint64_t startTime = time(NULL);

	while((readLen = Pfun_Recv((unsigned char*)(pRecvData + readAllLen), dataMaxLen - readAllLen)) >= 0)
	{
		readAllLen += readLen;
		if(readAllLen >= (int32_t)dataMaxLen)
		{
			break;
		}
		if((5 == readAllLen)
				&&(START_10H == pRecvData[0])
				&& (END_16H == pRecvData[4]))
		{
			if(SumCheck( pRecvData + 1, 2) == (uint8_t)pRecvData[3])
			{
				return readAllLen;
			}
			else
			{
				break;
			}
		}
		else if(readAllLen >= 14 )
		{
			if((START_68H == pRecvData[0])
					&& (pRecvData[1] == pRecvData[2])
					&& (START_68H == pRecvData[3])
					&& ((6 + pRecvData[1])== readAllLen)
					&& (END_16H == pRecvData[readAllLen -1 ])
					&& (SumCheck(pRecvData + 4, pRecvData[1]) == (uint8_t)(pRecvData[pRecvData[1]+4])))
			{
				return readAllLen;
			}
		}

		if(abs(time(NULL) - startTime) > m_outTime)
		{
			break;
		}
	}
	if(-1 == readLen)
	{
		ERROR("read error:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "serial read error");
		return -1;
	}
	if(0 == readAllLen)
	{
		return 0;
	}

	WARN("recv invalied data: %s\n", ToHexString(pRecvData, readAllLen).c_str());
	WarningLib::GetInstance()->WriteWarn2(WARNING_LV0, "recv invalied data");
	return -2;
}

/*
 * @Desc: 发送数据
 * @Param: pSendData 发送数据存放地址
 * @Param: uint32_t 发送的数据的长度
 * @Return: 发送的数据长度
 */
int32_t SerialNet::Write(const char * pSendData, uint32_t dataLen)
{
	int32_t writeLen = 0;
	if(-1 == (writeLen = Pfun_Send((unsigned char*)(pSendData), dataLen)))
	{
		ERROR("write error:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV2, "serial write error");
		return -1;
	}
	return writeLen;
}

/*
 * @Desc: 断开通讯连接
 * @Return: 成功或失败
 */
bool SerialNet::DisConnect()
{
	close(m_netFd);
	m_netFd = -1;
	return true;
}

/*
 * @Desc:设置波特率
 * @Return: 设置成功或失败
 */
bool SerialNet::SetSpeed()
{
	int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300, B38400, B19200, B9600, B4800, B2400, B1200, B300, };
	int name_arr[] = {38400, 19200, 9600, 4800, 2400, 1200, 300, 38400, 19200, 9600, 4800, 2400, 1200, 300, };
	struct termios Opt;    //定义termios结构

	//获取终端参数
	if(tcgetattr(m_netFd, &Opt) != 0)
	{
		ERROR("tcgetattr failed:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "tcgetattr failed");
		return false;
	}
	for(uint32_t i = 0; i < sizeof(speed_arr) / sizeof(speed_arr[0]); i++)
	{
		if(m_speed == name_arr[i])
		{
			tcflush(m_netFd, TCIOFLUSH); // 清除所有正在发生的I/O数据
			cfsetispeed(&Opt, speed_arr[i]); // 设置输入波特率
			cfsetospeed(&Opt, speed_arr[i]); // 设置输出波特率
			//设置属性生效
			if(tcsetattr(m_netFd, TCSANOW, &Opt) != 0)
			{
				ERROR("tcsetattr failed:%s \n",strerror(errno));
				WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "tcsetattr failed");
				return false;
			}
			tcflush(m_netFd, TCIOFLUSH);
			break;
		}
	}
	return true;
}

/*
 * @Desc:设置数据位、停止位和校验位
 * @Return: 设置成功或失败
 */
bool  SerialNet::SetParity()
{
	struct termios newtio,oldtio;
	if ( tcgetattr( m_netFd,&oldtio) != 0)
	{
		ERROR("tcgetattr failed: \n",strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "tcgetattr failed");
		return false;
	}

	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;
	newtio.c_oflag &= ~(ONLCR | OCRNL);
	newtio.c_iflag &= ~(IXON | IXOFF | IXANY);

	//设置数据位
	switch( m_databits )
	{
		case 7:
			newtio.c_cflag |= CS7;
		break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
		default:
			ERROR("set databits failed \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "set databits failed");
			return false;
	}

	// 设置奇偶较验位
	switch( m_parity )
	{
		case 'O':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;
		case 'E':
			newtio.c_iflag |= (INPCK | ISTRIP);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			break;
		case 'N':
			newtio.c_cflag &= ~PARENB;
			break;
		default:
			ERROR("set parity failed \n");
			WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "set parity failed");
			return false;
	}

	// 设置停止位
	if( m_stopbits == 1 )
	{
		newtio.c_cflag &= ~CSTOPB;
	}
	else if ( m_stopbits == 2 )
	{
		newtio.c_cflag |= CSTOPB;
	}
	else
	{
		ERROR("set stopbits failed \n");
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "set stopbits failed");
		return false;
	}
	newtio.c_cc[VTIME] =m_outTime; //测试时该大一点
	newtio.c_cc[VMIN] = 0;//set min read byte!
	tcflush(m_netFd,TCIFLUSH);

	if((tcsetattr(m_netFd,TCSANOW,&newtio))!=0)
	{
		ERROR("tcsetattr failed:%s \n", strerror(errno));
		WarningLib::GetInstance()->WriteWarn2(WARNING_LV3, "tcsetattr failed");
		return false;
	}

	return true;
}

uint8_t SerialNet::SumCheck( const char* buffer, int32_t count )
{
	uint8_t result = 0;
	for( int32_t i = 0; i < count; i++ )
	{
		result += buffer[i];
	}
	return result;
}

std::string SerialNet::ToHexString(const char* data, int32_t dataLen)
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
