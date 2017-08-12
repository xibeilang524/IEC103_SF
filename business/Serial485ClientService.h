/*
 * Serial485ClientService.h
 *
 *  Created on: Jul 14, 2015
 *      Author: root
 */

#ifndef SERIAL485CLIENTSERVICE_H_
#define SERIAL485CLIENTSERVICE_H_

#include "SerialService.h"

class Serial485ClientService :public SerialService
{
public:
	Serial485ClientService();
	virtual ~Serial485ClientService();
	virtual void Start();
protected:
	/*
	 * @Desc: 读取从站上送的数据
	 * @Return:　成功或失败
	 */
	virtual bool Read();
};

#endif /* SERIAL485CLIENTSERVICE_H_ */
