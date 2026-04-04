/****************************************/
/* SCSI Driver Error Handling Test 1.04 */
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
bool Inquiry(UWORD);


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

	scsiCall = GetScsiDriver("SCSI Driver Error Handling Test V1.04");
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

	if(!Inquiry(lun)) {
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


bool
Inquiry(UWORD lun)
{
	SENSE_BLK Inquiry = {
		0x12, 0x00, 0x00, 0x00, 0x00, (UBYTE)sizeof(INQUIRY_DATA), 0x00, 0x00, 0x00
	};

	LONG status;
	INQUIRY_DATA inquiryData;

	Inquiry.lun = lun;
	cmd.Cmd = (void *)&Inquiry;
	cmd.CmdLen = 6;
	cmd.Buffer = &inquiryData;
	cmd.TransferLen = Inquiry.length;

	memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		printf("INQUIRY failed: %ld\n", status);

		return false;
	}

	inquiryData.revision[0] = 0;
	printf("Device name: '%s'\n\n", inquiryData.vendor);

	printf("Removable media support: %s\n",
		inquiryData.RMB ? "Yes" : "No");

	if(!inquiryData.RMB) 	{
		printf("\nRemovable media support is required\n");
	};

	return inquiryData.RMB;
}
