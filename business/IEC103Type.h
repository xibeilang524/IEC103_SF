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


//标志位在报文中的位置
#define TYP_POSI      6        //类型标识
#define VSQ_POSI      7        //可变结构限定词
#define COT_POSI      8        //传送原因
#define ADDR_POSI     9        //设备地址
#define FUN_POSI      10       //功能类型
#define INF_POSI      11       //信息序号
#define DPI_POSI      12       //双点信息
#define RET_POSI      13       //相对时间   表示装置启动到该元件动作的相对时间，毫秒
#define FAN_POSI      14       //故障序号
#define ASDU_1_TIMESTAMP  13   //ASDU1中四字节时间起始位置
#define ASDU_2_TIMESTAMP  17   //ASDU2中四字节时间起始位置


#define CONFIRM_07H  0x07
#define CONFIRM_0BH  0x0B


#define START_10H 0x10    //固定长度报文启动字符
#define START_68H 0x68    //可变长度报文启动字符

#define END_16H   0x16    //结束字符
#define FCB_1     0x20   //帧计数位
#define FCV_1     0x10   //帧计数有效位
#define ACD_1     0x20   //有一级数据上送
#define PRA_1     0x40   //启动报文位　主到从

//功能码定义
#define FNA_S2C_RESET_CON    0   //复位通信功能码
#define FNA_S2C_RESET_NUM    7   //复位帧计数位功能码
#define FNA_S2C_POSTDATA     3   //传送数据（发送/确认帧）
#define FNA_S2C_GETDATA_LV1  10  //召唤一级数据功能码
#define FNA_S2C_GETDATA_LV2  11  //召唤二级数据功能码

//ASDU号定义
#define ASDU7_GETALL  7     // 总召唤
#define ASDU21_GETGROUP  21 // 总召唤

//COT传送原因定义
#define COT_S2C_GETALL_START 9     // 总查询(总召唤)的启动
#define COT_S2C_GENERAL_READ 0x2A   //通用分类读命令

//FUN定义
#define FUN_GETALL 255

#define MAX_SIZE 1024

typedef struct {
	uint8_t fcb; // 桢记数位
	uint8_t fcv; // 桢记数有效位
} IEC103CodeS2C;


union UnionConvertFloat
{
	float value;
	char buf[4];
};

union UnionConvertUint
{
	unsigned int value;
	char buf[4];
};

union UnionConvertShort
{
	short value;
	char buf[2];
};

typedef struct
{
	uint16_t msec;             //毫秒
	uint8_t  min:6,            //分钟
			 res:1,
		     iv:1;             //IV=0为有效  =1为无效
	uint8_t  hour:7,           //小时
		     su:1;             //su夏时制标志
}StructTime4Byte;

union UnionTime4Byte
{
	StructTime4Byte stTime4Byte;
	char buf[4];
};


// 带品质描述词的被测量(MEA)
struct MEA{
	union {
		short	mval:13,	// 带符号规一化定点数，最高位符合位，数据位占12位
				res:1,		// 备用位:未用(常为0)
				er:1,		// 差错位:0-有效，1-无效
				ov:1;		// 溢出位:0-无溢出，1-溢出

		short   wVal;
	}mea;
};


#endif /* IEC103TYPE_H_ */