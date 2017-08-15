/*
 * IEC103Type.h
 *
 *  Created on: Jun 15, 2015
 *      Author: root
 */

#ifndef IEC103TYPE_H_
#define IEC103TYPE_H_

#include <stdint.h>
#include <sys/types.h>


enum ASDU_TYPE
{
	ASDU_1 = 0x01,
	ASDU_2 = 0x02,
	ASDU_10 = 0x0a,
	ASDU_40 = 0x28,
	ASDU_41 = 0x29,
	ASDU_42 = 0x2a,
	ASDU_43 = 0x2b,
	ASDU_44 = 0x2c,
	ASDU_45 = 0x2d,
	ASDU_46 = 0x2e,
	ASDU_50 = 0x32,
};

enum CMD_SEND
{
	CMD_RESET_CON,
	CMD_RESET_NUM,
	CMD_RESET_TIMESTAMP,            //对时
	CMD_RESET_EVENT,                //复归命令
	CMD_SETTING_DOWNLOAD,           //定值下载
	CMD_SETTING_MODIFY,             //定值修改
	CMD_GET_ALL,
	CMD_GET_DATA_LV1,               //获取一级数据
	CMD_GET_DATA_LV2,               //获取二级数据
	CMD_GET_SINGLE_SETTING_VALUE,   //获取单个条目值
	CMD_GENERAL_READ_YX_GROUP_VALUE,
	CMD_GENERAL_READ_YC_GROUP_VALUE,
};

//标志位在报文中的位置
#define CODE_POSI     4        //控制域
#define TYP_POSI      6        //类型标识
#define VSQ_POSI      7        //可变结构限定词
#define COT_POSI      8        //传送原因
#define ADDR_POSI     9        //设备地址
#define FUN_POSI      10       //功能类型
#define INF_POSI      11       //信息序号
#define RII_POSI      12       //返回信息标识符
#define DPI_POSI      12       //双点信息
#define RET_POSI      13       //相对时间   表示装置启动到该元件动作的相对时间，毫秒
#define FAN_POSI      15       //故障序号
#define ASDU_1_TIMESTAMP  13   //ASDU1中四字节时间起始位置
#define ASDU_2_TIMESTAMP  17   //ASDU2中四字节时间起始位置
#define GROUP_POSI        14   //组号
#define TNTRY_POSI        15   //条目号
#define KOD_POSI          16   //描述的类别
#define GDD_POSI          17   //通用分类数据描述
#define GID_POSI          20   //通用分类标识数据
#define NGD_POSI          13   //通用分类数据集数目



#define START_10H 0x10    //固定长度报文启动字符
#define START_68H 0x68    //可变长度报文启动字符
#define END_16H   0x16    //结束字符
#define FCB_1     0x20    //帧计数位
#define FCV_1     0x10    //帧计数有效位
#define ACD_1     0x20    //有一级数据上送
#define PRA_1     0x40    //启动报文位　主到从
#define KOD_1     0x01    //描述类别  1为实际值

//功能码定义
#define FNA_S2C_RESET_CON    0   //复位通信功能码
#define FNA_S2C_RESET_NUM    7   //复位帧计数位功能码
#define FNA_S2C_POSTDATA_3   3   //传送数据（发送/确认帧）
#define FNA_S2C_POSTDATA_4   4   //传送数据（发送/无回答帧）
#define FNA_S2C_GETDATA_LV1  10  //召唤一级数据功能码
#define FNA_S2C_GETDATA_LV2  11  //召唤二级数据功能码

//ASDU号定义
#define ASDU6_TIMESTAMP  6      // 对时
#define ASDU7_GETALL     7      // 总召唤，遥信
#define ASDU10_SETTING   10     // 定值
#define ASDU20_RESET     20     // 复归
#define ASDU21_GETGROUP  21     // 总召唤，遥测

//COT传送原因定义
//控制方向
#define COT_S2C_TIMESTAMP     8      //时间同步
#define COT_S2C_GETALL_START  9      //总查询(总召唤)的启动
#define COT_S2C_COMMON_ORDER  0x14   //一般命令
#define COT_S2C_GENERAL_WRITE 0x28   //通用分类写命令
#define COT_S2C_GENERAL_READ  0x2A   //通用分类读命令


//FUN定义
#define FUN_GLB 0xFF       //全局功能类型
#define FUN_GEN 0xFE       //通用分类服务功能类型

//INF定义
#define INF_RESET_ORDER   0x13  //复归命令，所有装置相同
#define INF_READ_GROUP    0xF1  //读一个组的全部条目的值或者属性
#define INF_READ_EBTRY    0xF4  //读一个条目的值或者属性
#define INF_CONFIRM_WRITE 0xF9  //带确认的写条目
#define INF_EXEC_WRITE    0xFA  //带执行的写条目

#define MAX_SIZE 1024

typedef struct {
	uint8_t fcb; // 桢记数位
	uint8_t fcv; // 桢记数有效位
} IEC103CodeS2C;



union UnionConvert4Byte
{
	unsigned int uValue;
	float fValue;
	char buf[4];
};

union UnionConvert2Byte
{
	short  value;
	ushort uValue;
	char buf[2];
};

typedef struct
{
	//通用分类标识序号GIN
	uint8_t groupNo;    //组号
	uint8_t entryNo;    //条目号

	//描述的类别KOD
	uint8_t kod;        //1为实际值

	//通用分类数据描述GDD
	uint8_t datatype;   //数据类型
	uint8_t datasize;   //数据宽度
	uint8_t number:7,   //数目
			cont:1;     //后续状态位
}stDataUnit;     //通用分类数据单元


typedef struct
{
	uint16_t msec;              //毫秒
	uint8_t  min:6,             //分钟
			 res1:1,		    //备用
		     iv:1;              //IV=0为有效  =1为无效
	uint8_t  hour:7,            //小时
		     su:1;              //su夏时制标志
	uint8_t  day:5,             //日
			 dayofweek:3;       //未采用
	uint8_t  month:4,           //月
			 res2:4;            //备用
	uint8_t  year:7,            //年
			 res3:1;            //备用
}stTime7Byte;



// 带品质描述词的被测量(MEA)
typedef struct {
	union {
		short	mval:13,	// 带符号规一化定点数，最高位符合位，数据位占12位
				res:1,		// 备用位:未用(常为0)
				er:1,		// 差错位:0-有效，1-无效
				ov:1;		// 溢出位:0-无溢出，1-溢出

		short   wVal;
	}mea;
}MEA;







#endif /* IEC103TYPE_H_ */
