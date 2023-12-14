/*******************************//* SCSI2Pi client library 3.00 *//*                             *//* (C) 2022-2023 Uwe Seimet    *//*******************************/#include <stdio.h>
#include <stdlib.h>#include <tos.h>#include <string.h>#include "scsi3.h"#include "pi_lib.h"


static const char *ERROR_SCSI_DRIVER[] = {
	"SCSI Driver not found",
	"SCSI-Treiber nicht gefunden"
};

static const char *HOST_SERVICES[] = {
	"SCSI2Pi host services",
	"SCSI2Pi-Host-Dienste"
};

static const char *ERROR_EXECUTE[] = {
	"Could not execute ExecuteOperation",
	"ExecuteOperation konnte nicht ausgefÅhrt werden"
};

static const char *ERROR_RECEIVE[] = {
	"Could not execute ReceiveOperationResults",
	"ReceiveOperationResults konnte nicht ausgefÅhrt werden"
};


static SENSE_BLK Inquiry = {
	0x12, 0x00, 0x00, 0x00, 0x00, (UBYTE)sizeof(INQUIRY_DATA), 0x00, 0x00, 0x00
};
static UBYTE ReportLuns[] = {
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static UBYTE ExecuteOperation[] = {
	0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};static UBYTE ReceiveOperationResults[] =
{
	0xc1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int inputFormat;
static const char *input;
static UWORD inputLen;
static int outputFormat;
static char *output;
static UWORD outputLen;
static tpScsiCall scsiCall;static tSCSICmd cmd;
static tBusInfo busInfo;
static tDevInfo devInfo;
static ULONG maxLen;

static SENSE_DATA senseData;

static int active_device = -1;
static int active_lun = -1;


int executeForActive(bool (*execute)(tpScsiCall, tSCSICmd *, int));static UWORD getLuns(int *, int);

bool
executeCommand(int inFormat, const char *in, UWORD inLen, int outFormat,
	char *out, UWORD outLen)
{
	inputFormat = inFormat;
	input = in;
	inputLen = inLen;
	outputFormat = outFormat;
	output = out;
	outputLen = outLen;

	return !execute(executeOperation, 3, "Host Services", HOST_SERVICES);
}


bool
executeOperation(tpScsiCall scsiCall, tSCSICmd *cmd, int lun)
{
	ExecuteOperation[1] = inputFormat | (lun << 5);	ExecuteOperation[7] = inputLen >> 8;
	ExecuteOperation[8] = inputLen;
	cmd->Cmd = (void *)&ExecuteOperation;	cmd->CmdLen = 10;
	cmd->Buffer = input;
	cmd->TransferLen = inputLen;;

	if (*(cmd->Handle) & 0x40) {
		cmd->Flags = (lun << 8) | 0x40;
	}

	if (scsiCall->Out(cmd)) {
		printLocalized(ERROR_EXECUTE);
		return false;
	}

	ReceiveOperationResults[1] = outputFormat | (lun << 5);	ReceiveOperationResults[7] = outputLen >> 8;
	ReceiveOperationResults[8] = outputLen;
	cmd->Cmd = (void *)&ReceiveOperationResults;	cmd->CmdLen = 10;
	cmd->Buffer = output;
	cmd->TransferLen = outputLen;

	if (scsiCall->In(cmd)) {
		printLocalized(ERROR_RECEIVE);
		return false;
	}

	return true;}
intexecute(bool (*execute)(tpScsiCall, tSCSICmd *, int), int deviceType,
	const char *product, const char *messages[]){	LONG oldstack = 0;
	LONG result;	bool done = false;
	bool status = false;

	/* Run the host services detection only once */
	if (active_device != -1) {
		return executeForActive(execute);
	}
	getCookie('SCSI', (ULONG *)&scsiCall);	if (!scsiCall) {
		return printLocalized(ERROR_SCSI_DRIVER);
	}	if (!Super((void *)1L)) {		oldstack = Super(0L);	}

/* Only buses 0 (ACSI) and 1 (SCSI) need to be checked */
	result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while (!result && busInfo.BusNo < 2) {
		result = scsiCall->InquireBus(cInqFirst, busInfo.BusNo, &devInfo);
		while (!result) {

			const tHandle handle = (tHandle)scsiCall->Open(busInfo.BusNo,
				&devInfo.SCSIId, &maxLen);

			if (((LONG)handle & 0xff000000L) != 0xff000000L) {
				int lunList[32];
				int luns;
				int i;
				INQUIRY_DATA inquiryData;

				cmd.Handle = handle;
				cmd.SenseBuffer = (BYTE *)&senseData;
				cmd.Timeout = 400;
				cmd.Flags = 0;

				luns = getLuns(lunList, (int)(sizeof(lunList) / sizeof(int)));

				cmd.Cmd = (void *)&Inquiry;
				cmd.CmdLen = 6;
				cmd.Buffer = &inquiryData;
				cmd.TransferLen = Inquiry.length;

				for (i = 0; i < luns; i++) {
					int lun = lunList[i];

					Inquiry.lun = lun;

/* LUN in high byte of flags in case 32 LUNs are supported */
					if (*(cmd.Handle) & 0x40) {
						cmd.Flags = (lun << 8) | 0x40;
					}

					memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

					result = scsiCall->In(&cmd);
					if (!result) {
/* Find the required SCSI2Pi device, based on type and optional name */
						if (inquiryData.deviceType == deviceType &&
							(!product || !strncmpi(inquiryData.product, product, strlen(product)))) {
							char name[29];

							name[28] = 0;
							strncpy(name, inquiryData.vendor, 28);

							if (isGerman) {
								printf("%s gefunden: %s %ld.%d, '%s'\n",
									messages[1], busInfo.BusName, devInfo.SCSIId.lo, lun, name);
							}
							else {
								printf("Found %s: %s %ld.%d, '%s'\n",
									messages[0], busInfo.BusName, devInfo.SCSIId.lo, lun, name);
							}

							active_device = (int)devInfo.SCSIId.lo;
							active_lun = lun;

							status = !execute(scsiCall, &cmd, lun);

							done = true;
							break;
						}
					}

					lun++;
				}

				scsiCall->Close(handle);
			}

			if (done) {
				break;
			}

			result = scsiCall->InquireBus(cInqNext, busInfo.BusNo, &devInfo);
		}

		if (done) {
			break;
		}
			
		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}
	if (oldstack) {		Super((void *)oldstack);	}

	if (!done) {
		if (isGerman) {
			printf("%s nicht gefunden\n", messages[1]);
		}	
		else {
			printf("%s not found\n", messages[0]);
		}
	}
	return status;}


int
executeForActive(bool (*execute)(tpScsiCall, tSCSICmd *, int)){
	tHandle handle;
	LONG oldstack = 0;
	bool status = false;
	if (!Super((void *)1L)) {		oldstack = Super(0L);	}

	handle = (tHandle)scsiCall->Open(busInfo.BusNo, &devInfo.SCSIId,
		&maxLen);

	if (((LONG)handle & 0xff000000L) != 0xff000000L) {
		status = !execute(scsiCall, &cmd, active_lun);

		scsiCall->Close(handle);
	}
	if (oldstack) {		Super((void *)oldstack);	}

	return status;
}


static UWORD
getLuns(int *lunList, int maxLuns)
{
	ULONG buf[66];
	UWORD luns;
	LONG result = -1;
	int index;
	int i;

	/* REPORT LUNS requires a device with support for all commands */
	if (*(cmd.Handle) & cAllCmds) {
		ReportLuns[8] = sizeof(buf) >> 8;
		ReportLuns[9] = sizeof(buf);

		/* LUN 0 is considered to be available,
		   see SAM (SCSI Architecture Model) */

		cmd.Cmd = (void *)&ReportLuns;
		cmd.CmdLen = 12;
		cmd.Buffer = &buf;
		cmd.TransferLen = sizeof(buf);

		result = scsiCall->In(&cmd);
	}

	/* If REPORT LUNS is not supported assume 8 LUNs */
	if (result) {
		for (i = 0; i < 8; i++) {
			lunList[i] = i;
		}

		return 8;
	}

	luns = (UWORD)(buf[0] / 8);

	index = 0;
	for (i = 0; i < luns && i < sizeof(buf) - 8 && index < maxLuns; i++) {
		int lun = (int)buf[2 * i + 3];
		/* Whether more than 8 LUNs are supported depends on the bus
		   and the SCSI Driver. With HDDRIVER 12 for Falcon/TT SCSI
		   up to 32 LUNs are supported. */
		if (lun < (*(cmd.Handle) & 0x40) ? 32 : 8) {
			lunList[index++] = lun;
		}
	}

	return index;
}
