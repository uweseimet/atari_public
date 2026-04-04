/****************************************/
/* SCSI Driver Error Handling Test 1.05 */
/*                                      */
/* (C) 2021-2026 Uwe Seimet             */
/****************************************/


#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <tos.h>
#include <scsi3.h>
#include <scsidrv/scsidefs.h>
#include "std.h"
#include "util.h"


int HandleError(void);


LONG oldstack = 0;
tpScsiCall scsiCall;
tSCSICmd cmd;
SENSE_DATA senseData;


#pragma warn -par
int
main(WORD argc, const char *argv[])
{
	UWORD bus, lun;
	LONG result;

	scsiCall = GetScsiDriver("SCSI Driver Error Handling Test V1.05");
	if(!scsiCall) {
		Cconin();

		return 0;
	}

	cmd.Flags = 0;
	cmd.SenseBuffer = (BYTE *)&senseData;
	cmd.Timeout = 2000;

	if(!Super((void *)1L)) {
		oldstack = Super(0L);
	}

	cmd.Handle = GetHandle(scsiCall, &bus, NULL, &lun);
	if(!cmd.Handle) {
		return HandleError();
	}

	if(!Inquiry(scsiCall, &cmd, lun)) {
		return HandleError();
	}

	printf("\nChange medium and press a key\n");

	Cconin();

	printf("\nCalling Error()\n");

	result = scsiCall->Error(cmd.Handle, cErrRead, cErrMediach);

	scsiCall->Close(cmd.Handle);

	if(oldstack) {
		Super((void *)oldstack);
	}

	printf("\nStatus: %ld\n", result);

	Cconin();

	return 0;
}
#pragma warn .par


int
HandleError()
{
	if(cmd.Handle) {
		scsiCall->Close(cmd.Handle);
	}

	if(oldstack) {
		Super((void *)oldstack);
	}

	printf("\nTest failed\n");

	Cconin();

	return 0;
}
