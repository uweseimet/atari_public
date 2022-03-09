/****************************/
/* SCSI Driver Test 1.60    */
/*                          */
/* (C) 2014-2022 Uwe Seimet */
/****************************/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
	"Reserved Device Type",
	"Reserved Device Type",
	"Reserved Device Type",
	"Reserved Device Type",
	"Reserved Device Type",
	"Reserved Device Type",
	"Reserved Device Type",
	"Reserved Device Type",
	"Reserved Device Type",
	"Well Known Logical Unit"
};


typedef struct {
  unsigned int bootpref;
  char reserved1[4];
  unsigned char language;
  unsigned char keyboard;
  unsigned char datetime;
  char separator;
  unsigned char bootdelay;
  char reserved2[3];
  unsigned int vmode;
  unsigned char scsiid;
  char reserved_for_us;
} NVM;


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


#define NVMSIZE 18
#define NVMSIZE_MILAN 224


#define EUNDEV -15L
#define ENHNDL -35L
#define EACCDN -36L


UWORD findDevices(void);
void testCheckDev(UWORD, UWORD);
void testUnitReady(void);
UWORD testInquiry(void);
void testOpenClose(UWORD, UWORD, ULONG);
void testRequestSense(void);
void testReadCapacity(ULONG *);
void testRead(UWORD, ULONG, UBYTE *, UBYTE *, UBYTE *);
void testSeek(void);
void testReadLong(void);
void testModeSense(void);
void testReportLuns(void);
void testGetConfiguration(void);
bool checkRoot(UBYTE *, UBYTE *, ULONG);
void initBuffer(UBYTE *, ULONG);
char * DULongToString(const D_ULONG *);
void print(const char *, ...);
void printError(LONG);
void printSenseData(void);
void printExpectedSenseData(SENSE_DATA *, UWORD, UWORD);
bool execute(const char *);
bool getCookie(LONG, ULONG *);
bool getNvm(NVM *nvm);

DEVICEINFO deviceInfos[32];
FILE *out;
tpScsiCall scsiCall;
tSCSICmd cmd;
SENSE_DATA senseData;


int
main()
{
	DLONG scsiId;
	ULONG maxLen;
	tHandle handle;
	LONG oldstack = 0;
	UWORD devCount;
	NVM nvm;
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

	print("SCSI Driver test V1.60\n");
	print("½ 2014-2022 Uwe Seimet\n\n");

	if(getNvm(&nvm)) {
		print("SCSI initiator ID in NVRAM is %d\n", nvm.scsiid & 0x07);
		print("SCSI initiator identification is %s ", nvm.scsiid & 0x80 ?
			"enabled" : "disabled");		
	}
	else {
		print("SCSI initiator ID is not available");
	}

	print("\n\nFound SCSI Driver version %d.%02d\n\n", scsiCall->Version >> 8,
		scsiCall->Version & 0xff);

	cmd.Flags = 0;
	cmd.SenseBuffer = (BYTE *)&senseData;
	cmd.Timeout = 5000;

	if(!Super((void *)1L)) {
		oldstack = Super(0L);
	}

	devCount = findDevices();

	for(i = 0; i < devCount; i++) {
		print("\nTesting device ID %d on bus %d '%s'\n",
			deviceInfos[i].id, deviceInfos[i].busNo, deviceInfos[i].deviceBusName);

		testCheckDev(deviceInfos[i].busNo, deviceInfos[i].id);
		testOpenClose(deviceInfos[i].busNo, deviceInfos[i].id, deviceInfos[i].maxLen);

		scsiId.hi = 0;
		scsiId.lo = deviceInfos[i].id;

		handle = (tHandle)scsiCall->Open(deviceInfos[i].busNo, &scsiId,
			&maxLen);
		if(((LONG)handle >> 24) == 0xff) {
			print("    ERROR: No handle\n");
		}
		else {
			UWORD deviceType;

			cmd.Handle = handle;

			testUnitReady();

			deviceType = testInquiry();

			testRequestSense();

			if(deviceType != 0x1f) {
				switch(deviceType) {
					case 0x00:
					case 0x05:
					case 0x07:
						{
							ULONG blockSize;

							testReadCapacity(&blockSize);
							if(blockSize) {
								UBYTE *ptr1, *ptr2, *ptr3;

								ptr1 = malloc(blockSize);
								ptr2 = malloc(blockSize);
								ptr3 = malloc(blockSize + 1);

								if(!ptr1 || !ptr2 || !ptr3) {
									scsiCall->Close(handle);

									if(oldstack) {
										Super((void *)oldstack);
									}

									print("    Not enough memory\n");

									fclose(out);

									return -1;
								}

								testRead(deviceInfos[i].busNo, blockSize, ptr1, ptr2, ptr3 + 1);

								free(ptr3);
								free(ptr2);
								free(ptr1);

								testSeek();
								testModeSense();
								testReadLong();
								testGetConfiguration();
							}
						}
						break;

					default:
						break;
				}

				testReportLuns();
			}

			scsiCall->Close(handle);
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
findDevices()
{
	tBusInfo busInfo;
	LONG busResult;
	UWORD devCount = 0;

	print("Available buses:\n");

	busResult = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!busResult) {
		tDevInfo devInfo;
		LONG result;

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


	print("    Testing with valid bus ID %d\n", busNo);

	scsiId.hi = 0;
	scsiId.lo = id;
	result = scsiCall->CheckDev(busNo, &scsiId, name, &features);
	if(result < 0) {
		print("    ERROR: Valid bus ID %d was rejected with error code %ld\n",
			busNo, result);
	}

	print("    Testing with invalid bus ID 32\n");

	scsiId.hi = 0;
	scsiId.lo = 0;
	result = scsiCall->CheckDev(32, &scsiId, name, &features);
	if(result != EUNDEV) {
		print("    ERROR: Invalid bus ID 32 was accepted\n");
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
			printError(status);
		}

		printSenseData();
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


	print("    Calling with valid data\n");

	cmd.Cmd = (void *)&Inquiry;
	cmd.CmdLen = 6;
	cmd.Buffer = &inquiryData;
	cmd.TransferLen = Inquiry.length;

	memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		printError(status);

		return 0;
	}

	deviceType = inquiryData.deviceType;
	if(inquiryData.deviceType == 0x1f) {
		print("    ERROR: Unknown or no device type\n");

		return 0;
	}

	strncpy(name, inquiryData.vendor, 24);
	strncpy(revision, inquiryData.revision, 4);
	name[24] = 0;
	revision[4] = 0;
	print("      Device type: %s\n",
		DEVICE_TYPES[inquiryData.deviceType & 0x1f]);
	print("      Removable media support: %s\n",
		inquiryData.RMB ? "Yes" : "No");
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

	print("      Response data format: ");
	switch(inquiryData.responseDataFormat) {
		case 0:
			print("SCSI-1");
			break;

		case 1:
			print("SCSI-1 CCS");
			break;

		case 2:
			print("SCSI-2");
			break;

		default:
			{
				char format[3];
				sprintf(format, "$%2X", inquiryData.responseDataFormat);
				print(format);
				break;
			}
	}
	print("\n");

	print("      Additional length: $%02X\n", inquiryData.additionalLength);
	if(inquiryData.additionalLength < 0x1f) {
		print("      ERROR: Additional length must be at least $1F\n");
	}

	print("    Calling with non-existing LUN 7\n");

	Inquiry.lun = 7;

	memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		printError(status);
	}
	else if(inquiryData.peripheralQualifier != 0x03 ||
		inquiryData.deviceType != 0x1f) {
		print("      ERROR: Request was not correctly rejected\n");
		print("        Expected: Peripheral Qualifier $03  (got $%02X),"
			" Device Type $1F (got $%02X)\n",
			inquiryData.peripheralQualifier, inquiryData.deviceType);
	}

	Inquiry.lun = 0;

	print("    Testing with requested byte count of 10\n");

	Inquiry.length = 10;
	cmd.TransferLen = Inquiry.length;

	memset(&inquiryData, 0x44, sizeof(INQUIRY_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		printError(status);
	}
	else {
		BYTE *data = (BYTE *)&inquiryData;
		if(data[10] != 0x44 || data[11] != 0x44) {
			print("    ERROR: More than 10 requested bytes were returned\n");
		}
	}

	return deviceType;
}


void
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

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("      ERROR: Request failed with status %ld\n", status);
		print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
			localSenseData.senseKey, localSenseData.addSenseCode,
			localSenseData.addSenseCodeQualifier);
	}
	else {
		print("      Additional sense length: $%02X\n",
			localSenseData.addSenseLength);
		if(localSenseData.addSenseLength < 0x0a) {
			print("      ERROR: Additional sense length must be at least $0A\n");
		}
	}


/* Non-existing LUNs are a special case, assume that there is no LUN 7 */
	print("    Calling REQUEST SENSE for non-existing LUN 7\n");

	RequestSense[1] = 7 << 5;

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("      ERROR: Request failed with status %ld\n", status);
		print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
			localSenseData.senseKey, localSenseData.addSenseCode,
			localSenseData.addSenseCodeQualifier);
	}
	else {
/* Even though GOOD was returned Sense Key and ASC must be set */
		if(localSenseData.senseKey != 0x05 || localSenseData.addSenseCode != 0x25) {
			printExpectedSenseData(&localSenseData, 0x05, 0x25);
		}
	}

	RequestSense[1] = 0;


	print("    Calling REQUEST SENSE again for existing LUN 0\n");

	cmd.Cmd = (void *)&RequestSense;
	cmd.CmdLen = (UWORD)sizeof(RequestSense);
	cmd.Buffer = &localSenseData;
	cmd.TransferLen = sizeof(SENSE_DATA);

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("      ERROR: Request failed with status %ld\n", status);
		print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
			localSenseData.senseKey, localSenseData.addSenseCode,
			localSenseData.addSenseCodeQualifier);
	}
}


void
testOpenClose(UWORD busNo, UWORD id, ULONG maxLen)
{
	DLONG scsiId;
	tHandle handles[256];
	tHandle handle;
	ULONG len;
	LONG result;
	int i;

	print("  Open/Close()\n");

	scsiId.hi = 0;
	scsiId.lo = id;

/* Determine the available handle count */
	for(i = 0; i < 256; i++) {
		handle = (tHandle)scsiCall->Open(busNo, &scsiId, &len);

		if(len != maxLen) {
			print("    ERROR: Transfer length mismatch: $%lX\n", len);

			break;
		}

		result = (LONG)handle;

		handles[i] = (tHandle)-1;
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
				handles[i] = handle;
				break;
		}

		if(result < 0) {
			if(i != 256) {
				print("    Available handles for bus %d: %d\n", busNo, i);
			}
			else {
				print("    At least 256 handles are supported for bus %d\n", busNo);
			}

			break;
		}
	}

	while(--i >= 0) {
		if(handles[i] != (tHandle)-1) { 		
			result = scsiCall->Close(handles[i]);
			if(result != 0) {
				print("    ERROR: Couldn't close handle %ld\n", handles[i]);
			}
		}
	}
}


void
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
		return;
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

		return;
	}
	else if(status) {
		printError(status);

		return;
	}

	maxBlock64.hi = 0;
	maxBlock64.lo = capacity10[0];
	capacity64.hi = 0;
	capacity64.lo = maxBlock64.lo + 1;

	if(!maxBlock64.lo) {
		print("    ERROR: Wrong maximum block number '0'\n");

		return;
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
		print("      READ CAPACITY (16) is not supported by device\n");
		printSenseData();
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

			return;
		}

		sprintf(maxBlockString, "%s", DULongToString(&maxBlock64));
		sprintf(capacityString, "%s", DULongToString(&capacity64));

		print("      Maximum block number hi: %lu, maximum block number lo: %lu\n",
			maxBlock64.hi, maxBlock64.lo);
		print("      Block size: %lu\n", capacity16[2]);
		if(!ratio) {
			print("      Logical sectors per physical sector: Unknown (1 or more)\n");
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
		printError(status);

		return;
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
		print("      ERROR: Request for last block + 1 was not rejected\n");
		printExpectedSenseData(&senseData, 0x05, 0x21);
	}
}


void
testRead(UWORD busNo, ULONG blockSize, UBYTE *ptr1, UBYTE* ptr2, UBYTE* ptr3)
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
		print("      READ (6) is not supported by device\n");
		printSenseData();

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
			printError(status);

			return;
		}

		if(ptrRoot) {
			checkRoot(ptrRoot, ptr2, blockSize);
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
			print("      READ (12) is not supported by device\n");
			printSenseData();
		}
		else {
			checkRoot(ptrRoot, ptr3, blockSize);
		}


		print("    Reading block 0 with READ (16)\n");

		cmd.Cmd = (void *)&Read16;
		cmd.CmdLen = (UWORD)sizeof(Read16);
		cmd.Buffer = ptr3;
		cmd.TransferLen = blockSize;
		initBuffer(ptr3, blockSize);

		status = scsiCall->In(&cmd);
		if(status) {
			print("      READ (16) is not supported by device\n");
			printSenseData();
		}
		else {
			checkRoot(ptrRoot, ptr3, blockSize);

			if(busNo == 2) {
				/* This is beyond the IDE LBA48 limit */
				print("    Trying to read IDE block 281474976710656 with READ (16)\n");

				cmd.Cmd = (void *)&Read16;
				cmd.CmdLen = (UWORD)sizeof(Read16);
				cmd.Buffer = ptr3;
				cmd.TransferLen = blockSize;
				initBuffer(ptr3, blockSize);

				Read16[3] = 1;

				status = scsiCall->In(&cmd);
				if(!status) {
					print("    ERROR: IDE block 281474976710656 is not supposed to be readable\n");

					Read16[3] = 0;
				}
			}
		}
	}


	print("    Reading block 0 to odd address with ");

	if(hasRW6) {
		cmd.Cmd = (void *)&Read6;
		cmd.CmdLen = (UWORD)sizeof(Read6);

		print("READ (6)");
	}
	else if(*cmd.Handle & cAllCmds) {
		cmd.Cmd = (void *)&Read10;
		cmd.CmdLen = (UWORD)sizeof(Read10);

		print("READ (10)");
	}
	print("\n");

	cmd.Buffer = ptr3;
	cmd.TransferLen = blockSize;
	initBuffer(ptr3, blockSize);

	status = scsiCall->In(&cmd);
	if(status) {
		printError(status);
	}

	for(i = 0; i < blockSize; i++) {
		if(ptrRoot[i] != ptr3[i]) {
			break;
		}
	}

	if(i != blockSize) {
		print("    ERROR: Block contents differ at offset %d\n", i);
	}


	print("    Trying to read block 0 from non-existing LUN 7\n");

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
		printExpectedSenseData(&senseData, 0x05, 0x25);
	}
}


void
testSeek()
{
	BYTE Seek6[] = { 0x0b, 0, 0, 0, 0, 0 };
	BYTE Seek10[] = { 0x2b, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


	print("  SEEK\n");


	print("    Seeking block 0 with SEEK (6)\n");

	cmd.Cmd = (void *)&Seek6;
	cmd.CmdLen = (UWORD)sizeof(Seek6);
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	if(!execute("      SEEK (6)")) {
		return;
	}

	if(*cmd.Handle & cAllCmds) {
		print("    Seeking block 0 with SEEK (10)\n");

		cmd.Cmd = (void *)&Seek10;
		cmd.CmdLen = (UWORD)sizeof(Seek10);

		execute("      SEEK (6)");
	}
}


void
testReadLong()
{
	BYTE ReadLong10[] = {
		0x3e, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	BYTE ReadLong16[] = {
		0x9e, 0x11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	LONG status;
	UBYTE buffer[512];

	if(!(*cmd.Handle & cAllCmds)) {
		return;
	}

	print("  READ LONG\n");


	print("    Reading 0 bytes for sector 0 with READ LONG (10)\n");

	cmd.Cmd = (void *)&ReadLong10;
	cmd.CmdLen = (UWORD)sizeof(ReadLong10);
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	if(!execute("      READ LONG (10)")) {
		return;
	}

	print("    Reading 1 byte for sector 0 with READ LONG (10)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 1;
	ReadLong10[8] = 1;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("      Request has been rejected\n");
	}

	print("    Reading 512 bytes for sector 0 with READ LONG (10)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 512;
	ReadLong10[7] = 2;
	ReadLong10[8] = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
			print("       Request has been rejected\n");
	}


	print("    Reading 0 bytes for sector 0 with READ LONG (16)\n");

	cmd.Cmd = (void *)&ReadLong16;
	cmd.CmdLen = (UWORD)sizeof(ReadLong16);
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	if(!execute("      READ LONG (16)")) {
		return;
	}

	print("    Reading 1 byte for sector 0 with READ LONG (16)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 1;
	ReadLong16[13] = 1;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("      Request has been rejected\n");
	}

	print("    Reading 512 bytes for sector 0 with READ LONG (16)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 512;
	ReadLong16[12] = 2;
	ReadLong16[13] = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("      Request has been rejected\n");
	}
}


void
testModeSense()
{
	BYTE ModeSense6[] = {
		0x1a, 0x08, 0x3f, 0, 0xff, 0
	};
	BYTE ModeSense10[] = {
		0x5a, 0x08, 0x3f, 0, 0, 0, 0, 0x10, 0x00, 0
	};

	UBYTE buf[4096];
	int size;
	int i;

	print("  MODE SENSE\n");


	print("    Reading all mode pages with MODE SENSE (6)\n");

	cmd.Cmd = (void *)&ModeSense6;
	cmd.CmdLen = (UWORD)sizeof(ModeSense6);
	cmd.Buffer = buf;
	cmd.TransferLen = 255;

	if(!execute("      MODE SENSE (6)")) {
		return;
	}

	size = buf[0];
	print("      Received %d data bytes\n", size);

	if(size > 4) {
		print("        Available pages list: ");

		i = 4;
		while(i < size) {
			if(i > 4) {
				print(", ");
			}

			print("%d", buf[i] & 0x3f);

			i += buf[i + 1] + 2;
		}

		print("\n");
	}

	if(!(*cmd.Handle & cAllCmds)) {
		return;
	}


	print("    Reading all mode pages with MODE SENSE (10)\n");

	cmd.Cmd = (void *)&ModeSense10;
	cmd.CmdLen = (UWORD)sizeof(ModeSense10);
	cmd.Buffer = buf;
	cmd.TransferLen = 4096;

	if(!execute("      MODE SENSE (10)")) {
		return;
	}

	size = (buf[0] << 8) + buf[1];
	print("      Received %d data bytes\n", size);


	if(size > 8) {
		print("        Available pages list: ");

		i = 8;
		while(i < size) {
			if(i > 8) {
				print(", ");
			}

			print("%d", buf[i] & 0x3f);

			i += buf[i + 1] + 2;
		}

		print("\n");
	}
}


void
testReportLuns()
{
	BYTE ReportLuns[] = { 0xa0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0x08, 0, 0 };

	ULONG i;
	ULONG buffer[264];

	if(!(*cmd.Handle & cAllCmds)) {
		return;
	}

	print("  REPORT LUNS\n");


	cmd.Cmd = (void *)&ReportLuns;
	cmd.CmdLen = (UWORD)sizeof(ReportLuns);
	cmd.Buffer = &buffer;
	cmd.TransferLen = sizeof(buffer);

	memset(&senseData, 0, sizeof(SENSE_DATA));

	if(!execute("    REPORT LUNS")) {
		return;
	}

	print("    Number of LUNs: %lu\n", buffer[0] / 8);

	print("          LUN list: ");

	for(i = 0; i < buffer[0] / 8 && i < sizeof(buffer) - 8; i++) {
		if(i) {
			print(", ");
		}

		print("%lu", buffer[2 * i + 3]);
	}

	print("\n");
}


void
testGetConfiguration()
{
	BYTE GetConfiguration[] = { 0x46, 2, 0, 0, 0, 0, 0, 0, 254, 0 };

	LONG status;
	UBYTE profileData[254];

	if(!(*cmd.Handle & cAllCmds)) {
		return;
	}

	print("  GET CONFIGURATION\n");


	cmd.Cmd = (void *)&GetConfiguration;
	cmd.CmdLen = (UWORD)sizeof(GetConfiguration);
	cmd.Buffer = &profileData;
	cmd.TransferLen = sizeof(profileData);

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(!status) {
		UWORD i;
		UWORD profileListLen = profileData[11] >> 2;
		UWORD *profiles = (UWORD *)&profileData[12];

		print("     Number of supported profiles: %u\n", profileListLen);

		print("                  Current profile: $%04X\n",
			((UWORD *)&profileData[6])[0]);

		print("           Supported profile list: ");

		for(i = 0; i < profileListLen; i++) {
			if(i) {
				print(", ");
			}

			print("$%04X", profiles[2 * i]);
		}

		print("\n");
	}
	else if(status == 2 && senseData.senseKey == 0x05 &&
		senseData.addSenseCode == 0x20) {
		print("    GET CONFIGURATION is not supported by device\n");
	}
	else if(status == 2 && senseData.senseKey == 0x02 &&
		senseData.addSenseCode == 0x3a) {
		print("    No medium inserted\n");
	}
	else {
		printExpectedSenseData(&senseData, 0x05, 0x20);
	}
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


void
printError(LONG status)
{
	print("      ERROR: Request failed with status %ld\n", status);
	printSenseData();
}


void
printSenseData()
{
		print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
			senseData.senseKey, senseData.addSenseCode, senseData.addSenseCodeQualifier);
}


void
printExpectedSenseData(SENSE_DATA *senseData, UWORD senseKey, UWORD addSenseCode)
{
		print("      ERROR: Request was not correctly rejected\n");
		print("        Expected: Sense Key $%02X (got $%02X),"
			" ASC $%02X (got $%02X)\n",
			senseKey, senseData->senseKey, addSenseCode, senseData->addSenseCode);
}

bool
execute(const char *msg)
{
	LONG status = scsiCall->In(&cmd);
	if(status == 2) {
		if(senseData.senseKey == 0x05 && senseData.addSenseCode == 0x20) {
			print(msg);
			print(" is not supported by device\n");
		}
		else {
			printError(status);
		}

		return false;
	}

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


bool
getNvm(NVM *nvm)
{
	ULONG cookie;

	if(getCookie('_MCH', &cookie) && cookie >= 0x00020000L) {
		int nvmSize = getCookie('_MIL', NULL) ? NVMSIZE_MILAN : NVMSIZE;

		return !NVMaccess(0, 0, nvmSize, nvm);
	}
	
	return false;
}