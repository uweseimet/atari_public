/***********************************/
/* SCSI Driver/Firmware Test 3.00ž */
/*                                 */
/* (C) 2014-2025 Uwe Seimet        */
/***********************************/


#include <stdio.h>
#include "sdrvio.h"


FILE *out;


void
logMsg(const char *msg)
{
	printf(msg);
	fprintf(out, msg);
}
