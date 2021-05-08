/**************************************/
/* SCSI Driver Error Status Test 1.00 */
/*                                    */
/* (C) 2021 Uwe Seimet                */
/**************************************/


#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <std.h>
#include <tos.h>
#include <scsi3.h>
#include <scsidrv/scsidefs.h>


void ReadCapacity(tSCSICmd *, UWORD);
bool getCookie(LONG, ULONG *);


tpScsiCall scsiCall;
tSCSICmd cmd1, cmd2;
SENSE_DATA senseData;


#pragma warn -par
void
main(WORD argc, const char *argv[])
{
	UWORD bus, device;
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

	printf("SCSI Driver Error Status Test V1.00\n");
	printf("½ 2021 Uwe Seimet\n\n");

	printf("Found SCSI Driver version %d.%02d\n\n", scsiCall->Version >> 8,
		scsiCall->Version & 0xff);

	cmd1.Flags = 0;
	cmd2.Flags = 0;
	cmd1.SenseBuffer = (BYTE *)&senseData;
	cmd2.SenseBuffer = (BYTE *)&senseData;
	cmd1.Timeout = 2000;
	cmd2.Timeout = 2000;

	if(!Super((void *)1L)) oldstack = Super(0L);

	result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result) {
		printf("Bus ID: %d, Bus name: '%s'\n", busInfo.BusNo, busInfo.BusName);

		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	printf("\nEnter bus ID, device ID: ");
	scanf("%d,%d", &bus, &device);
	printf("\n");

	scsiId.hi = 0;
	scsiId.lo = device;

	cmd1.Handle = (tHandle)scsiCall->Open(bus, &scsiId, &maxLen);
	if(((LONG)cmd1.Handle >> 24) < 0) {
		printf("Unknown IDs or device not found\n");

		goto error;
	}

	cmd2.Handle = (tHandle)scsiCall->Open(bus, &scsiId, &maxLen);
	if(((LONG)cmd2.Handle >> 24) < 0) {
		printf("Unknown IDs or device not found\n");

		goto error;
	}

/*	ReadCapacity(&cmd1, 3);*/
	ReadCapacity(&cmd2, 3);

	printf("\nSetting error status for handle 1\n");
	scsiCall->Error(cmd1.Handle, cErrWrite, cErrMediach);

	result = scsiCall->Error(cmd1.Handle, cErrRead, cErrMediach);
	printf("\nError status 1 for handle 1: %ld\n", result);

	result = scsiCall->Error(cmd1.Handle, cErrRead, cErrMediach);
	printf("\nError status 2 for handle 1: %ld\n", result);

	result = scsiCall->Error(cmd2.Handle, cErrRead, cErrMediach);
	printf("\nError status 1 for handle 2: %ld\n", result);

	result = scsiCall->Error(cmd2.Handle, cErrRead, cErrMediach);
	printf("\nError status 2 for handle 2: %ld\n", result);

	scsiCall->Close(cmd1.Handle);
	scsiCall->Close(cmd2.Handle);

	if(oldstack) Super((void *)oldstack);

	printf("\nStatus: %ld\n", result);

	Cconin();

	return;

error:

	scsiCall->Close(cmd1.Handle);
	scsiCall->Close(cmd2.Handle);

	if(oldstack) Super((void *)oldstack);

	printf("\nTest failed\n");

	Cconin();
}
#pragma warn .par


void
ReadCapacity(tSCSICmd *cmd, UWORD lun)
{
	BYTE ReadCapacityCmd[10] = {
		0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0x00
	};

	LONG status;
	ULONG capacity[] = { 0L, 0L };

	ReadCapacityCmd[1] = lun << 5;
	cmd->Cmd = (void *)&ReadCapacityCmd;
	cmd->CmdLen = (UWORD)sizeof(ReadCapacityCmd);
	cmd->Buffer = capacity;
	cmd->TransferLen = sizeof(capacity);

	status = scsiCall->In(cmd);
	if(status) {
		printf("READ CAPACITY failed: %ld\n", status);

		return;
	}

	if(!capacity[0]) {
		printf("Wrong capacity '0'\n");

		return;
	}

	if(!capacity[1]) {
		printf("Wrong block size '0'\n");

		return;
	}
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
