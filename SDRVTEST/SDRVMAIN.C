/***********************************/
/* SCSI Driver/Firmware Test 3.00ž */
/*                                 */
/* (C) 2014-2025 Uwe Seimet        */
/***********************************/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tos.h>
#include <scsidrv/scsidefs.h>
#include "sdrvtest.h"
#include "sdrvio.h"


#define NVMSIZE 18
#define NVMSIZE_MILAN 224


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


static DEVICEINFO deviceInfos[32];
static UWORD lunList[32];


bool testDevice(DEVICEINFO *);
UWORD findDevices(void);
bool getCookie(LONG, ULONG *);
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

	print("SCSI Driver and firmware test V3.00ž\n");
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
		if(!testDevice(&deviceInfos[i])) {
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
testDevice(DEVICEINFO *deviceInfo)
{
	DLONG scsiId;
	ULONG maxLen;
	tHandle handle;
	UWORD deviceType;
	UWORD luns;
	UWORD nonExistingLun = 0;
	int i;

	print("\nTesting device ID %d on bus %d '%s'\n",
		deviceInfo->id, deviceInfo->busNo, deviceInfo->deviceBusName);

	printFeatures(deviceInfo->features);

	testCheckDev(deviceInfo->busNo, deviceInfo->id);
	testOpenClose(deviceInfo->busNo, deviceInfo->id, deviceInfo->maxLen);

	scsiId.hi = 0;
	scsiId.lo = deviceInfo->id;

	handle = (tHandle)scsiCall->Open(deviceInfo->busNo, &scsiId,
		&maxLen);
	if(((LONG)handle & 0xff000000L) == 0xff000000L) {
		printDriverError(4, "No free SCSI Driver handle\n");
		return false;
	}

	cmd.Handle = handle;

	luns = testReportLuns(lunList);

	for(i = 1; i < 8; i++) {
		int j;

		for(j = 0; j < luns; j++) {
			if(lunList[j] != i) {
				nonExistingLun = i;
				break;
			}
		}

		if(nonExistingLun) {
			break;
		}
	}

	for(i = 0; i < luns; i++) {
		UWORD lun = lunList[i];

		print("Testing LUN %d\n", lun);

		testUnitReady(lun);
	
		deviceType = testInquiry(lun, nonExistingLun);
	
		testRequestSense(lun, nonExistingLun);
	
		testSenseBuffer(lun);
	
		if(deviceType != 0x1f) {
			switch(deviceType) {
				case 0x00:
				case 0x05:
				case 0x07: {
					ULONG blockSize;
	
					testReadCapacity(lun, &blockSize);
					if(blockSize) {
						UBYTE *ptr1, *ptr2, *ptr3;
	
						ptr1 = malloc(blockSize);
						ptr2 = malloc(blockSize);
						ptr3 = malloc(blockSize + 1);
	
						if(!ptr1 || !ptr2 || !ptr3) {
							scsiCall->Close(handle);
	
							print("    Not enough memory\n");
	
							return false;
						}
	
						testRead(lun, nonExistingLun, deviceInfo->busNo,
							blockSize, ptr1, ptr2, ptr3 + 1);
	
						free(ptr3);
						free(ptr2);
						free(ptr1);
	
						testSeek(lun);
						testModeSense(lun);
						testReadLong(lun);
						testReadFormatCapacities(lun);
					}
					testGetConfiguration(lun);
					break;
				}
	
				default:
					testModeSense(lun);
					break;
			}
		}
	}

	scsiCall->Close(handle);

	return true;
}


UWORD
findDevices()
{
	tBusInfo busInfo;
	LONG busResult;
	UBYTE busNos[32];
	UWORD busCount = 0;
	UWORD devCount = 0;

/* Manually clearing the bus info must be equivalent to using cInqFirst */
	memset(&busInfo, 0, sizeof(busInfo));
	busResult = scsiCall->InquireSCSI(cInqNext, &busInfo);
	if(!busResult) {
		UWORD busNo = busInfo.BusNo;
		
		busResult = scsiCall->InquireSCSI(cInqFirst, &busInfo);
		if(busResult || busNo != busInfo.BusNo) {
			printDriverError(2, "Inconsistent handling of cInqFirst/cInqNext\n\n");
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
		int i;
		int bitCount = 0;

		busCount++;

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

		result = scsiCall->InquireBus(cInqFirst, busInfo.BusNo, &devInfo);
		while(!result) {
			char deviceBusName[20];
			UWORD features;

			deviceBusName[0] = 0;
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
