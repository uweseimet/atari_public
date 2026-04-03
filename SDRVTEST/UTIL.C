/*****************************/
/* SCSI Driver/Firmware Test */
/*                           */
/* (C) 2014-2026 Uwe Seimet  */
/*****************************/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tos.h>
#include <scsidrv/scsidefs.h>
#include "std.h"
#include "util.h"


WORD SortBuses(const void *, const void *);


tHandle
GetHandle(tpScsiCall scsiCall, UWORD *bus, ULONG *device, UWORD *lun)
{
	tBusInfo busInfos[32];
	tBusInfo busInfo;
	DLONG scsiId = { 0, 0 };
	ULONG maxLen;
	UWORD busId;
	UWORD busCount = 0;
	tHandle handle;
	int s;

	LONG result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result && busCount < 32) {
		memcpy(&busInfos[busCount], &busInfo, sizeof(tBusInfo));

		busCount++;

		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	qsort(busInfos, busCount, sizeof(tBusInfo), SortBuses);

	for(busId = 0; busId < busCount; busId++) {
		printf("Bus ID: %d, Bus name: '%s'\n", busInfos[busId].BusNo,
		busInfos[busId].BusName);
	}

	if(lun) {
		printf("\nEnter bus ID, device ID, LUN ID: ");
		s = scanf("%d,%ld,%d", bus, &scsiId.lo, lun);
	}
	else {
		printf("\nEnter bus ID, device ID: ");
		s = scanf("%d,%ld", bus, &scsiId.lo);
	}
	printf("\n");

	if(!s) {
		printf("Input error\n");
		
		Cconin();

		return NULL;
	}

	handle = (tHandle)scsiCall->Open(*bus, &scsiId, &maxLen);
	if(((LONG)handle >> 24) < 0) {
		printf("Unknown IDs or device not found\n");

		return NULL;
	}

	if(device) {
		*device = scsiId.lo;
	}

	return handle;
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
