/*****************************/
/* SCSI Driver/Firmware Test */
/*                           */
/* (C) 2014-2026 Uwe Seimet  */
/*****************************/


#include <stdio.h>
#include "sdrvio.h"


FILE *out;


void
logMsg(const char *msg)
{
	printf("%s", msg);
	fprintf(out, "%s", msg);
}
