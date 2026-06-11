/**********************************/
/* SCSI Driver/Firmware Test 3.05 */
/*                                */
/* (C) 2014-2026 Uwe Seimet       */
/**********************************/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tos.h>
#include <scsidrv/scsidefs.h>
#include "scsi3.h"
#include "std.h"
#include "util.h"
#include "sdrvtest.h"
#include "sdrvio.h"


#define NVMSIZE 18
#define NVMSIZE_MILAN 224

#define MAX_DEVICES 64


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


static DEVICEINFO deviceInfos[MAX_DEVICES];
SENSE_DATA localSenseData;
tpScsiCall scsiCall;


bool testDevice(UWORD, const char *, UWORD, ULONG);
UWORD findDevices(void);
int sortBuses(const void *, const void *);
bool getNvm(NVM *nvm);


int
main()
{
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

	print("SCSI Driver and device firmware test V3.05\n");
	print("½ 2014-2026 Uwe Seimet\n\n");

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
	cmd.SenseBuffer = (BYTE *)&localSenseData;
	cmd.Timeout = 5000;

	if(!Super((void *)1L)) {
		oldstack = Super(0L);
	}

	devCount = findDevices();

	for(i = 0; i < devCount && i < MAX_DEVICES; i++) {
		DEVICEINFO *deviceInfo = &deviceInfos[i];

		print("\nTesting bus %d '%s', device %d\n", deviceInfo->busNo,
			deviceInfo->busName, deviceInfo->id);
		printFeatures(deviceInfo->features, "device");

		if(!testDevice(deviceInfo->busNo, deviceInfo->busName,
			deviceInfo->id, deviceInfo->maxLen)) {
			break;
		}
	}

	if(oldstack) {
		Super((void *)oldstack);
	}

	fclose(out);

	Cconin();

	return 0;
}


bool
testDevice(UWORD busNo, const char *busName, UWORD id, ULONG maxLen)
{
	DLONG scsiId;
	tHandle handle;
	ULONG lunVector;
	UWORD lun;
	UWORD nonExistingLun = 0;
	int i;

	testCheckDev(busNo, id);
	testOpenClose(busNo, id, maxLen);

	scsiId.hi = 0;
	scsiId.lo = id;

	handle = (tHandle)scsiCall->Open(busNo, &scsiId, &maxLen);
	if(((LONG)handle & 0xff000000L) == 0xff000000L) {
		printDriverError(4, "No free SCSI Driver handle\n");
		return false;
	}

	cmd.Handle = handle;

	lunVector = testReportLuns();

	for(lun = 0; lun < 8; lun++) {
		if(lunVector & (1L << lun)) {
			break;
		}
	}

	for(i = 1; i < 8; i++) {
		if(!(lunVector & (1L << i))) {
			nonExistingLun = i;
			break;
		}
	}

	scsiDriverErrors = 0;
	deviceErrors = 0;

	print("\nTesting bus %d '%s', device %d, LUN %d\n", busNo, busName, id, lun);

	runTest(busNo, lun, nonExistingLun);

	scsiCall->Close(handle);

	print("\nTest result summary:\n"
		"SCSI Driver bugs: %d, "
		"SCSI compatibility bugs: %d\n",
		scsiDriverErrors, deviceErrors);

	if(scsiDriverErrors || deviceErrors) {
		print("Issues have been marked with 'ERROR'.\n");

		if(scsiDriverErrors) {
			print("For SCSI Driver issues contact the maintainers\n"
				"of the SCSI Driver for the respective bus.\n");
		}

		if(deviceErrors) {
			print("For device issues contact the maintainers\n"
				"of the device or the device emulation firmware.\n");
		}
	}

	return true;
}


UWORD
findDevices()
{
	tBusInfo busInfos[32];
	tBusInfo busInfo;
	tDevInfo devInfo;
	LONG result;
	UBYTE busNos[32];
	UWORD busCount = 0;
	UWORD devCount = 0;
	int i;

/* Manually clearing the bus info must be equivalent to using cInqFirst */
	memset(&busInfo, 0, sizeof(busInfo));
	result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	if(!result) {
		UWORD busNo = busInfo.BusNo;
		
		result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
		if(result || busNo != busInfo.BusNo) {
			printDriverError(2, "Inconsistent handling of cInqFirst/cInqNext\n\n");
		}
	}

	memset(busNos, 0, sizeof(busNos));

/* Deliberately initialize with non-zero data */
	memset(&busInfo, -1, sizeof(busInfo));

	result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result && busCount < 32) {
		int bitCount = 0;
		int i;

		memcpy(&busInfos[busCount++], &busInfo, sizeof(tBusInfo));

		if(!(busInfo.Private.BusIds & (1L << busInfo.BusNo))) {
			printDriverError(2, "Bus ID vector has not been updated for bus %d\n\n",
				busInfo.BusNo);
		}

		for(i = busInfo.BusNo; i < 32; i++) {
			if(busInfo.Private.BusIds & (1L << i)) {
				bitCount++;
			}
		}
		if(bitCount > busCount) {
			printDriverError(2, "Bus ID vector mismatch for bus %d\n\n",
				busInfo.BusNo);
		}

		if(busNos[busInfo.BusNo]) {
			printDriverError(2, "Duplicate bus number: %d\n\n", busInfo.BusNo);
			break;
		}

		if(!scsiCall->InquireBus(cInqFirst, 32, &devInfo)) {
			printDriverError(2, "Invalid bus numer 32 was accepted\n\n");
		}

		busNos[busInfo.BusNo] = 1;

		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	qsort(busInfos, busCount, sizeof(tBusInfo), sortBuses);

	print("\nAvailable buses:\n");

	for(i = 0; i < busCount; i++) {
		tBusInfo *info = &busInfos[i];

		print("  ID: %d\n", info->BusNo);
		print("  Name: '%s'\n", info->BusName);
		print("  Maximum transfer length: %lu ($%lX)\n", info->MaxLen,
			info->MaxLen);

		printFeatures(info->Features, "bus");

		if(!info->BusNo && info->Features & cAllCmds) {
			print("    WARNING: Only ICD compatible adapters/devices are supported\n");
		}

		print("\n");

		result = scsiCall->InquireBus(cInqFirst, info->BusNo, &devInfo);
		while(!result && devCount < MAX_DEVICES) {
			char deviceBusName[20];
			UWORD features;

			deviceBusName[0] = 0;
			scsiCall->CheckDev(info->BusNo, &devInfo.SCSIId,
				deviceBusName, &features);

			deviceInfos[devCount].busNo = info->BusNo;
			deviceInfos[devCount].id = (UWORD)devInfo.SCSIId.lo;
			deviceInfos[devCount].maxLen = info->MaxLen;
			deviceInfos[devCount].features = features;
			strcpy(deviceInfos[devCount].busName, info->BusName);	
			strcpy(deviceInfos[devCount].deviceBusName, deviceBusName);	
			devCount++;

			result = scsiCall->InquireBus(cInqNext, info->BusNo, &devInfo);
		}
	}

	return devCount;
}


int
sortBuses(const void *b1, const void *b2)
{
	const tBusInfo *i1 = b1;
	const tBusInfo *i2 = b2;

	return i1->BusNo - i2->BusNo;
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
