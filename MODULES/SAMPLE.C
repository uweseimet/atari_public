/**************************/
/* HDDRIVER sample module */
/*                        */
/* (C) 2019 Uwe Seimet    */
/**************************/

#include <stdio.h>
#include <stdlib.h>
#include <tos.h>
#include <modstart.h>

int
main()
{
	if(isHddriverModule()) {
		Cconws("\r\nSample was started as an HDDRIVER module");
	}
	else {
		Cconws("\r\nSample was started as a regular program");
	}

	Cconws("\r\nPress any key to continue ...");

	Cconin();

	/* A module has to terminate with Pterm*() */
	if(isHddriverModule()) {
		Pterm0();
	}

	return 0;
}