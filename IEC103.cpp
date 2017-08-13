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
	IEC103Manager app;

	if(!app.Init())
	{
		return -1;
	}

	app.Start();
	app.Stop();

	return 0;
}
