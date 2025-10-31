/***********************************/
/* SCSI Driver/Firmware Test 2.70ž */
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


DEVICEINFO deviceInfos[32];


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

	print("SCSI Driver and firmware test V2.70ž\n");
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

							print("    Not enough memory\n");

							return false;
						}

						testRead(deviceInfo->busNo, blockSize, ptr1, ptr2, ptr3 + 1);

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

	return true;
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
