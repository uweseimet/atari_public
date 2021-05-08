/****************************************/
/* SCSI Driver Error Handling Test 1.00 */
/*                                      */
/* (C) 2021 Uwe Seimet                  */
/****************************************/


#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <std.h>
#include <tos.h>
#include <scsi3.h>
#include <scsidrv/scsidefs.h>


bool Inquiry(UWORD);
bool getCookie(LONG, ULONG *);


tpScsiCall scsiCall;
tSCSICmd cmd;
SENSE_DATA senseData;


#pragma warn -par
void
main(WORD argc, const char *argv[])
{
	UWORD bus, device, lun;
	tBusInfo busInfo;
	DLONG scsiId;
	ULONG maxLen;
	LONG oldstack = 0;
	LONG result;

	getCookie('SCSI', (ULONG *)&scsiCall);
	if(!scsiCall) {
		printf("SCSI Driver not found\n");

		goto error;
	}

	printf("SCSI Driver Error Handling Test V1.00\n");
	printf("½ 2021 Uwe Seimet\n\n");

	printf("Found SCSI Driver version %d.%02d\n\n", scsiCall->Version >> 8,
		scsiCall->Version & 0xff);

	cmd.Flags = 0;
	cmd.SenseBuffer = (BYTE *)&senseData;
	cmd.Timeout = 2000;

	if(!Super((void *)1L)) oldstack = Super(0L);

	result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result) {
		printf("Bus ID: %d, Bus name: '%s'\n", busInfo.BusNo, busInfo.BusName);

		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	printf("\nEnter bus ID, device ID, LUN ID: ");
	scanf("%d,%d,%d", &bus, &device, &lun);
	printf("\n");

	scsiId.hi = 0;
	scsiId.lo = device;
	cmd.Handle = (tHandle)scsiCall->Open(bus, &scsiId, &maxLen);
	if(((LONG)cmd.Handle >> 24) < 0) {
		printf("Unknown IDs or device not found\n");

		goto error;
	}

	if(!Inquiry(lun)) {
		goto error;
	}

	printf("\nChange medium and press a key\n");

	Cconin();

	printf("\nCalling Error()\n");

	result = scsiCall->Error(cmd.Handle, cErrRead, cErrMediach);

	scsiCall->Close(cmd.Handle);

	if(oldstack) Super((void *)oldstack);

	printf("\nStatus: %ld\n", result);

	Cconin();

	return;

error:

	scsiCall->Close(cmd.Handle);

	if(oldstack) Super((void *)oldstack);

	printf("\nTest failed\n");

	Cconin();
}
#pragma warn .par


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
