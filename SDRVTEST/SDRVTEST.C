/**********************************/
/* SCSI Driver/Firmware Test 2.61 */
/*                                */
/* (C) 2014-2025 Uwe Seimet       */
/**********************************/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tos.h>
#include <scsidrv/scsidefs.h>


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

const char* STATUS_CODES = {
	"Direct Access"
};


typedef struct {
	UWORD peripheralQualifier : 3;
	UWORD deviceType : 5;
	UWORD RMB : 1;
	UWORD deviceTypeModifier : 7;
	UWORD ISOVersion : 2;
	UWORD ECMAVersion : 3;
	UWORD ANSIVersion : 3;
	UWORD AENC : 1;
	UWORD TrmIOP : 1;
	UWORD : 2;
	UWORD responseDataFormat : 4;
	UBYTE additionalLength;
	UBYTE res;
	UWORD : 8;
	UWORD RelAdr : 1;
	UWORD WBus32 : 1;
	UWORD WBus16 : 1;
	UWORD Sync : 1;
	UWORD Linked : 1;
	UWORD : 1;
	UWORD CmdQue : 1;
	UWORD SftRe : 1;
	char vendor[8];
	char product[16];
	char revision[4];
} INQUIRY_DATA;	

typedef struct {
	UWORD valid : 1;
	UWORD errorClass : 3;
	UWORD errorCode : 4;
	UWORD segmentNumber : 8;
	UWORD fileMark : 1;
	UWORD EOM : 1;
	UWORD ILI : 1;
	UWORD : 1;
	UWORD senseKey : 4;
	UWORD information1 : 8;
	UWORD information2 : 8;
	UWORD information3 : 8;
	UWORD information4 : 8;
	UWORD addSenseLength : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD addSenseCode : 8;
	UWORD addSenseCodeQualifier : 8;
	UWORD fieldReplUnitCode : 8;
	UWORD sksv : 1;
	UWORD senseKeySpecific0 : 7;
	UWORD senseKeySpecific1 : 8;
	UWORD senseKeySpecific2 : 8;
} SENSE_DATA;

typedef struct {
	UWORD opcode : 8;
	UWORD lun : 3;
	UWORD flags : 5;
	UWORD PC : 2;
	UWORD pagecode : 6;
	UWORD : 8;
	UWORD length : 8;
	UWORD vu : 1;
	UWORD : 5;
	UWORD flag : 1;
	UWORD link : 1;
} SENSE_BLK;

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
	UWORD features;
	char busName[20];
	char deviceBusName[20];
} DEVICEINFO;

typedef struct
{
	UBYTE reserved1[3];
	UBYTE length;
	ULONG currentBlocks;
	UBYTE descriptorType;
	BYTE reserved2[1];
	UWORD currentBlockSize;
	ULONG formattableBlocks;
	ULONG formattableBlockSize;
} CAPACITY_LIST;

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
void testSenseBuffer(void);
void testRequestSense(void);
void testReadCapacity(ULONG *);
void testRead(UWORD, ULONG, UBYTE *, UBYTE *, UBYTE *);
void testSeek(void);
void testReadLong(void);
void testModeSense(void);
void testReportLuns(void);
void testReadFormatCapacities(void);
void testGetConfiguration(void);
void printFeatures(UWORD);
void printPageHeader(UBYTE *, int, const char *, int);
void printPages(UBYTE *, int *, int, int);
void printPage1(UBYTE *, int);
void printPage2(UBYTE *, int);
void printPage3(UBYTE *, int);
void printPage4(UBYTE *, int);
void printPage5(UBYTE *, int);
void printPage7(UBYTE *, int);
void printPage8(UBYTE *, int);
void printPage10(UBYTE *, int);
void printPage12(UBYTE *, int);
void printPage15(UBYTE *, int);
void printPage16(UBYTE *, int);
void printPages17_20(UBYTE *, int, int);
void printPage0(UBYTE *, int, int);
void printRawData(UBYTE *, int, int, const char *);
void checkRoot(UBYTE *, UBYTE *, ULONG);
void initBuffer(UBYTE *, ULONG);
char * DULongToString(const D_ULONG *);
void print(const char *, ...);
void printError(LONG);
LONG printSenseData(void);
void printExpectedSenseData(SENSE_DATA *, UWORD, UWORD);
LONG execute(const char *, bool);
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

	print("SCSI Driver and firmware test V2.61\n");
	print("½ 2014-2025 Uwe Seimet\n\n");

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

		printFeatures(deviceInfos[i].features);

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

			testSenseBuffer();

			if(deviceType != 0x1f) {
				switch(deviceType) {
					case 0x00:
					case 0x05:
					case 0x07: {
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
							testReadFormatCapacities();
						}
						testGetConfiguration();
						break;
					}

					default:
						testModeSense();
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
	UBYTE busNos[32];
	UWORD devCount = 0;

/* Manually clearing the bus info must be equivalent to using cInqFirst */
	memset(&busInfo, 0, sizeof(busInfo));
	busResult = scsiCall->InquireSCSI(cInqNext, &busInfo);
	if(!busResult) {
		UWORD busNo = busInfo.BusNo;
		
		busResult = scsiCall->InquireSCSI(cInqFirst, &busInfo);
		if(busResult || busNo != busInfo.BusNo) {
			print("  ERROR: Inconsistent handling of cInqFirst/cInqNext\n\n");
		}
	}

	print("\nAvailable buses:\n");

	memset(busNos, 0, sizeof(busNos));

/* Deliberately initialize with non-zero data */
	memset(&busInfo, -1, sizeof(busInfo));

	busResult = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!busResult) {
		tDevInfo devInfo;
		LONG result;

		if(!(busInfo.Private.BusIds & (1L << busInfo.BusNo))) {
			print("  ERROR: Bus ID vector has not been updated for bus %d\n\n",
				busInfo.BusNo);
		}

		if(busNos[busInfo.BusNo]) {
				print("  ERROR: Duplicate bus number: %d\n\n", busInfo.BusNo);
				break;
		}

		if(!scsiCall->InquireBus(cInqFirst, 32, &devInfo)) {
				print("  ERROR: Invalid bus numer 32 was accepted\n\n");
		}

		busNos[busInfo.BusNo] = 1;

		result = scsiCall->InquireBus(cInqFirst, busInfo.BusNo, &devInfo);
		while(!result) {
			char deviceBusName[20];
			UWORD features;

			scsiCall->CheckDev(busInfo.BusNo, &devInfo.SCSIId,
				deviceBusName, &features);

			deviceInfos[devCount].busNo = busInfo.BusNo;
			deviceInfos[devCount].id = (UWORD)devInfo.SCSIId.lo;
			deviceInfos[devCount].maxLen = busInfo.MaxLen;
			deviceInfos[devCount].features = features;
			strcpy(deviceInfos[devCount].busName, busInfo.BusName);	
			strcpy(deviceInfos[devCount].deviceBusName, deviceBusName);	
			devCount++;

			result = scsiCall->InquireBus(cInqNext, busInfo.BusNo, &devInfo);
		}

		print("  ID: %d\n", busInfo.BusNo);
		print("  Name: '%s'\n", busInfo.BusName);
		print("  Maximum transfer length: %lu ($%lX)\n", busInfo.MaxLen,
			busInfo.MaxLen);

		printFeatures(busInfo.Features);

		print("\n");

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
	UBYTE TestUnitReady[] = { 0x00, 0, 0, 0, 0, 0 };

	LONG status;

	print("  TEST UNIT READY\n");

	cmd.Cmd = (void *)&TestUnitReady;
	cmd.CmdLen = 6;
/* There is no data transfer, i.e. an invalid address must not matter */
	cmd.Buffer = (void *)0xffffffffL;
	cmd.TransferLen = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		if(status == 2 && senseData.senseKey == 0x02 &&
			senseData.addSenseCode == 0x3a) {
			print("    Medium not present\n");
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


	print("    Calling with valid parameters\n");

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

	printRawData((UBYTE *)&inquiryData, 0, inquiryData.additionalLength + 5,
		"      ");

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
	print("      Device name: '%s'\n", name);
	print("      Firmware revision: '%s'\n", revision);

	print("      Removable media support: %s\n",
		inquiryData.RMB ? "Yes" : "No");
	print("      Linked command support: %s\n",
		inquiryData.Linked ? "Yes" : "No");
	print("      Relative addressing support: %s\n",
		inquiryData.RelAdr ? "Yes" : "No");
	print("      Tagged command queuing support: %s\n",
		inquiryData.CmdQue ? "Yes" : "No");
	print("      Synchronous data transfer support: %s\n",
		inquiryData.Sync ? "Yes" : "No");
	print("      16-bit wide data transfer support: %s\n",
		inquiryData.WBus16 ? "Yes" : "No");
	print("      32-bit wide data transfer support: %s\n",
		inquiryData.WBus32 ? "Yes" : "No");
	print("      RESET condition behavior: %s\n",
		inquiryData.SftRe ? "Soft RESET" : "Hard RESET");

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
			print("SCSI-3 (SPC)");
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
		UBYTE *data = (UBYTE *)&inquiryData;
		if(data[10] != 0x44 || data[11] != 0x44) {
			print("    ERROR: More than 10 requested bytes were returned\n");
		}
	}

	return deviceType;
}


void
testRequestSense()
{
	UBYTE RequestSense[] = { 0x03, 0, 0, 0, sizeof(SENSE_DATA), 0 };

	UBYTE buffer[sizeof(SENSE_DATA)];
	LONG status;

	print("  REQUEST SENSE\n");


	print("    Calling REQUEST SENSE for existing LUN 0\n");

	cmd.Cmd = (void *)&RequestSense;
	cmd.CmdLen = (UWORD)sizeof(RequestSense);
	cmd.Buffer = &buffer;
	cmd.TransferLen = sizeof(SENSE_DATA);

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);

	if(status) {
		print("      ERROR: Request failed with status %ld\n", status);
		if(!senseData.errorClass) {
			print("      Device uses SCSI-1 4 byte legacy sense data format\n");
		}
		else {
			print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
				senseData.senseKey, senseData.addSenseCode,
				senseData.addSenseCodeQualifier);
		}
	}
	else if(senseData.errorClass) {
		print("      Additional sense length: $%02X\n",
			senseData.addSenseLength);
		if(senseData.addSenseLength < 0x0a) {
			print("      ERROR: Additional sense length must be at least $0A\n");
		}
	}


/* Non-existing LUNs are a special case, assume that there is no LUN 7 */
	print("    Calling REQUEST SENSE for non-existing LUN 7\n");

	RequestSense[1] = 7 << 5;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("      ERROR: Request failed with status %ld\n", status);
		if(senseData.errorClass) {
			print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
				senseData.senseKey, senseData.addSenseCode,
				senseData.addSenseCodeQualifier);
		}
	}
	else if(senseData.errorClass) {
/* Even though GOOD was returned Sense Key and ASC must be set */
		if(senseData.senseKey != 0x05 || senseData.addSenseCode != 0x25) {
			printExpectedSenseData(&senseData, 0x05, 0x25);
		}
	}


	print("    Calling REQUEST SENSE again for existing LUN 0\n");

	RequestSense[1] = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		print("      ERROR: Request failed with status %ld\n", status);
		if(senseData.errorClass) {
			print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
				senseData.senseKey, senseData.addSenseCode,
				senseData.addSenseCodeQualifier);
		}
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
	UBYTE ReadCapacity10[] = {
		0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	UBYTE ReadCapacity16[] = {
		0x9e, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0
	};
	UBYTE Read10[] = {
		0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0
	};
	UBYTE Read16[] = {
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
		/* READ CAPACITY cannot be used */
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

	status = execute("      READ CAPACITY (16)", true);
	if(!status) {
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

	cmd.Buffer = &buffer;
	cmd.TransferLen = *blockSize;

	print("    Reading last block (%s)\n", maxBlockString);

	if(!maxBlock64.hi) {
		cmd.Cmd = (void *)&Read10;
		cmd.CmdLen = (UWORD)sizeof(Read10);

		Read10[2] = (maxBlock64.lo >> 24) & 0xff;
		Read10[3] = (maxBlock64.lo >> 16) & 0xff;
		Read10[4] = (maxBlock64.lo >> 8) & 0xff;
		Read10[5] = maxBlock64.lo & 0xff;
	}
	else {
		cmd.Cmd = (void *)&Read16;
		cmd.CmdLen = (UWORD)sizeof(Read16);

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

		Read10[2] = (capacity64.lo >> 24) & 0xff;
		Read10[3] = (capacity64.lo >> 16) & 0xff;
		Read10[4] = (capacity64.lo >> 8) & 0xff;
		Read10[5] = capacity64.lo & 0xff;
	}
	else {
		cmd.Cmd = (void *)&Read16;
		cmd.CmdLen = (UWORD)sizeof(Read16);

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
	UBYTE Read6[] = { 0x08, 0, 0, 0, 1, 0 };
	UBYTE Read10[] = { 0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
	UBYTE Read12[] = { 0xa8, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 };
	UBYTE Read16[] = { 0x88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 };

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

	status = execute("    READ (6)", true);
	if(status) {
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

		status = execute("      READ (12)", true);
		if(!status) {
			checkRoot(ptrRoot, ptr3, blockSize);
		}


		print("    Reading block 0 with READ (16)\n");

		cmd.Cmd = (void *)&Read16;
		cmd.CmdLen = (UWORD)sizeof(Read16);
		cmd.Buffer = ptr3;
		cmd.TransferLen = blockSize;
		initBuffer(ptr3, blockSize);

		status = execute("      READ (16)", true);
		if(!status) {
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
		print("      ERROR: Block data differ at offset %d\n", i);
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
	UBYTE Seek6[] = { 0x0b, 0, 0, 0, 0, 0 };
	UBYTE Seek10[] = { 0x2b, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


	print("  SEEK\n");


	print("    Seeking block 0 with SEEK (6)\n");

	cmd.Cmd = (void *)&Seek6;
	cmd.CmdLen = (UWORD)sizeof(Seek6);
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	if(execute("      SEEK (6)", true)) {
		return;
	}

	if(*cmd.Handle & cAllCmds) {
		print("    Seeking block 0 with SEEK (10)\n");

		cmd.Cmd = (void *)&Seek10;
		cmd.CmdLen = (UWORD)sizeof(Seek10);

		execute("      SEEK (10)", true);
	}
}


void
testReadLong()
{
	UBYTE ReadLong10[] = {
		0x3e, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	UBYTE ReadLong16[] = {
		0x9e, 0x11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	LONG status;
	UBYTE buffer[516];

	if(!(*cmd.Handle & cAllCmds)) {
		return;
	}

	print("  READ LONG\n");


	print("    Reading 0 bytes of sector 0 with READ LONG (10)\n");

	cmd.Cmd = (void *)&ReadLong10;
	cmd.CmdLen = (UWORD)sizeof(ReadLong10);
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	if(execute("      READ LONG (10)", true)) {
		return;
	}

	print("    Reading 512 bytes of sector 0 with READ LONG (10)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 512;
	ReadLong10[7] = 2;
	ReadLong10[8] = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		LONG size;
		print("      Request has been rejected\n");
		size = printSenseData();
		if(size) {
			print("      Available data size is %ld bytes\n", cmd.TransferLen - size);
		}
	}


	print("    Reading 516 bytes of sector 0 with READ LONG (10)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 516;
	ReadLong10[7] = 2;
	ReadLong10[8] = 4;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		LONG size;
		print("      Request has been rejected\n");
		size = printSenseData();
		if(size) {
			print("      Available data size is %ld bytes\n", cmd.TransferLen - size);
		}
	}


	print("    Reading 0 bytes of sector 0 with READ LONG (16)\n");

	cmd.Cmd = (void *)&ReadLong16;
	cmd.CmdLen = (UWORD)sizeof(ReadLong16);
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	if(execute("      READ LONG (16)", true)) {
		return;
	}

	print("    Reading 512 bytes of sector 0 with READ LONG (16)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 512;
	ReadLong16[12] = 2;
	ReadLong16[13] = 0;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		LONG size;
		print("      Request has been rejected\n");
		size = printSenseData();
		if(size) {
			print("      Available data size is %ld bytes\n", cmd.TransferLen - size);
		}
	}

	print("    Reading 516 bytes of sector 0 with READ LONG (16)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 516;
	ReadLong16[12] = 2;
	ReadLong16[13] = 4;

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = scsiCall->In(&cmd);
	if(status) {
		LONG size;
		print("      Request has been rejected\n");
		size = printSenseData();
		if(size) {
			print("      Available data size is %ld bytes\n", cmd.TransferLen - size);
		}
	}
}


void
testModeSense()
{
	UBYTE ModeSense6[] = {
		0x1a, 0x08, 0x3f, 0, 0xff, 0
	};
	UBYTE ModeSense10[] = {
		0x5a, 0x08, 0x3f, 0, 0, 0, 0, 0x10, 0x00, 0
	};

	UBYTE buffer[4096];
	int pageOffsets[64];
	int size;
	int i;

	for(i = 0; i < 64; i++) {
		pageOffsets[i] = -1;
	}

	print("  MODE SENSE\n");


	print("    Reading all mode pages with MODE SENSE (6)\n");

	cmd.Cmd = (void *)&ModeSense6;
	cmd.CmdLen = (UWORD)sizeof(ModeSense6);
	cmd.Buffer = buffer;
	cmd.TransferLen = 255;

	if(execute("      MODE SENSE (6)", true)) {
		return;
	}

	size = buffer[0] + 1;
	print("      Received %d data bytes\n", size);

	if(size > 4) {
		print("        Available pages list: ");

		i = 4;
		while(i < size) {
			int page = buffer[i] & 0x3f;

			if(i > 4) {
				print(", ");
			}
			print("%d", page);

			pageOffsets[page] = i;

			i += buffer[i + 1] + 2;
		}

		print("\n");
	}

	printPages(buffer, pageOffsets, 64, size);

	if(!(*cmd.Handle & cAllCmds)) {
		return;
	}


	print("    Reading all mode pages with MODE SENSE (10)\n");

	cmd.Cmd = (void *)&ModeSense10;
	cmd.CmdLen = (UWORD)sizeof(ModeSense10);
	cmd.TransferLen = 4096;

	if(execute("      MODE SENSE (10)", true)) {
		return;
	}

	size = (buffer[0] << 8) + buffer[1];
	print("      Received %d data bytes\n", size);


	if(size > 8) {
		print("        Available pages list: ");

		i = 8;
		while(i < size) {
			if(i > 8) {
				print(", ");
			}

			print("%d", buffer[i] & 0x3f);

			i += buffer[i + 1] + 2;
		}

		print("\n");
	}
}


void
testReportLuns()
{
	UBYTE ReportLuns[] = { 0xa0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0x08, 0, 0 };

	ULONG i;
	ULONG buffer[264];

	if(!(*cmd.Handle & cAllCmds)) {
		return;
	}


	memset(buffer, 0, sizeof(buffer));

	print("  REPORT LUNS\n");

	cmd.Cmd = (void *)&ReportLuns;
	cmd.CmdLen = (UWORD)sizeof(ReportLuns);
	cmd.Buffer = &buffer;
	cmd.TransferLen = sizeof(buffer);

	memset(&senseData, 0, sizeof(SENSE_DATA));

	if(execute("    REPORT LUNS", true)) {
		return;
	}

	print("    Number of LUNs: %lu\n", buffer[0] / 8);

	if(buffer[0] / 8) {
		print("          LUN list: ");

		for(i = 0; i < buffer[0] / 8 && i < sizeof(buffer) - 8; i++) {
			if(i) {
				print(", ");
			}

			print("%lu", buffer[2 * i + 3]);
		}

		print("\n");
	}
}


void
testReadFormatCapacities()
{
	UBYTE ReadFormatCapacities[10] = {
		0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 254, 0x00
	};

	LONG status;
	UBYTE capacityData[254];
	CAPACITY_LIST *capacityList = (CAPACITY_LIST *)capacityData;
	ULONG *formattableData = (ULONG *)&capacityData[12];

	if(!(*cmd.Handle & cAllCmds)) {
		return;
	}

	print("  READ FORMAT CAPACITIES\n");


	cmd.Cmd = (void *)&ReadFormatCapacities;
	cmd.CmdLen = (UWORD)sizeof(ReadFormatCapacities);
	cmd.Buffer = &capacityData;
	cmd.TransferLen = sizeof(capacityData);

	memset(&senseData, 0, sizeof(SENSE_DATA));

	status = execute("    READ FORMAT CAPACITIES", true);
	if(!status) {
		int i;

		int length = capacityList->length;
		if(length < 8) {
			print("    ERROR: Invalid format capacities list length: %d\n", length);
			return;
		}

		if(length > 254) {
			length = 254;
		}
		length -= 8;

		print("    Current blocks: %ld\n", capacityList->currentBlocks);
		print("    Current block size: %d\n", capacityList->currentBlockSize);
		print("    Descriptor type: $%02X\n", capacityList->descriptorType);

		for(i = 0; i < length / 8; i++) {
			print("    Formattable capacity descriptor %d:\n", i + 1);
			print("      Formattable blocks: %ld\n", formattableData[2 * i] &
				0x03ffffffL);
			print("      Formattable block size: %ld\n",
				formattableData[2 * i + 1]);
			print("      Format type: $%02X\n", formattableData[2 * i] >> 30);
		}
	}
}


void
testGetConfiguration()
{
	UBYTE GetConfiguration[] = { 0x46, 2, 0, 0, 0, 0, 0, 0, 254, 0 };

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

	status = execute("    GET CONFIGURATION", true);
	if(!status) {
		UWORD i;
		UWORD profileListLen = profileData[11] >> 2;
		UWORD *profiles = (UWORD *)&profileData[12];

		print("     Number of supported profiles: %u\n", profileListLen);

		print("                  Current profile: $%04X\n",
			((UWORD *)&profileData[6])[0]);

		if(profileListLen) {
			print("           Supported profile list: ");

			for(i = 0; i < profileListLen; i++) {
				if(i) {
					print(", ");
				}

				print("$%04X", profiles[2 * i]);
			}

			print("\n");
		}
	}
}


/*
The following method tests the behavior of the SCSI Driver when the
sense buffer pointer is NULL. For this test a SCSI command that
definitely results in an error has to be sent. It is assumed that
MODE SENSE with a subpage code of 0xff (see SPC-5 specification) is such
a command.
If the device does not report an error for this command the respective
SCSI Driver test is not executed.
*/
void
testSenseBuffer()
{
	UBYTE ModeSense6[] = {
		0x1a, 0x08, 0x3f, 0, 0xff, 0
	};

	LONG status;
	UBYTE buffer[256];

	ModeSense6[3] = 0xff;
	cmd.Cmd = (void *)&ModeSense6;
	cmd.CmdLen = (UWORD)sizeof(ModeSense6);
	cmd.Buffer = buffer;
	cmd.TransferLen = 255;

	status = scsiCall->In(&cmd);
	if(!status || senseData.senseKey != 0x05 ||	senseData.addSenseCode != 0x20) {
		/* The command has not been rejected and cannot be used for this test */
		return;
	}

	print("    Testing SCSI Driver sense buffer handling\n");

	cmd.SenseBuffer = NULL;

	if(scsiCall->In(&cmd) != status) {
		print("      ERROR: Status code mismatch\n");
	}
	else {
		UBYTE RequestSense[] = { 0x03, 0, 0, 0, sizeof(SENSE_DATA), 0 };
		SENSE_DATA localSenseData;

		cmd.Cmd = (void *)&RequestSense;
		cmd.CmdLen = (UWORD)sizeof(RequestSense);
		cmd.Buffer = (BYTE *)&localSenseData;
		cmd.TransferLen = sizeof(SENSE_DATA);
		cmd.SenseBuffer = NULL;

		memset(&localSenseData, 0, sizeof(SENSE_DATA));

		status = scsiCall->In(&cmd);
		if(status) {
			print("      ERROR: Request failed with status %ld\n", status);
			print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
				localSenseData.senseKey, localSenseData.addSenseCode,
				localSenseData.addSenseCodeQualifier);
		}
		else {
			if(localSenseData.senseKey != senseData.senseKey ||
				localSenseData.addSenseCode != senseData.addSenseCode) {
				print("      ERROR: Sense data have not been preserved\n");
			}
		}
	}

	cmd.SenseBuffer = (BYTE *)&senseData;
}


void
printFeatures(UWORD features)
{
	bool hasFeature = false;

	print("  Supported SCSI Driver features:\n");

	if(features & cArbit) {
		print("    Arbitration\n");
		hasFeature = true;
	}

	if(features & cAllCmds) {
		print("    All SCSI commands\n");
		hasFeature = true;
	}

	if(features & cTargCtrl) {
		print("    Target control\n");
		hasFeature = true;
	}

	if(features & cTarget) {
		print("    Target installation\n");
		hasFeature = true;
	}

	if(features & cCanDisconnect) {
		print("    Disconnects\n");
		hasFeature = true;
	}

	if(features & cScatterGather) {
		print("    Scatter gather\n");
		hasFeature = true;
	}

/* SCSI Driver extension supported since HDDRIVER 12: Support for 32 LUNs,
	 see https://www.hddriver.net/en/scsidriver_extension.html. */
	if(features & 0x40) {
		print("    32 LUNs\n");
		hasFeature = true;
	}

	if(!hasFeature) {
		print("    -\n");
	}
}


void
printPages(UBYTE *buf, int *pageOffsets, int offsets, int size)
{
	int i;

	for(i = 1; i < offsets; i++) {
		if(pageOffsets[i] != -1) {
			switch(i) {
				case 1:
					printPage1(buf, pageOffsets[i]);
					break;

				case 2:
					printPage2(buf, pageOffsets[i]);
					break;

				case 3:
					printPage3(buf, pageOffsets[i]);
					break;

				case 4:
					printPage4(buf, pageOffsets[i]);
					break;

				case 5:
					printPage5(buf, pageOffsets[i]);
					break;

				case 7:
					printPage7(buf, pageOffsets[i]);
					break;

				case 8:
					printPage8(buf, pageOffsets[i]);
					break;

				case 10:
					printPage10(buf, pageOffsets[i]);
					break;

				case 12:
					printPage12(buf, pageOffsets[i]);
					break;

				case 15:
					printPage15(buf, pageOffsets[i]);
					break;

				case 16:
					printPage16(buf, pageOffsets[i]);
					break;

				case 17:
				case 18:
				case 19:
				case 20:
					printPages17_20(buf, pageOffsets[i], i - 16);
					break;

				default:
					printPageHeader(buf, pageOffsets[i], "Unknown",
						buf[pageOffsets[i] + 1]);
					break;
			}
		}
	}

	if(pageOffsets[0] != -1) {
		printPage0(buf, pageOffsets[0], size - pageOffsets[0] - 1);
	}
}


void
printPageHeader(UBYTE *buf, int offset, const char *name, int expected)
{
	const int size = buf[offset + 1];

	print("        Page %d: %s page (current, %s)\n", buf[offset], name,
		buf[offset] & 0x80 ? "savable" : "not savable");

	printRawData(buf, offset, size + 2, "          ");

	if(size < expected) {
		print("          ERROR: Page size: %d bytes, which is less than the expected %d\n",
			size, expected);
	}
	else {
		print("          Page size: %d bytes\n", size);
	}
}


void
printPage1(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Read-write error recovery", 10);

	print("          Disable correction (DCR): %d\n",
		buf[offset + 2] & 0x01);
	print("          Disable transfer on error (DTE): %d\n",
		(buf[offset + 2] & 0x02) >> 1);
	print("          Post error (PER): %d\n",
		(buf[offset + 2] & 0x04) >> 2);
	print("          Enable early recovery (EER): %d\n",
		(buf[offset + 2] & 0x08) >> 3);
	print("          Read continuous (RC): %d\n",
		(buf[offset + 2] & 0x10) >> 4);
	print("          Transfer block (TB): %d\n",
		(buf[offset + 2] & 0x20) >> 5);
	print("          Automatic read reallocation (ARRE): %d\n",
		(buf[offset + 2] & 0x40) >> 6);
	print("          Automatic write reallocation (AWRE): %d\n",
		(buf[offset + 2] & 0x80) >> 7);
	print("          Read retry count: %d\n",
		buf[offset + 3]);
	print("          Correction span: %d\n",
		buf[offset + 4]);
	print("          Head offset count: %d\n",
		buf[offset + 5]);
	print("          Data strobe offset count: %d\n",
		buf[offset + 6]);
	print("          Write retry count: %d\n",
		buf[offset + 8]);
	print("          Recovery time limit: %d ms\n",
		(buf[offset + 10] << 8) + buf[offset + 11]);
}


void
printPage2(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Disconnect-reconnect", 14);

	print("          Buffer full ratio: %d\n",
		buf[offset + 2]);
	print("          Buffer empty ratio: %d\n",
		buf[offset + 3]);
	print("          Bus inactivity limit: %d\n",
		(buf[offset + 4] << 8) + buf[offset + 5]);
	print("          Disconnect time limit: %d * 100 us\n",
		(buf[offset + 6] << 8) + buf[offset + 7]);
	print("          Connect time limit: %d * 100 us\n",
		(buf[offset + 8] << 8) + buf[offset + 9]);
	print("          Maximum burst size: %d\n",
		(buf[offset + 10] << 8) + buf[offset + 11]);
	print("          Data transfer disconnect control (DTDC): %d\n",
		buf[offset + 12] & 0x03);
}


void
printPage3(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Format device", 22);

	print("          Tracks per zone: %d\n",
		(buf[offset + 2] << 8) + buf[offset + 3]);
	print("          Alternate sectors per zone: %d\n",
		(buf[offset + 4] << 8) + buf[offset + 5]);
	print("          Alternate tracks per zone: %d\n",
		(buf[offset + 6] << 8) + buf[offset + 7]);
	print("          Alternate tracks per logical unit: %d\n",
		(buf[offset + 8] << 8) + buf[offset + 9]);
	print("          Sectors per track: %d\n",
		(buf[offset + 10] << 8) + buf[offset + 11]);
	print("          Data bytes per physical sector: %d\n",
		(buf[offset + 12] << 8) + buf[offset + 13]);
	print("          Interleave: %d\n",
		(buf[offset + 14] << 8) + buf[offset + 15]);
	print("          Track skew factor: %d\n",
		(buf[offset + 16] << 8) + buf[offset + 17]);
	print("          Cylinder skew factor: %d\n",
		(buf[offset + 18] << 8) + buf[offset + 19]);
	print("          Soft sector formatting (SSEC): %d\n",
		(buf[offset + 20] & 0x80) >> 7);
	print("          Hard sector formatting (HSEC): %d\n", 
		(buf[offset + 20] & 0x40) >> 6);
	print("          Removable (RMB): %d\n", 
		(buf[offset + 20] & 0x20) >> 5);
	print("          Surface (SURF): %d\n", 
		(buf[offset + 20] & 0x10) >> 4);
}


void
printPage4(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Rigid disk drive geometry", 22);

	print("          Number of cylinders: %d\n",
		(buf[offset + 2] << 16) + (buf[offset + 3] << 8)
			+ buf[offset + 4]);
	print("          Number of heads: %d\n", buf[offset + 5]);
	print("          Starting cylinder-write precompensation: %d\n",
		(buf[offset + 6] << 16) + (buf[offset + 7] << 8)
			+ buf[offset + 8]);
	print("          Starting cylinder-reduced write current: %d\n",
		(buf[offset + 9] << 16) + (buf[offset + 10] << 8)
			+ buf[offset + 11]);
	print("          Drive step rate: %d\n",
		(buf[offset + 12] << 8) + buf[offset + 13]);
	print("          Landing zone cylinder %d\n",
		(buf[offset + 14] << 16) + (buf[offset + 15] << 8)
			+ buf[offset + 16]);
	print("          Rotational offset: %d\n", buf[offset + 18]);
	print("          Medium rotation rate: %d\n",
		(buf[offset + 20] << 8) + buf[offset + 21]);
}


void
printPage5(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Flexible disk", 30);

	print("            Transfer rate: %d\n",
		(buf[offset + 2] << 8) + buf[offset + 3]);
	print("            Number of heads: %d\n", buf[offset + 4]);
	print("            Sectors per track: %d\n", buf[offset + 5]);
	print("            Data bytes per sector: %d\n",
		(buf[offset + 6] << 8) + buf[offset + 7]);
	print("            Number of cylinders: %d\n",
		(buf[offset + 8] << 8) + buf[offset + 9]);
	print("            Starting cylinder-write precompensation: %d\n",
		(buf[offset + 10] << 8) + buf[offset + 11]);
	print("            Starting cylinder-reduced write-current: %d\n",
		(buf[offset + 12] << 8) + buf[offset + 13]);
	print("            Drive step rate: %d/100 us\n",
		(buf[offset + 14] << 8) + buf[offset + 15]);
	print("            Drive step pulse with: %d ms\n", buf[offset + 16]);
	print("            Head settle delay: %d/100 us\n",
		(buf[offset + 17] << 8) + buf[offset + 18]);
	print("            Motor on delay: %d/10 seconds\n", buf[offset + 19]);
	print("            Motor off delay: %d/10 seconds\n", buf[offset + 20]);
	print("            True ready (TRDY): %d\n",
		(buf[offset + 21] & 0x80) >> 7);
	print("            Start sector number (SSN): %d\n",
		(buf[offset + 21] & 0x40) >> 6);
	print("            Motor on (MO): %d\n",
		(buf[offset + 21] & 0x20) >> 5);
	print("            Step pulse per cylinder (SPC): %d\n",
		(buf[offset + 22] & 0x0f) >> 4);
	print("            Write compensation: %d\n", buf[offset + 23]);
	print("            Head load delay: %d\n", buf[offset + 24]);
	print("            Head unload delay: %d\n", buf[offset + 25]);
	print("            Medium rotation rate: %d per minute\n",
		(buf[offset + 28] << 8) + buf[offset + 29]);
}


void
printPage7(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Verify error recovery", 10);

	print("          Error recovery parameter (DCR): %d\n",
		buf[offset + 2] & 0x01);
	print("          Disable transfer on error (DTE): %d\n",
		(buf[offset + 2] & 0x02) >> 1);
	print("          Post error (PER): %d\n",
		(buf[offset + 2] & 0x04) >> 2);
	print("          Enable early recovery (EER): %d\n",
		(buf[offset + 2] & 0x08) >> 3);
	print("          Verify retry count: %d\n",
		buf[offset + 3]);
	print("          Verify correction span: %d\n",
		buf[offset + 4]);
	print("          Verify recovery time limit: %d ms\n",
		(buf[offset + 10] << 8) + buf[offset + 11]);
}


void
printPage8(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Caching", 10);

	print("          Read cache disable (RCD): %d\n",
		buf[offset + 2] & 0x01);
	print("          Write cache enable (WCE): %d\n",
		(buf[offset + 2] & 0x04) >> 2);
	print("          Multiplication factor (MF): %d\n",
		(buf[offset + 2] & 0x02) >> 1);
	print("          Write retention priority: %d\n",
		buf[offset + 3] & 0x0f);
	print("          Demand read retention priority: %d\n",
		(buf[offset + 3] & 0xf0) >> 4);
	print("          Disable pre-fetch transfer length: %d\n",
		(buf[offset + 4] << 8) + buf[offset + 5]);
	print("          Minimum pre-fetch: %d\n",
		(buf[offset + 6] << 8) + buf[offset + 7]);
	print("          Maxium pre-fetch: %d\n",
		(buf[offset + 8] << 8) + buf[offset + 9]);
	print("          Maximum pre-fetch ceiling: %d\n",
		(buf[offset + 10] << 8) + buf[offset + 11]);
}


void
printPage10(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Control mode", 6);

	print("          Report log exception condition (RLEC): %d\n",
		buf[offset + 2] & 0x01);
	print("          Disable queuing (DQue): %d\n",
		buf[offset + 3] & 0x01);
	print("          Queue error management (QErr): %d\n",
		(buf[offset + 3] & 0x02) >> 1);
	print("          Queue algorithm modifier: %d\n",
		(buf[offset + 3] & 0xf0) >> 4);
	print("          Error AEN permission (EAENP): %d\n",
		buf[offset + 4] & 0x01);
	print("          Unit attention AEN permission (UAAENP): %d\n",
		(buf[offset + 4] & 0x02) >> 1);
	print("          Ready AEN permission (RAENP): %d\n",
		(buf[offset + 4] & 0x04) >> 2);
	print("          Enable extended contingent allegiance (EECA): %d\n",
		(buf[offset + 4] & 0x80) >> 7);
}


void
printPage12(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Notch", 22);

	print("          Notched drive (ND): %d\n",
		(buf[offset + 2] & 0x80) >> 7);
	print("          Logical or physical notch (LPN): %d\n",
		(buf[offset + 2] & 0x40) >> 6);
	print("          Maximum number of notches: %d\n",
		(buf[offset + 4] << 8) + buf[offset + 5]);
	print("          Active notch: %u\n",
		(buf[offset + 6] << 8) + buf[offset + 7]);
	print("          Starting boundary: %u\n",
		(buf[offset + 8] << 24) + (buf[offset + 9] << 16) +
		(buf[offset + 10] << 8) + buf[offset + 11]);
	print("          Ending boundary: %u\n",
		(buf[offset + 12] << 24) + (buf[offset + 10] << 16) +
		(buf[offset + 14] << 8) + buf[offset + 15]);
	print("          Pages notched high: %u\n",
		(buf[offset + 16] << 24) + (buf[offset + 17] << 16) +
		(buf[offset + 18] << 8) + buf[offset + 19]);
	print("          Pages notched low: %u\n",
		(buf[offset + 20] << 24) + (buf[offset + 21] << 16) +
		(buf[offset + 22] << 8) + buf[offset + 23]);
}


void
printPage15(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Data compression", 14);

	print("          Data compression enable (DCE): %d\n",
		(buf[offset + 2] & 0x80) >> 7);
	print("          Data compression capable (DCC): %d\n",
		(buf[offset + 2] & 0x40) >> 6);
	print("          Data decompression enable (DDE): %d\n",
		(buf[offset + 3] & 0x80) >> 7);
	print("          Report exception on decompression (RED): %d\n",
		(buf[offset + 3] & 0x60) >> 5);
	print("          Compression algorithm: %u\n",
		(buf[offset + 4] << 24) + (buf[offset + 5] << 16) +
		(buf[offset + 6] << 8) + buf[offset + 7]);
	print("          Decompression algorithm: %u\n",
		(buf[offset + 8] << 24) + (buf[offset + 9] << 16) +
		(buf[offset + 10] << 8) + buf[offset + 11]);
}


void
printPage16(UBYTE *buf, int offset)
{
	printPageHeader(buf, offset, "Device configuration", 14);

	print("          Change active partition (CAP): %d\n",
		(buf[offset + 2] & 0x40) >> 6);
	print("          Change active format (CAF): %d\n",
		(buf[offset + 2] & 0x20) >> 5);
	print("          Active format: %d\n", buf[offset + 2] & 0x1f);
	print("          Active partition: %d\n", buf[offset + 3]);
	print("          Write buffer full ratio: %d\n", buf[offset + 4]);
	print("          Read buffer full ratio: %d\n", buf[offset + 5]);
	print("          Write delay time: %u\n",
		(buf[offset + 6] << 8) + buf[offset + 7]);
	print("          Data buffer recovery (DBR): %d\n",
		(buf[offset + 8] & 0x80) >> 7);
	print("          Block identifiers supported (BIS): %d\n",
		(buf[offset + 8] & 0x40) >> 6);
	print("          Report setmarks (RSmk): %d\n",
		(buf[offset + 8] & 0x20) >> 5);
	print("          Automatic velocity control (AVC): %d\n",
		(buf[offset + 8] & 0x10) >> 4);
	print("          Stop on consecutive filemarks (SOCF): %d\n",
		(buf[offset + 8] & 0x0c) >> 3);
	print("          Recover buffer order (RBO): %d\n",
		(buf[offset + 8] & 0x02) >> 1);
	print("          Report early-warning (REW): %d\n", buf[offset + 8] & 0x01);
	print("          Gap size: %u\n", buf[offset + 9]);

	print("          End-of-data defined (EOD defined): %d\n",
		(buf[offset + 10] & 0xe0) >> 5);
	print("          Enable EOD generation (EEG): %d\n",
		(buf[offset + 10] & 0x10) >> 4);
	print("          Synnchronize at early-warning (SEW): %d\n",
		(buf[offset + 10] & 0x08) >> 3);
	print("          Buffer size at early-warning: %u\n",
		(buf[offset + 11] << 16) + (buf[offset + 12] << 8) + buf[offset + 13]);
	print("          Select data compression algorithm: %d\n", buf[offset + 14]);
}


void
printPages17_20(UBYTE *buf, int offset, int index)
{
	char name[30];

	sprintf(name, "Medium partition(%d)", index);
	printPageHeader(buf, offset, name, 6);

	print("          Maximum additional partitions: %d\n", buf[offset + 2]);
	print("          Additional partitions defined: %d\n", buf[offset + 3]);
	print("          Fixed data partitions (FDP): %d\n",
		(buf[offset + 4] & 0x80) >> 7);
	print("          Select data partitions (SDP): %d\n",
		(buf[offset + 4] & 0x40) >> 6);
	print("          Initiator-defined partitions (IDP): %d\n",
		(buf[offset + 4] & 0x20) >> 5);
	print("          Partition size unit of measure (PSUM): %d\n",
		(buf[offset + 4] & 0x18) >> 3);
	print("          Medium format recognition: %d\n", buf[offset + 5]);

	if(buf[offset + 1] >= 10) {
		print("          Approximate partition size in PSUM units: %d\n",
			(buf[offset + 8] << 8) + buf[offset + 9]);
	}
}
		

void
printPage0(UBYTE *buf, int offset, int size)
{
	print("        Page 0: Vendor-specific page (current, %s)\n",
		buf[offset] & 0x80 ? "savable" : "not savable");

	printRawData(buf, offset, size + 1, "          ");
}


void
printRawData(UBYTE *buf, int offset, int length, const char *indent)
{
	int i;

	print("%sRaw data: ", indent);

	for(i = 0; i < length; i++) {
		if(i && !(i % 16)) {
			print("\n          %s", indent);
		}
		else if(i) {
			print(":");
		}
		print("%02x", buf[offset + i]);
	}

	print("\n");
}


void
checkRoot(UBYTE *root, UBYTE *buffer, ULONG blockSize)
{
	int i;

	for(i = 0; i < blockSize; i++) {
		if(root[i] != buffer[i]) {
			break;
		}
	}

	if(i != blockSize) {
		print("      ERROR: Block data differ at offset %d\n", i);
	}
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


LONG
printSenseData()
{
	if(senseData.errorClass) {
		print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
			senseData.senseKey, senseData.addSenseCode, senseData.addSenseCodeQualifier);
		
		if(senseData.valid) {
			const LONG information = (senseData.information1 << 24) |
				(senseData.information2 << 16) | (senseData.information3 << 8) |
				senseData.information4;
			print("      ILI: %d, Information: %ld\n", senseData.ILI, information);

			return information;
		}
	}

	return 0;
}


void
printExpectedSenseData(SENSE_DATA *senseData, UWORD senseKey, UWORD addSenseCode)
{
	print("      ERROR: Request was not correctly rejected\n");
	if(senseData->errorClass) {
		print("        Expected: Sense Key $%02X (got $%02X),"
			" ASC $%02X (got $%02X)\n",
			senseKey, senseData->senseKey, addSenseCode, senseData->addSenseCode);
	}
}

LONG
execute(const char *msg, bool reportError)
{
	LONG status = scsiCall->In(&cmd);
	if(status == 2 || status == 4) {
		if(senseData.errorClass && senseData.senseKey == 0x05 &&
			senseData.addSenseCode == 0x20) {
			print(msg);
			print(" is not supported by device\n");
		}
		else if(senseData.errorClass && senseData.senseKey == 0x02 &&
			senseData.addSenseCode == 0x3a) {
			printf("      Medium not present, test skipped\n");
		}			
		else if(reportError) {
			printError(status);
		}
	}
	else if(status) {
		print("      Device reported status code %ld\n", status);
	}

	return status;
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

	if(!cookiejar) {
		return false;
	}

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