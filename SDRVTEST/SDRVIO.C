/***********************************/
/* SCSI Driver/Firmware Test 2.70ž */
/*                                 */
/* (C) 2014-2025 Uwe Seimet        */
/***********************************/


#include <stdio.h>
#include "sdrvio.h"


FILE *out;


void
output(const char *msg)
{
	printf(msg);
	fprintf(out, msg);
}
