/*****************************/
/* SCSI Driver/Firmware Test */
/*                           */
/* (C) 2014-2026 Uwe Seimet  */
/*****************************/


#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <scsidrv/scsidefs.h>
#include "std.h"
#include "util.h"


UWORD
ScanBuses(tBusInfo *busInfos, tpScsiCall scsiCall)
{
	tBusInfo busInfo;
	UWORD busCount = 0;

	LONG result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result && busCount < 32) {
		memcpy(&busInfos[busCount], &busInfo, sizeof(tBusInfo));

		busCount++;

		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	qsort(busInfos, busCount, sizeof(tBusInfo), SortBuses);

	return busCount;
}


WORD
SortBuses(const void *b1, const void *b2)
{
	const tBusInfo *i1 = b1;
	const tBusInfo *i2 = b2;

	return i1->BusNo - i2->BusNo;
}


LONG
cookieptr()
{
	return *((LONG *)0x5a0);
}


bool
getCookie(LONG cookie, ULONG *p_value)
{
	LONG *cookiejar = (LONG *)Supexec(cookieptr);

	if(!cookiejar) return false;

	do {
		if(cookiejar[0] == cookie) {
			if (p_value) *p_value = (ULONG)cookiejar[1];
			return true;
		}
		else
			cookiejar = &(cookiejar[2]);
	} while(cookiejar[-2]);

	return false;
}
