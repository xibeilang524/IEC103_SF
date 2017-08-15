//============================================================================
// Name        : IEC103.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <unistd.h>
#include "business/IEC103Manager.h"


int main(int32_t argc, char *argv[])
{
	IEC103Manager manager;

	if(!manager.Init())
	{
		return -1;
	}

	manager.Start();
	manager.Stop();

	return 0;
}
