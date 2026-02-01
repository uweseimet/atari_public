/***********************************/
/* SCSI Driver/Firmware Test 3.00ž */
/*                                 */
/* (C) 2014-2026 Uwe Seimet        */
/***********************************/


#include <assert.h>
#include <atarierr.h>
#include <string.h>
#include <stdio.h>
#include <scsidrv/scsidefs.h>
#include "scsi3.h"
#include "std.h"
#include "sdrvtest.h"
#include "sdrvio.h"


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
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"Well Known Logical Unit"
};


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


tSCSICmd cmd;

UWORD scsiDriverErrors;
UWORD deviceErrors;


bool
runTest(UWORD busNo, UWORD lun, UWORD nonExistingLun)
{
	UWORD deviceType;

	testUnitReady(lun);

	deviceType = testInquiry(busNo, lun, nonExistingLun);

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
						print("    Not enough memory\n");

						return false;
					}

					testRead(lun, nonExistingLun, busNo, blockSize,
						ptr1, ptr2, ptr3 + 1);

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

	return true;
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
		printDriverError(4, "Valid bus ID %d was rejected with error code %ld\n",
			busNo, result);
	}

	print("    Testing with invalid bus ID 32\n");

	scsiId.hi = 0;
	scsiId.lo = 0;
	result = scsiCall->CheckDev(32, &scsiId, name, &features);
	if(!result) {
		printDriverError(4, "Invalid bus ID 32 was accepted\n");
	}
}


void
testUnitReady(UWORD lun)
{
	UBYTE TestUnitReady[] = { 0x00, 0, 0, 0, 0, 0 };

	LONG status;

	print("  TEST UNIT READY\n");

	cmd.Cmd = (void *)&TestUnitReady;
	cmd.CmdLen = 6;
/* There is no data transfer, i.e. an invalid address must not matter */
	cmd.Buffer = (void *)0xffffffffL;
	cmd.TransferLen = 0;

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = callInWithLun(&cmd, lun);
	if(status) {
		if(status == 2 && localSenseData.senseKey == 0x02 &&
			localSenseData.addSenseCode == 0x3a) {
			print("    Medium not present\n");
		}

		printSenseData();
	}
}


UWORD
testInquiry(UWORD busNo, UWORD lun, UWORD nonExistingLun)
{
	SENSE_BLK Inquiry = {
		0x12, 0x00, 0x00, 0x00, 0x00, (UBYTE)sizeof(INQUIRY_DATA), 0x00, 0x00, 0x00
	};

	LONG status;
	UWORD deviceType;
	INQUIRY_DATA inquiryData;
	INQUIRY_DATA fullInquiryData;
	char name[25];
	char revision[5];

	print("  INQUIRY\n");


	print("    Calling with valid parameters\n");

	cmd.Cmd = (void *)&Inquiry;
	cmd.CmdLen = 6;
	cmd.Buffer = &inquiryData;
	cmd.TransferLen = Inquiry.length;

	memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

	status = callInWithLun(&cmd, lun);
	if(status) {
		printStatus(status);

		return 0;
	}

	printRawData((UBYTE *)&inquiryData, 0, inquiryData.additionalLength + 5,
		"      ");

	deviceType = inquiryData.deviceType & 0x1f;
	if(deviceType == 0x1f) {
		printDeviceError(4, "Unknown or no device type\n");

		return 0;
	}

	memcpy(&fullInquiryData, &inquiryData, sizeof(INQUIRY_DATA));

	strncpy(name, inquiryData.vendor, 24);
	strncpy(revision, inquiryData.revision, 4);
	name[24] = 0;
	revision[4] = 0;
	print("      Device type: %s\n", DEVICE_TYPES[deviceType] ?
		DEVICE_TYPES[deviceType] : "Reserved Device Type");
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

	/* For ACSI and SCSI the device must support a well-defined SCSI/SPC version */
	if(!inquiryData.ANSIVersion && busNo < 2) {
		printDeviceError(6, "Invalid SCSI/SPC version\n");
	}

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
		printDeviceError(6, "Additional length must be at least $1F\n");
	}

	if(nonExistingLun) {
		print("    Calling with non-existing LUN %d\n", nonExistingLun);

		memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

		status = callInWithLun(&cmd, nonExistingLun);
		if(status) {
			printStatus(status);
		}
		else if(inquiryData.peripheralQualifier != 0x03 ||
			inquiryData.deviceType != 0x1f) {
			printDeviceError(6, "Request was not correctly rejected\n");
			print("        Expected: Peripheral Qualifier $03  (got $%02X),"
				" Device Type $1F (got $%02X)\n",
				inquiryData.peripheralQualifier, inquiryData.deviceType);
		}
	}


	print("    Testing with requested byte count of 10\n");

	Inquiry.length = 10;
	cmd.TransferLen = Inquiry.length;

	memset(&inquiryData, 0x44, sizeof(INQUIRY_DATA));

	status = callInWithLun(&cmd, lun);
	if(status) {
		printStatus(status);
	}
	else {
		UBYTE *data = (UBYTE *)&inquiryData;
		if(data[10] != 0x44 || data[10] != 0x44) {
			printDriverError(4, "More than 10 requested bytes were returned\n");
		}
	}

	if(memcmp(&fullInquiryData, &inquiryData, 10)) {
		printDriverError(4, "INQUIRY data mismatch\n");
	}


	print("    Testing with requested byte count of 0\n");

	Inquiry.length = 0;
	cmd.TransferLen = Inquiry.length;

	memset(&inquiryData, 0x44, sizeof(INQUIRY_DATA));

	status = callInWithLun(&cmd, lun);
	if(status) {
		printStatus(status);
	}
	else {
		UBYTE *data = (UBYTE *)&inquiryData;
		if(data[0] != 0x44 || data[1] != 0x44) {
			printDeviceError(4, "More than 0 requested bytes were returned\n");
		}
	}

	return deviceType;
}


void
testRequestSense(UWORD lun, UWORD nonExistingLun)
{
	UBYTE RequestSense[] = { 0x03, 0, 0, 0, sizeof(SENSE_DATA), 0 };

	UBYTE buffer[sizeof(SENSE_DATA)];
	LONG status;

	print("  REQUEST SENSE\n");


	print("    Calling REQUEST SENSE for existing LUN %d\n", lun);

	cmd.Cmd = (void *)&RequestSense;
	cmd.CmdLen = (UWORD)sizeof(RequestSense);
	cmd.Buffer = &buffer;
	cmd.TransferLen = sizeof(SENSE_DATA);

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = callInWithLun(&cmd, lun);

	if(status) {
		printDeviceError(6, "Request failed with status %ld\n", status);
		if(!localSenseData.errorClass) {
			print("      Device uses SCSI-1 4 byte legacy sense data format\n");
		}
		else {
			print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
				localSenseData.senseKey, localSenseData.addSenseCode,
				localSenseData.addSenseCodeQualifier);
		}
	}
	else if(localSenseData.errorClass) {
		print("      Additional sense length: $%02X\n",
			localSenseData.addSenseLength);
		if(localSenseData.addSenseLength < 0x0a) {
			printDeviceError(6, "Additional sense length must be at least $0A\n");
		}
	}

	if(nonExistingLun) {
		print("    Calling REQUEST SENSE for non-existing LUN %d\n", nonExistingLun);
	
		memset(&localSenseData, 0, sizeof(SENSE_DATA));
	
		status = callInWithLun(&cmd, nonExistingLun);
		if(status) {
			printDeviceError(6, "Request failed with status %ld\n", status);
			if(localSenseData.errorClass) {
				print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
					localSenseData.senseKey, localSenseData.addSenseCode,
					localSenseData.addSenseCodeQualifier);
			}
		}
		else if(localSenseData.errorClass) {
			/* Even though GOOD was returned Sense Key and ASC must be set */
			if(localSenseData.senseKey != 0x05 || localSenseData.addSenseCode != 0x25) {
				printExpectedSenseData(&localSenseData, 0x05, 0x25);
			}
		}
	
	
		print("    Calling REQUEST SENSE again for existing LUN %d\n", lun);
	
		memset(&localSenseData, 0, sizeof(SENSE_DATA));
	
		status = callInWithLun(&cmd, lun);
		if(status) {
			printDeviceError(6, "Request failed with status %ld\n", status);
			if(localSenseData.errorClass) {
				print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
					localSenseData.senseKey, localSenseData.addSenseCode,
					localSenseData.addSenseCodeQualifier);
			}
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
	bool twice = false;
	int i;

	print("  Open/Close()\n");

	scsiId.hi = 0;
	scsiId.lo = id;

/* Determine the available handle count */
	for(i = 0; i < 256; i++) {
		handle = (tHandle)scsiCall->Open(busNo, &scsiId, &len);

		if(len != maxLen) {
			printDeviceError(4, "Transfer length mismatch: $%lX\n", len);

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

	if(scsiCall->Close((tHandle)0xfffffffeL) == 0) {
		printDeviceError(4, "SCSU Driver: Invalid handles can be closed\n");
	}

	while(--i >= 0) {
		if(handles[i] != (tHandle)-1) { 		
			if(scsiCall->Close(handles[i]) != 0) {
				printDriverError(4, "Can't close handle %ld\n", handles[i]);
			}
			else if(scsiCall->Close(handles[i]) == 0) {
				twice = true;
			}
		}
	}

	if(twice) {
		printDriverError(4, "Handles can be closed more than once\n");
	}
}


void
testReadCapacity(UWORD lun, ULONG *blockSize)
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

	status = callInWithLun(&cmd, lun);
	if(status == 2 && localSenseData.senseKey == 0x02 &&
		localSenseData.addSenseCode == 0x3a) {
		print("    Medium not present\n");

		return;
	}
	else if(status) {
		printStatus(status);

		return;
	}

	maxBlock64.hi = 0;
	maxBlock64.lo = capacity10[0];
	capacity64.hi = 0;
	capacity64.lo = maxBlock64.lo + 1;

	if(!maxBlock64.lo) {
		printDeviceError(4, "Wrong maximum block number '0'\n");

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

	status = execute(lun, "      READ CAPACITY (16)", true);
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
			printDeviceError(6, "Illegal maximum block number '0'\n");

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

	status = callInWithLun(&cmd, lun);
	if(status) {
		printStatus(status);

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

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = callInWithLun(&cmd, lun);
	if(status != 2 || localSenseData.senseKey != 0x05 ||
		localSenseData.addSenseCode != 0x21) {
		printDeviceError(6, "Request for last block + 1 was not rejected\n");
		printExpectedSenseData(&localSenseData, 0x05, 0x21);
	}
}


void
testRead(UWORD lun, UWORD nonExistingLun, UWORD busNo, ULONG blockSize,
	UBYTE *ptr1, UBYTE* ptr2, UBYTE* ptr3)
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

	status = execute(lun, "      READ (6)", true);
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

		status = callInWithLun(&cmd, lun);
		if(status) {
			printStatus(status);

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

		status = execute(lun, "      READ (12)", true);
		if(!status) {
			checkRoot(ptrRoot, ptr3, blockSize);
		}


		print("    Reading block 0 with READ (16)\n");

		cmd.Cmd = (void *)&Read16;
		cmd.CmdLen = (UWORD)sizeof(Read16);
		cmd.Buffer = ptr3;
		cmd.TransferLen = blockSize;
		initBuffer(ptr3, blockSize);

		status = execute(lun, "      READ (16)", true);
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

				status = callInWithLun(&cmd, lun);
				if(!status) {
					printDeviceError(4, "IDE block 281474976710656 is not supposed to be readable\n");

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

	status = callInWithLun(&cmd, lun);
	if(status) {
		printStatus(status);
	}

	for(i = 0; i < blockSize; i++) {
		if(ptrRoot[i] != ptr3[i]) {
			break;
		}
	}

	if(i != blockSize) {
		printDeviceError(6, "Block data differ at offset %d\n", i);
	}


	if(nonExistingLun) {
		print("    Trying to read block 0 from non-existing LUN %d\n", nonExistingLun);
	
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
	
		memset(&localSenseData, 0, sizeof(SENSE_DATA));
	
		status = callInWithLun(&cmd, nonExistingLun);
		if(status != 2 || localSenseData.senseKey != 0x05 ||
			localSenseData.addSenseCode != 0x25) {
			printExpectedSenseData(&localSenseData, 0x05, 0x25);
		}
	}
}


void
testSeek(UWORD lun)
{
	UBYTE Seek6[] = { 0x0b, 0, 0, 0, 0, 0 };
	UBYTE Seek10[] = { 0x2b, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


	print("  SEEK\n");


	print("    Seeking block 0 with SEEK (6)\n");

	cmd.Cmd = (void *)&Seek6;
	cmd.CmdLen = (UWORD)sizeof(Seek6);
	cmd.Buffer = NULL;
	cmd.TransferLen = 0;

	if(execute(lun, "      SEEK (6)", true)) {
		return;
	}

	if(*cmd.Handle & cAllCmds) {
		print("    Seeking block 0 with SEEK (10)\n");

		cmd.Cmd = (void *)&Seek10;
		cmd.CmdLen = (UWORD)sizeof(Seek10);

		execute(lun, "      SEEK (10)", true);
	}
}


void
testReadLong(UWORD lun)
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

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	if(execute(lun, "      READ LONG (10)", true)) {
		return;
	}

	print("    Reading 512 bytes of sector 0 with READ LONG (10)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 512;
	ReadLong10[7] = 2;
	ReadLong10[8] = 0;

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = callInWithLun(&cmd, lun);
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

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = callInWithLun(&cmd, lun);
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

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	if(execute(lun, "      READ LONG (16)", true)) {
		return;
	}

	print("    Reading 512 bytes of sector 0 with READ LONG (16)\n");

	cmd.Buffer = &buffer;
	cmd.TransferLen = 512;
	ReadLong16[12] = 2;
	ReadLong16[13] = 0;

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = callInWithLun(&cmd, lun);
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

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = callInWithLun(&cmd, lun);
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
testModeSense(UWORD lun)
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

	if(execute(lun, "      MODE SENSE (6)", true)) {
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

	if(execute(lun, "      MODE SENSE (10)", true)) {
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


ULONG
testReportLuns()
{
	UBYTE ReportLuns[] = { 0xa0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0x08, 0, 0 };

	ULONG luns;
	ULONG lunVector = 0;
	ULONG i;
	ULONG buffer[264];

	if(!(*cmd.Handle & cAllCmds)) {
		return 0x01;
	}

	memset(buffer, 0, sizeof(buffer));

	print("  REPORT LUNS\n");

	cmd.Cmd = (void *)&ReportLuns;
	cmd.CmdLen = (UWORD)sizeof(ReportLuns);
	cmd.Buffer = &buffer;
	cmd.TransferLen = sizeof(buffer);

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	if(execute(0, "    REPORT LUNS", true)) {
		return 0x01;
	}

	luns = buffer[0] / 8;

	print("    Number of LUNs: %lu\n", luns);

	if(luns) {
		print("          LUN list: ");

		for(i = 0; i < luns && i < 32 && i < sizeof(buffer) - 8; i++) {
			UWORD lun = (UWORD)buffer[2 * i + 3];

/* Only add LUNS > 7 to LUN list if the SCSI Driver supports 32 LUNs */
			if(lun < 8 || *((UWORD *)cmd.Handle) & 0x40) {
				lunVector |= (1L << lun);
			}

			if(i) {
				print(", ");
			}

			print("%u", lun);
		}

		print("\n");
	}

	return lunVector;
}


void
testReadFormatCapacities(UWORD lun)
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

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = execute(lun, "    READ FORMAT CAPACITIES", true);
	if(!status) {
		int i;

		int length = capacityList->length;
		if(length < 8) {
			printDeviceError(4, "Invalid format capacities list length: %d\n", length);
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
testGetConfiguration(UWORD lun)
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

	memset(&localSenseData, 0, sizeof(SENSE_DATA));

	status = execute(lun, "    GET CONFIGURATION", true);
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
If the device does not report an error for this command, the respective
SCSI Driver test is not executed.
*/
void
testSenseBuffer(UWORD lun)
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

	status = callInWithLun(&cmd, lun);
	if(!status || localSenseData.senseKey != 0x05 ||	localSenseData.addSenseCode != 0x20) {
		/* The command has not been rejected and cannot be used for this test */
		return;
	}

	print("    Testing SCSI Driver sense buffer handling\n");

	cmd.SenseBuffer = NULL;

	if(callInWithLun(&cmd, lun) != status) {
		printDeviceError(6, "Status code mismatch\n");
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

		status = callInWithLun(&cmd, lun);
		if(status) {
			printDeviceError(6, "Request failed with status %ld\n", status);
			print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
				localSenseData.senseKey, localSenseData.addSenseCode,
				localSenseData.addSenseCodeQualifier);
		}
		else {
			if(localSenseData.senseKey != localSenseData.senseKey ||
				localSenseData.addSenseCode != localSenseData.addSenseCode) {
				printDeviceError(6, "Sense data have not been preserved\n");
			}
		}
	}

	cmd.SenseBuffer = (BYTE *)&localSenseData;
}


void
printFeatures(UWORD features, const char *type)
{
	bool hasFeature = false;

	print("  Supported SCSI Driver features on %s level:\n", type);

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

	print("        Page %d: %s page (current, %s)\n", buf[offset] & 0x7f, name,
		buf[offset] & 0x80 ? "savable" : "not savable");

	printRawData(buf, offset, size + 2, "          ");

	if(size < expected) {
		printDeviceError(10, "Page size: %d bytes, which is less than the expected %d\n",
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
		printDeviceError(6, "Block data differ at offset %d\n", i);
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


LONG
callInWithLun(tpSCSICmd c, UWORD l)
{
	assert(c->Handle);

	c->Cmd[1] &= 0x1f;
	if(l < 8) {
		c->Cmd[1] |= l << 5;
	}

/* Encode LUN in flags if 32 LUNs are supported */
	if(*((UWORD *)c->Handle) & 0x40) {
		c->Flags = (l << 8) | 0x40;
	}

	return scsiCall->In(c);
}


LONG
execute(UWORD lun, const char *msg, bool reportError)
{
	LONG status = callInWithLun(&cmd, lun);
	if(status == 2 || status == 4) {
		if(localSenseData.errorClass && localSenseData.senseKey == 0x05 &&
			localSenseData.addSenseCode == 0x20) {
			print(msg);
			print(" is not supported by device\n");
		}
		else if(localSenseData.errorClass && localSenseData.senseKey == 0x02 &&
			localSenseData.addSenseCode == 0x3a) {
			print("      Medium not present, test skipped\n");
		}			
		else if(reportError) {
			printStatus(status);
		}
	}
	else if(status) {
		print("      Device reported status code %ld", status);

		if(status == SELECTERROR) {
				print(" (SELECTERROR)");
		}
		else if(status == STATUSERROR) {
				print(" (STATUSERROR)");
		}
		else if(status == PHASEERROR) {
				print(" (PHASEERROR)");
		}
		else if(status == BSYERROR) {
				print(" (BSYERROR)");
		}
		else if(status == BUSERROR) {
				print(" (BUSERROR)");
		}
		else if(status == TRANSERROR) {
				print(" (TRANSERROR)");
		}
		else if(status == FREEERROR) {
				print(" (FREEERROR)");
		}
		else if(status == TIMEOUTERROR) {
				print(" (TIMEOUTERROR)");
		}
		else if(status == DATATOOLONG) {
				print(" (DATATOOLONG)");
		}
		else if(status == LINKERROR) {
				print(" (LINKERROR)");
		}
		else if(status == TIMEOUTARBIT) {
				print(" (TIMEOUTARBIT)");
		}
		else if(status == PENDINGERROR) {
				print(" (PENDINGERROR)");
		}
		else if(status == PARITYERROR) {
				print(" (PARITYERROR)");
		}

		print("\n");
	}

	return status;
}


void
printStatus(LONG status)
{
	printDeviceError(6, "Request failed with status %ld\n", status);
	printSenseData();
}


LONG
printSenseData()
{
	if(localSenseData.errorClass) {
		print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
			localSenseData.senseKey, localSenseData.addSenseCode, localSenseData.addSenseCodeQualifier);
		
		if(localSenseData.valid) {
			const LONG information = (localSenseData.information1 << 24) |
				(localSenseData.information2 << 16) | (localSenseData.information3 << 8) |
				localSenseData.information4;
			print("      ILI: %d, Information: %ld\n", localSenseData.ILI, information);

			return information;
		}
	}

	return 0;
}


void
printExpectedSenseData(SENSE_DATA *localSenseData, UWORD senseKey, UWORD addSenseCode)
{
	printDeviceError(6, "Request was not correctly rejected\n");
	if(localSenseData->errorClass) {
		print("        Expected: Sense Key $%02X (got $%02X),"
			" ASC $%02X (got $%02X)\n",
			senseKey, localSenseData->senseKey, addSenseCode, localSenseData->addSenseCode);
	}
}


void
print(const char *msg, ...)
{
	va_list args;
	char s[161];

	va_start(args, msg);
	vsprintf(s, msg, args);
	va_end(args);
	logMsg(s);
}


void
printDeviceError(UWORD blanks, const char *msg, ...)
{
	int i;

	for(i = 0; i < blanks / 2; i++) {
		print("  ");
	}

	print("ERROR (Device): ");
	print(msg);

	deviceErrors++;
}


void
printDriverError(UWORD blanks, const char *msg, ...)
{
	int i;

	for(i = 0; i < blanks / 2; i++) {
		print("  ");
	}

	print("ERROR (SCSI Driver): ");
	print(msg);

	scsiDriverErrors++;
}
