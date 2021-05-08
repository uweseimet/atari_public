/**************************************/
/* SCSI Driver Media Change Test 1.00 */
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


bool Inquiry(UWORD);
ULONG ReadCapacity(UWORD);
bool TestUnitReady(UWORD, bool);
bool Read(UWORD, ULONG);
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
	ULONG blockSize;
	LONG oldstack = 0;
	LONG result;

	getCookie('SCSI', (ULONG *)&scsiCall);
	if(!scsiCall) {
		printf("SCSI Driver not found\n");

		goto error;
	}


	printf("SCSI Driver Media Change Test V1.00\n");
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
	

	blockSize = ReadCapacity(lun);
	if(blockSize) {
		if(TestUnitReady(lun, false) && Read(lun, blockSize)) {
			printf("Now change the medium and then press a key\n");

			Cconin();

			printf("\n");

			if(TestUnitReady(lun, true)) {
				if(Read(lun, blockSize)) {
					scsiCall->Close(cmd.Handle);

					if(oldstack) Super((void *)oldstack);

					printf("Test was successful\n");

					Cconin();

					return;
				}
			}
		}
	}

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


ULONG
ReadCapacity(UWORD lun)
{
	BYTE ReadCapacityCmd[10] = {
		0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0x00
	};

	LONG status;
	ULONG capacity[] = { 0L, 0L };

	ReadCapacityCmd[1] = lun << 5;
	cmd.Cmd = (void *)&ReadCapacityCmd;
	cmd.CmdLen = (UWORD)sizeof(ReadCapacityCmd);
	cmd.Buffer = capacity;
	cmd.TransferLen = sizeof(capacity);

	status = scsiCall->In(&cmd);
	if(status) {
		printf("READ CAPACITY failed: %ld\n", status);

		return 0;
	}

	if(!capacity[0]) {
		printf("Wrong capacity '0'\n");

		return 0;
	}

	if(!capacity[1]) {
		printf("Wrong block size '0'\n");

		return 0;
	}

	printf("\nNumber of blocks: %ld, Block size: %ld\n\n",
		capacity[0] + 1, capacity[1]);

	return capacity[1];
}


bool
TestUnitReady(UWORD lun, bool expectChange)
{
	BYTE TestUnitReadyCmd[6] = { 0x00, 0, 0, 0, 0, 0 };

	LONG status;

	TestUnitReadyCmd[1] = lun << 5;
	cmd.Cmd = (void *)&TestUnitReadyCmd;
	cmd.CmdLen = (UWORD)sizeof(TestUnitReadyCmd);
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	status = scsiCall->In(&cmd);
	if(status) {
		char s[20];

		sprintf(s, "%ld", status);

		printf("TEST UNIT READY failed (%s): %s",
			expectChange ? "expected" : "unexpected",
			status == 2 ? "CHECK CONDITION" : s);

		if(status == 2) {
			printf(", Sense Key: $%02X, ASC: $%02X, ASCQ: $%02X\n\n",
				senseData.senseKey, senseData.addSenseCode,
				senseData.addSenseCodeQualifier);
		}

		if(!expectChange) {
			return false;
		}

		if(senseData.senseKey != 0x06 || senseData.addSenseCode != 0x28) {
			printf("Media change was not reported\n");
			
			return false;
		}
		else {
			printf("Media change was reported correctly\n\n");
		}

		return true;
	}
	else if(expectChange) {
		if(senseData.senseKey != 0x06 || senseData.addSenseCode != 0x28) {
			printf("Media change was not reported\n");
			
			return false;
		}
	}

	printf("TEST UNIT READY succeeded\n\n");

	return true;
}


bool
Read(UWORD lun, ULONG blockSize)
{
	BYTE Read6[6] = { 0x08, 0, 0, 0, 1, 0 };
	BYTE Read10[10] = { 0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0 };

	LONG status;
	char buffer[2048];

	Read6[1] = lun << 5;
	cmd.Cmd = (void *)&Read6;
	cmd.CmdLen = (UWORD)sizeof(Read6);
	cmd.Buffer = &buffer;
	cmd.TransferLen = blockSize;

	status = scsiCall->In(&cmd);
	if(status) {
		Read10[1] = lun << 5;
		cmd.Cmd = (void *)&Read10;
		cmd.CmdLen = (UWORD)sizeof(Read10);
		status = scsiCall->In(&cmd);
	}

	if(status) {
		char s[20];

		sprintf(s, "%ld", status);

		printf("Reading root sector failed: %s",
			status == 2 ? "CHECK CONDITION" : s);

		if(status == 2) {
			printf(", Sense Key: $%02X, ASC: $%02X, ASCQ: $%02X\n",
				senseData.senseKey, senseData.addSenseCode,
				senseData.addSenseCodeQualifier);
		}

		return false;
	}

	printf("Reading root sector succeeded\n\n");

	return true;
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
