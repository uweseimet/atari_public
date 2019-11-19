/**************************/
/* Sample HDDRIVER module */
/*                        */
/* (C) 2019 Uwe Seimet    */
/**************************/

#include <tos.h>
#include <modstart.h>

int
main()
{
	if(isHddriverModule()) {
		Cconws("\r\nSample program was started as an HDDRIVER module");
	}
	else {
		Cconws("\r\nSample program was started as a regular program");
	}

	Cconws("\r\nPress any key to continue ...");

	Cconin();

	/* A non-resident module has to terminate with Pterm*() */
	if(isHddriverModule()) {
		Pterm0();
	}

	return 0;
}