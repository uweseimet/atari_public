/****************************/
/* SCSI Driver Test 1.35    */
/*                          */
/* (C) 2014-2018 Uwe Seimet */
/****************************/


#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <tos.h>
#include <scsidrv/scsidefs.h>
#include "scsi3.h"


const char *DEVICE_TYPES[] = {
	"Direct Access",
	"Sequential Access",
	"Printer",
	"Processor",
	"Write-Once",
	"CD-ROM/DVD/BD/DVD-RAM",
	"Scanner",
	"Optical Memory",
	"Media Changer",
	"Communications",
	"Graphic Arts Pre-Press",
	"Graphic Arts Pre-Press",
	"Storage Array Controller",
	"Enclosure Services",
	"Simplified Direct Access",
	"Optical Card Reader/Writer",
	"Bridge Controller",
	"Object-based Storage",
	"Automation/Drive Interface",
	"Security Manager",
	"Host Managed Zoned Block",
	"Unknown Device Type",
	"Unknown Device Type",
	"Unknown Device Type",
	"Unknown Device Type",
	"Unknown Device Type",
	"Unknown Device Type",
	"Unknown Device Type",
	"Unknown Device Type",
	"Unknown Device Type",
	"Well Known Logical Unit"
};


typedef struct {
	UWORD busNo;
	UWORD id;
	ULONG maxLen;
	char busName[20];
	char deviceBusName[20];
} DEVICEINFO;


typedef struct {
	ULONG hi;
	ULONG lo;
} D_ULONG;


typedef int bool;
#define true TRUE
#define false FALSE


#define EUNDEV -15L
#define ENHNDL -35L
#define EACCDN -36L


UWORD findDevices(const char *);
void testCheckDev(UWORD, UWORD);
void testUnitReady(void);
UWORD testInquiry(void);
void testOpenClose(UWORD, UWORD, ULONG);
bool testError(UWORD, UWORD);
bool testRequestSense(void);
bool testReadCapacity(ULONG *);
bool testRead(ULONG, UBYTE *, UBYTE *, UBYTE *);
bool testReportLuns(void);
bool testSendDiagnostic(void);
bool checkRoot(UBYTE *, UBYTE *, ULONG);
void initBuffer(UBYTE *, ULONG);
char * DULongToString(const D_ULONG *);
void print(const char *, ...);
bool getCookie(LONG, ULONG *);


DEVICEINFO deviceInfos[32];
FILE *out;
bool hasError = false;
tpScsiCall scsiCall;
tSCSICmd cmd;
SENSE_DATA senseData;


int
main(WORD argc, const char *argv[])
{
	DLONG scsiId;
	ULONG maxLen;
	tHandle handle;
	LONG oldstack = 0;
	UWORD devCount;
	int i;

	getCookie('SCSI', (ULONG *)&scsiCall);
	if(!scsiCall) {
		printf("SCSI Driver not found\n");

		Cconin();

		return -1;
	}

	out = fopen("SDRVTEST.LOG", "w");
	if(!out) {
		printf("Couldn't open 'SDRVTEST.LOG' for writing logfile\n");

		Cconin();

		return -1;
	}

	print("SCSI Driver test V1.35\n");
	print("½ 2014-2018 Uwe Seimet\n\n");

	print("Found SCSI Driver version %d.%02d\n\n", scsiCall->Version >> 8,
		scsiCall->Version & 0xff);

	cmd.Flags = 0;
	cmd.SenseBuffer = (BYTE *)&senseData;
	cmd.Timeout = 5000;

	if(!Super((void *)1L)) {
		oldstack = Super(0L);
	}

	devCount = findDevices(argc > 1 ? argv[1] : NULL);

	for(i = 0; i < devCount; i++) {
		print("\nTesting bus '%s', device ID %d\n",
			deviceInfos[i].deviceBusName, deviceInfos[i].id);

		testCheckDev(deviceInfos[i].busNo, deviceInfos[i].id);
		testOpenClose(deviceInfos[i].busNo, deviceInfos[i].id, deviceInfos[i].maxLen);

		scsiId.hi = 0;
		scsiId.lo = deviceInfos[i].id;

		handle = (tHandle)scsiCall->Open(deviceInfos[i].busNo, &scsiId,
			&maxLen);
		if(((LONG)handle >> 24) == 0xff) {
			print("    ERROR: No handle\n");

			hasError = true;
		}
		else {
			UWORD deviceType;

			cmd.Handle = handle;

			testUnitReady();

			deviceType = testInquiry();

			hasError |= !testRequestSense();

			if(deviceType != 0x1f) {
				switch(deviceType) {
					case 0x00:
					case 0x05:
					case 0x07:
						{
							ULONG blockSize;

							hasError |= !testReadCapacity(&blockSize);
							if(blockSize) {
								UBYTE *ptr1, *ptr2, *ptr3;

								ptr1 = malloc(blockSize);
								ptr2 = malloc(blockSize);
								ptr3 = malloc(blockSize + 1);

								if(!ptr1 || !ptr2 || !ptr3) {
									print("    Not enough memory\n");

									return -1;
								}

								hasError |= !testRead(blockSize, ptr1, ptr2, ptr3 + 1);

								free(ptr3);
								free(ptr2);
								free(ptr1);
							}
						}
						break;

					default:
						break;
				}

/* This only works if no logfile is written to the tested drives */
/*				hasError |= !testError(deviceInfos[i].busNo, deviceInfos[i].id);*/

				hasError |= !testReportLuns();
/*				hasError |= !testSendDiagnostic(); */
			}

			scsiCall->Close(handle);
		}

		if(hasError) {
			print("ERROR\n");

			hasError = false;
		}
		else {
			print("OK\n");
		}
	}

	if(oldstack) {
		Super((void *)oldstack);
	}

	fclose(out);

	Cconin();

	return 0;
}


UWORD
findDevices(const char *busNos)
{
	tBusInfo busInfo;
	LONG busResult;
	UWORD devCount = 0;

	print("Buses:\n");

	busResult = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!busResult) {
		tDevInfo devInfo;
		LONG result;

		if(busNos) {
			char *tmp = strdup(busNos);

			bool hasBusNoMatch = false;

			char *busNo = strtok(tmp, ",");
			while(busNo && !hasBusNoMatch) {
				hasBusNoMatch = atoi(busNo) == busInfo.BusNo;

				busNo = strtok(NULL, ",");
			}

			free(tmp);

			if(!hasBusNoMatch) {
				busResult = scsiCall->InquireSCSI(cInqNext, &busInfo);

				continue;
			}
		}

		result = scsiCall->InquireBus(cInqFirst, busInfo.BusNo, &devInfo);
		while(!result) {
			char deviceBusName[20];
			UWORD features;

			scsiCall->CheckDev(busInfo.BusNo, &devInfo.SCSIId,
				deviceBusName, &features);

			deviceInfos[devCount].busNo = busInfo.BusNo;
			deviceInfos[devCount].id = (UWORD)devInfo.SCSIId.lo;
			deviceInfos[devCount].maxLen = busInfo.MaxLen;
			strcpy(deviceInfos[devCount].busName, busInfo.BusName);	
			strcpy(deviceInfos[devCount].deviceBusName, deviceBusName);	
			devCount++;

			result = scsiCall->InquireBus(cInqNext, busInfo.BusNo, &devInfo);
		}

		print("  ID: %d, Name: '%s', Maximum transfer length: %lu ($%lX)\n",
			busInfo.BusNo, busInfo.BusName, busInfo.MaxLen, busInfo.MaxLen);

		busResult = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	return devCount;
}


void
testCheckDev(UWORD busNo, UWORD id)
{
	DLONG scsiId;
	LONG result;
	char name[20];
	UWORD features;

	print("  CheckDev()\n");

	print("    Checking with illegal bus ID\n");

	scsiId.hi = 0;
	scsiId.lo = 0;
	result = scsiCall->CheckDev(32, &scsiId, name, &features);
	if(result != EUNDEV) {
		print("    ERROR: Illegal bus ID 32 was accepted\n");

		hasError = true;
	}
				
	print("    Checking with legal bus ID\n");

	scsiId.hi = 0;
	scsiId.lo = id;
	result = scsiCall->CheckDev(busNo, &scsiId, name, &features);
	if(result < 0) {
		print("    ERROR: Legal bus ID was rejected with error code %ld\n", result);

		hasError = true;
	}
}


void
testUnitReady()
{
	BYTE TestUnitReady[] = { 0x00, 0, 0, 0, 0, 0 };

	LONG status;

	print("  TEST UNIT READY\n");

	cmd.Cmd = (void *)&TestUnitReady;
	cmd.CmdLen = 6;
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		if(status == 2 && senseData.senseKey == 0x02 &&
			senseData.addSenseCode == 0x3a) {
			print("    Medium not present\n");
		}
		else {
			print("    ERROR: Call failed: %ld\n", status);
			print("      SenseKey $%02X, ASC $%02X\n",
				senseData.senseKey, senseData.addSenseCode);
		}
	}
}


UWORD
testInquiry()
{
	SENSE_BLK Inquiry = {
		0x12, 0x00, 0x00, 0x00, 0x00, (UBYTE)sizeof(INQUIRY_DATA), 0x00, 0x00, 0x00
	};

	LONG status;
	UWORD deviceType;
	INQUIRY_DATA inquiryData;
	char name[25];
	char revision[5];

	print("  INQUIRY\n");


	print("    Calling with legal data\n");

	cmd.Cmd = (void *)&Inquiry;
	cmd.CmdLen = 6;
	cmd.Buffer = &inquiryData;
	cmd.TransferLen = Inquiry.length;

	memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("    ERROR: Call failed: %ld\n", status);

		return 0;
	}

	deviceType = inquiryData.deviceType;
	if(inquiryData.deviceType == 0x1f) {
		print("    ERROR: Illegal device type\n");

		return 0;
	}

	strncpy(name, inquiryData.vendor, 24);
	strncpy(revision, inquiryData.revision, 4);
	name[24] = 0;
	revision[4] = 0;
	print("      Device type: %s\n",
		DEVICE_TYPES[inquiryData.deviceType & 0x1f]);
	print("      Device name: '%s'\n", name);
	print("      Firmware revision: '%s'\n", revision);

	print("      SCSI/SPC version: ");
	switch(inquiryData.ANSIVersion) {
		case 0:
			print("-");
			break;

		case 1:
			print("SCSI-1 CCS");
			break;

		case 2:
			print("SCSI-2");
			break;

		case 3:
			print("SPC");
			break;

		default:
			{
				char version[6];
				sprintf(version, "SPC-%d", inquiryData.ANSIVersion - 2);
				print(version);
				break;
			}
	}
	print("\n");

	print("      Additional length: $%02X\n", inquiryData.additionalLength);
	if(inquiryData.additionalLength < 0x1f) {
		print("      ERROR: Additional length must be at least $1F\n");

		hasError = true;
	}

	print("    Calling with non-existing LUN 7\n");

	Inquiry.lun = 7;

	memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("    ERROR: Call failed: %ld\n", status);

		hasError = true;
	}
	else if(inquiryData.peripheralQualifier != 0x03 ||
		inquiryData.deviceType != 0x1f) {
		print("    ERROR: Call was not properly rejected\n");

		hasError = true;
	}

	Inquiry.lun = 0;

	return deviceType;
}


bool
testRequestSense()
{
	BYTE RequestSense[] = { 0x03, 0, 0, 0, sizeof(SENSE_DATA), 0 };
	SENSE_DATA localSenseData;

	LONG status;

	print("  REQUEST SENSE\n");


	print("    Calling REQUEST SENSE for existing LUN 0\n");

	cmd.Cmd = (void *)&RequestSense;
	cmd.CmdLen = (UWORD)sizeof(RequestSense);
	cmd.Buffer = &localSenseData;
	cmd.TransferLen = sizeof(SENSE_DATA);

	status = scsiCall->In(&cmd);
	if(status) {
		print("    ERROR: Call failed: %ld\n", status);

		hasError = true;
	}
	else {
		print("      Additional sense length: $%02X\n",
			localSenseData.addSenseLength);
		if(localSenseData.addSenseLength < 0x0a) {
			print("      ERROR: Additional sense length must be at least $0A\n");

			hasError = true;
		}
	}


	print("    Calling REQUEST SENSE for non-existing LUN 7\n");

	RequestSense[1] = 7 << 5;

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("    ERROR: Call failed: %ld\n", status);

		hasError = true;
	}

	/* Even though GOOD is returned Sense Key and ASC must be set */
	if(localSenseData.senseKey != 0x05 ||
		localSenseData.addSenseCode != 0x25) {
		print("    ERROR: Call was not properly rejected\n");
		print("      Expected: SenseKey $05 ($%02X), ASC $25 ($%02X)\n",
			localSenseData.senseKey, localSenseData.addSenseCode);

		hasError = true;
	}

	RequestSense[1] = 0;


	print("    Calling REQUEST SENSE again for existing LUN 0\n");

	cmd.Cmd = (void *)&RequestSense;
	cmd.CmdLen = (UWORD)sizeof(RequestSense);
	cmd.Buffer = &localSenseData;
	cmd.TransferLen = sizeof(SENSE_DATA);

	status = scsiCall->In(&cmd);
	if(status) {
		print("    ERROR: Call failed: %ld\n", status);

		hasError = true;
	}

	return true;
}


void
testOpenClose(UWORD busNo, UWORD id, ULONG maxLen)
{
	DLONG scsiId;
	tHandle handle[256];
	ULONG len;
	LONG result;
	int i;

	print("  Open/Close()\n");

	scsiId.hi = 0;
	scsiId.lo = id;

	for(i = 0; i < 256; i++) {
		handle[i] = (tHandle)scsiCall->Open(busNo, &scsiId, &len);

		if(len != maxLen) {
			print("    ERROR: Transfer length mismatch: $%lX\n", len);

			hasError = true;

			Cconin();

			break;
		}

		result = (LONG)handle[i];

		switch((WORD)result) {
			case ENHNDL:
				break;

			case EUNDEV:
				print("    Handle %d: EUNDEV\n", i);
				break;

			case EACCDN:
				print("    Handle %d: EACCDN\n", i);
				break;

			default:
				break;
		}

		if(result < 0) {
			print("    Available handles: %d\n", i);

			break;
		}
	}

	if(i == 256) {
		print("    At least 256 handles are supported\n");
	}

	while(--i >= 0) {
		result = scsiCall->Close(handle[i]);
		if(result != 0) {
			print("    ERROR: Couldn't close handle %d\n", i);

			hasError = true;
		}
	}
}


bool
testError(UWORD busNo, UWORD id)
{
	DLONG scsiId;
	tHandle handle1, handle2;
	LONG status1, status2;
	ULONG maxLen;

	print("  Error()\n");

	scsiId.hi = 0;
	scsiId.lo = id;

	handle1 = (tHandle)scsiCall->Open(busNo, &scsiId, &maxLen);
	if(handle1 <= 0) {
		goto error;
	}

	handle2 = (tHandle)scsiCall->Open(busNo, &scsiId, &maxLen);
	if(handle2 <= 0) {
		goto error;
	}

	status1 = scsiCall->Error(handle1, cErrRead, 1);
	if(status1 != 0) {
		print("    ERROR: cErrMediach status 1 before change: %ld\n", status1);

		goto error;
	}

	status2 = scsiCall->Error(handle2, cErrRead, 1);
	if(status2 != 0) {
		print("    ERROR: cErrMediach status 2 before change: %ld\n", status2);

		goto error;
	}

	status1 = scsiCall->Error(handle1, cErrWrite, 1);
	if(status1 != 0) {
		print("    ERROR: cErrMediach status 1 after change: %ld\n", status1);

		goto error;
	}

	status2 = scsiCall->Error(handle2, cErrRead, 1);
	if(status2 != 1) {
		print("    ERROR: cErrMediach status 2 after change, first call: %ld\n", status2);

		goto error;
	}

	status2 = scsiCall->Error(handle2, cErrRead, 1);
	if(status2 != 0) {
		print("    ERROR: cErrMediach status 2 after change, second call: %ld\n", status2);

		goto error;
	}

	scsiCall->Close(handle1);
	scsiCall->Close(handle2);

	return true;
 
error:
	scsiCall->Close(handle1);
	scsiCall->Close(handle2);

	return false;
}


bool
testReadCapacity(ULONG *blockSize)
{
	BYTE ReadCapacity10[] = {
		0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	BYTE ReadCapacity16[] = {
		0x9e, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0
	};
	BYTE Read10[] = {
		0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0
	};
	BYTE Read16[] = {
		0x88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0
	};

	LONG status;
	ULONG capacity10[] = { 0, 0 };
	ULONG capacity16[] = { 0, 0, 0, 0 };
	D_ULONG maxBlock64;
	D_ULONG capacity64;
	WORD buffer[2048];
	char maxBlockString[20];
	char capacityString[20];

	*blockSize = 512;

	if(!(*cmd.Handle & cAllCmds)) {
		return true;
	}

	*blockSize = 0;


	print("  READ CAPACITY\n");


	print("    Reading capacity with READ CAPACITY (10)\n");

	cmd.Cmd = (void *)&ReadCapacity10;
	cmd.CmdLen = (UWORD)sizeof(ReadCapacity10);
	cmd.Buffer = capacity10;
	cmd.TransferLen = sizeof(capacity10);

	status = scsiCall->In(&cmd);
	if(status == 2 && senseData.senseKey == 0x02 &&
		senseData.addSenseCode == 0x3a) {
		print("    Medium not present\n");

		return true;
	}
	else if(status) {
		print("    ERROR: Call failed: %ld\n", status);

		return false;
	}

	maxBlock64.hi = 0;
	maxBlock64.lo = capacity10[0];
	capacity64.hi = 0;
	capacity64.lo = maxBlock64.lo + 1;

	if(!maxBlock64.lo) {
		print("    ERROR: Wrong maximum block number '0'\n");

		return false;
	}

	*blockSize = capacity10[1];
	sprintf(maxBlockString, "%lu", maxBlock64.lo);
	sprintf(capacityString, "%lu", capacity64.lo);

	print("      Maximum block number: %s\n", maxBlockString);
	print("      Block size: %lu\n", *blockSize);


	print("    Reading capacity with READ CAPACITY (16)\n");

	cmd.Cmd = (void *)&ReadCapacity16;
	cmd.CmdLen = (UWORD)sizeof(ReadCapacity16);
	cmd.Buffer = capacity16;
	cmd.TransferLen = sizeof(capacity16);

	status = scsiCall->In(&cmd);
	if(status) {
		print("      READ CAPACITY (16) is not supported\n");
	}
	else {
		UWORD ratio;

		maxBlock64.hi = capacity16[0];
		maxBlock64.lo = capacity16[1];
		capacity64.hi = maxBlock64.hi;
		capacity64.lo = maxBlock64.lo;
		capacity64.lo++;
		if(!capacity64.lo) {
			capacity64.hi++;
		}

		ratio = (UWORD)(capacity16[3] >> 16) & 0x0f;

		if(!maxBlock64.hi && !maxBlock64.lo) {
			print("      ERROR: Illegal maximum block number '0'\n");

			return false;
		}

		sprintf(maxBlockString, "%s", DULongToString(&maxBlock64));
		sprintf(capacityString, "%s", DULongToString(&capacity64));

		print("      Maximum block number hi: %lu, maximum block number lo: %lu\n",
			maxBlock64.hi, maxBlock64.lo);
		print("      Maximum block number: %s\n", maxBlockString);
		print("      Block size: %lu\n", capacity16[2]);
		if(!ratio) {
			print("      Logical sectors per physical sector: Unknown\n");
		}
		else {
			print("      Logical sectors per physical sector: %d\n",
				1 << ratio);
		}
	}


	print("    Reading last block (%s)\n", maxBlockString);

	if(!maxBlock64.hi) {
		cmd.Cmd = (void *)&Read10;
		cmd.CmdLen = (UWORD)sizeof(Read10);
		cmd.Buffer = &buffer;
		cmd.TransferLen = *blockSize;

		Read10[2] = (maxBlock64.lo >> 24) & 0xff;
		Read10[3] = (maxBlock64.lo >> 16) & 0xff;
		Read10[4] = (maxBlock64.lo >> 8) & 0xff;
		Read10[5] = maxBlock64.lo & 0xff;
	}
	else {
		cmd.Cmd = (void *)&Read16;
		cmd.CmdLen = (UWORD)sizeof(Read16);
		cmd.Buffer = &buffer;
		cmd.TransferLen = *blockSize;

		Read16[2] = (maxBlock64.hi >> 24) & 0xff;
		Read16[3] = (maxBlock64.hi >> 16) & 0xff;
		Read16[4] = (maxBlock64.hi >> 8) & 0xff;
		Read16[5] = maxBlock64.hi & 0xff;
		Read16[6] = (maxBlock64.lo >> 24) & 0xff;
		Read16[7] = (maxBlock64.lo >> 16) & 0xff;
		Read16[8] = (maxBlock64.lo >> 8) & 0xff;
		Read16[9] = maxBlock64.lo & 0xff;
	}

	status = scsiCall->In(&cmd);
	if(status) {
		print("    ERROR: Call failed: %ld\n", status);
		print("      SenseKey $%02X, ASC $%02X\n",
			senseData.senseKey, senseData.addSenseCode);

		return false;
	}


	print("    Trying to read last block + 1 (%s)\n", capacityString);

	if(!capacity64.hi) {
		cmd.Cmd = (void *)&Read10;
		cmd.CmdLen = (UWORD)sizeof(Read10);
		cmd.Buffer = &buffer;
		cmd.TransferLen = capacity10[1];

		Read10[2] = (capacity64.lo >> 24) & 0xff;
		Read10[3] = (capacity64.lo >> 16) & 0xff;
		Read10[4] = (capacity64.lo >> 8) & 0xff;
		Read10[5] = capacity64.lo & 0xff;
	}
	else {
		cmd.Cmd = (void *)&Read16;
		cmd.CmdLen = (UWORD)sizeof(Read16);
		cmd.Buffer = &buffer;
		cmd.TransferLen = *blockSize;

		Read16[2] = (capacity64.hi >> 24) & 0xff;
		Read16[3] = (capacity64.hi >> 16) & 0xff;
		Read16[4] = (capacity64.hi >> 8) & 0xff;
		Read16[5] = capacity64.hi & 0xff;
		Read16[6] = (capacity64.lo >> 24) & 0xff;
		Read16[7] = (capacity64.lo >> 16) & 0xff;
		Read16[8] = (capacity64.lo >> 8) & 0xff;
		Read16[9] = capacity64.lo & 0xff;
	}

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status != 2 || senseData.senseKey != 0x05 ||
		senseData.addSenseCode != 0x21) {
		print("    ERROR: Call was not properly rejected\n");
		print("      Expected: SenseKey $05 ($%02X), ASC $21 ($%02X)\n",
			senseData.senseKey, senseData.addSenseCode);

		return false;
	}

	return true;
}


bool
testRead(ULONG blockSize, UBYTE *ptr1, UBYTE* ptr2, UBYTE* ptr3)
{
	BYTE Read6[] = { 0x08, 0, 0, 0, 1, 0 };
	BYTE Read10[] = { 0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
	BYTE Read12[] = { 0xa8, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 };
	BYTE Read16[] = { 0x88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 };

	UBYTE *ptrRoot = NULL;
	LONG status;
	bool hasRW6 = true;
	UWORD i;


	print("  READ\n");


	print("    Reading block 0 with READ (6)\n");

	cmd.Cmd = (void *)&Read6;
	cmd.CmdLen = (UWORD)sizeof(Read6);
	cmd.Buffer = ptr1;
	cmd.TransferLen = blockSize;
	initBuffer(ptr1, blockSize);

	status = scsiCall->In(&cmd);
	if(status) {
		print("      READ (6) is not supported\n");

		hasRW6 = false;
	}
	else {
		ptrRoot = ptr1;
	}

	if(*cmd.Handle & cAllCmds) {
		print("    Reading block 0 with READ (10)\n");

		cmd.Cmd = (void *)&Read10;
		cmd.CmdLen = (UWORD)sizeof(Read10);
		cmd.Buffer = ptr2;
		cmd.TransferLen = blockSize;
		initBuffer(ptr2, blockSize);

		status = scsiCall->In(&cmd);
		if(status) {
			print("    ERROR: Call failed: %ld\n", status);

			return false;
		}

		if(ptrRoot) {
			hasError = checkRoot(ptrRoot, ptr2, blockSize);
		}
		else {
			ptrRoot = ptr2;
		}


		print("    Reading block 0 with READ (12)\n");

		cmd.Cmd = (void *)&Read12;
		cmd.CmdLen = (UWORD)sizeof(Read12);
		cmd.Buffer = ptr3;
		cmd.TransferLen = blockSize;
		initBuffer(ptr3, blockSize);

		status = scsiCall->In(&cmd);
		if(status) {
			print("      READ (12) is not supported\n");
		}
		else {
			hasError = checkRoot(ptrRoot, ptr3, blockSize);
		}


		print("    Reading block 0 with READ (16)\n");

		cmd.Cmd = (void *)&Read16;
		cmd.CmdLen = (UWORD)sizeof(Read16);
		cmd.Buffer = ptr3;
		cmd.TransferLen = blockSize;
		initBuffer(ptr3, blockSize);

		status = scsiCall->In(&cmd);
		if(status) {
			print("      READ (16) is not supported\n");
		}
		else {
			hasError = checkRoot(ptrRoot, ptr3, blockSize);

			/* This is beyond the IDE LBA48 limit */
			print("    Trying to read block 281474976710656 with READ (16)\n");

			cmd.Cmd = (void *)&Read16;
			cmd.CmdLen = (UWORD)sizeof(Read16);
			cmd.Buffer = ptr3;
			cmd.TransferLen = blockSize;
			initBuffer(ptr3, blockSize);

			Read16[3] = 1;

			status = scsiCall->In(&cmd);
			if(!status) {
				print("    ERROR: Block 281474976710656 is not supposed to be readable\n");

				hasError = true;
			}

			Read16[3] = 0;
		}
	}


	print("    Reading block 0 to odd address with ");

	if(hasRW6) {
		cmd.Cmd = (void *)&Read6;
		cmd.CmdLen = (UWORD)sizeof(Read6);

		print("READ (6)\n");
	}
	else if(*cmd.Handle & cAllCmds) {
		cmd.Cmd = (void *)&Read10;
		cmd.CmdLen = (UWORD)sizeof(Read10);

		print("READ (10)\n");
	}

	cmd.Buffer = ptr3;
	cmd.TransferLen = blockSize;
	initBuffer(ptr3, blockSize);

	status = scsiCall->In(&cmd);
	if(status) {
		print("    ERROR: Call failed: %ld\n", status);

		hasError = true;
	}

	for(i = 0; i < blockSize; i++) {
		if(ptrRoot[i] != ptr3[i]) {
			break;
		}
	}

	if(i != blockSize) {
		print("    ERROR: Block contents differ at offset %d\n", i);

		hasError = true;
	}


	print("    Reading block 0 from non-existing LUN 7\n");

	Read6[1] = 7 << 5;
	Read10[1] = 7 << 5;

	if(hasRW6) {
		cmd.Cmd = (void *)&Read6;
		cmd.CmdLen = (UWORD)sizeof(Read6);
	}
	else if(*cmd.Handle & cAllCmds) {
		cmd.Cmd = (void *)&Read10;
		cmd.CmdLen = (UWORD)sizeof(Read10);
	}

	cmd.Buffer = ptr3;
	cmd.TransferLen = blockSize;
	initBuffer(ptr3, blockSize);

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status != 2 || senseData.senseKey != 0x05 ||
		senseData.addSenseCode != 0x25) {
		print("    ERROR: Call was not properly rejected\n");
		print("      Expected: SenseKey $05 ($%02X), ASC $25 ($%02X)\n",
			senseData.senseKey, senseData.addSenseCode);

		return false;
	}

	return true;
}


bool
testReportLuns()
{
	BYTE ReportLuns[] = { 0xa0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 0, 0 };

	LONG status;
	ULONG buffer[32];

	if(!(*cmd.Handle & cAllCmds)) {
		return true;
	}

	print("  REPORT LUNS\n");


	cmd.Cmd = (void *)&ReportLuns;
	cmd.CmdLen = (UWORD)sizeof(ReportLuns);
	cmd.Buffer = &buffer;
	cmd.TransferLen = sizeof(buffer);

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(!status) {
		ULONG i;

		print("    Number of LUNs: %lu\n", buffer[0] / 8);

		print("      List of LUNs: ");

		for(i = 0; i < buffer[0] / 8 && i < sizeof(buffer) - 8; i++) {
			if(i) {
				print(", ");
			}

			print("%lu", buffer[2 + i]);
		}

		print("\n");
	}
	else if(status == 2 && senseData.senseKey == 0x05 &&
		senseData.addSenseCode == 0x20) {
		print("    REPORT LUNS is not supported\n");
	}
	else if(status == 2 && senseData.senseKey == 0x02 &&
		senseData.addSenseCode == 0x3a) {
		print("    No medium inserted\n");
	}
	else {
		print("    ERROR: Call was not properly rejected\n");
		print("      Expected: Sense Key $05 ($%02X), ASC $20 ($%02X)\n",
			senseData.senseKey, senseData.addSenseCode);

		return false;
	}
	
	return true;
}


bool
testSendDiagnostic()
{
	BYTE SendDiagnostic[6] = { 0x1d, 0x04, 0, 0, 0, 0 };

	LONG status;

	print("  SEND DIAGNOSTIC\n");


	print("    Default self test\n");

	cmd.Cmd = (void *)&SendDiagnostic;
	cmd.CmdLen = (UWORD)sizeof(SendDiagnostic);
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status == 2 && senseData.senseKey == 0x05 &&
		senseData.addSenseCode == 0x20) {
		print("      SEND DIAGNOSTIC is not supported\n");
	}
	else if(status) {
		print("      ERROR: Call was not properly rejected\n");
		print("        Expected: Sense Key $05 ($%02X), ASC $20 ($%02X)\n",
			senseData.senseKey, senseData.addSenseCode);

		return false;
	}

	return true;
}


bool
checkRoot(UBYTE *root, UBYTE *buffer, ULONG blockSize)
{
	int i;

	for(i = 0; i < blockSize; i++) {
		if(root[i] != buffer[i]) {
			break;
		}
	}

	if(i != blockSize) {
		print("    ERROR: Block contents differ at offset %d\n", i);

		return true;
	}

	return false;
}


void
initBuffer(UBYTE *buffer, ULONG count)
{
	ULONG i;

	for(i = 0; i < count; i++) {
		buffer[i] = i & 0xff;
	}
}


char *
DULongToString(const D_ULONG *value)
{
	static char result[21];

	if(value->hi) {
		double v = 4294967296.0 * value->hi + value->lo;

		sprintf(result, "%.0lf", v + 0.1);
	}
	else {
		sprintf(result, "%lu", value->lo);
	}

	return result;
}


void
print(const char *msg, ...)
{
	va_list args;
	char s[161];

	va_start(args, msg);
	vsprintf(s, msg, args);
	va_end(args);
	printf(s);
	fprintf(out, s);
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
