/******************************//* PiSCSI printer client 2.00 *//*                            *//* (C) 2022-2023 Uwe Seimet   *//******************************/

#define VERSION "2.00"
#include <stdio.h>#include <std.h>#include <tos.h>#include <scsidrv/scsidefs.h>#include "pi_lib.h"


bool print(tpScsiCall, tSCSICmd *, int);


FILE *file;

UBYTE Reserve[] = { 0x16, 0x00, 0x00, 0x00, 0x00, 0x00 };UBYTE Release[] = { 0x17, 0x00, 0x00, 0x00, 0x00, 0x00 };UBYTE Print[] = { 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00 };UBYTE SynchronizeBuffer[] = { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };intmain(int argc, char *argv[]){
	const char *filename;
	bool status;

	setLanguage();

	if(isGerman) {
		printf("\n\x1b\x70PiSCSI Drucker-Client V" VERSION "\x1b\x71\n");	}	else {
		printf("\n\x1b\x70PiSCSI printer client V" VERSION "\x1b\x71\n");	}	printf("Ω 2022-2023 Uwe Seimet\n");
	if(argc < 2) {
		return -1;
	}

	filename = argv[1];

	file = fopen(filename, "rb");
	if(!file) {
		if(isGerman) {			printf("Datei nicht gefunden: %s\n", filename);
		}		else {
			printf("File not found: %s\n", filename);
		}

		return -1;
	}

	status = execute(print, 2, NULL, "SCSI printer", "SCSI-Drucker");
	fclose(file);

	return status;
}

bool
print(tpScsiCall scsiCall, tSCSICmd *cmd, int lun)
{
	/* Currently PiSCSI only supports up to 4096 bytes per transfer */
	BYTE buf[4096];
	size_t length;
	bool status = true;

	Reserve[1] |= lun << 5;	cmd->Cmd = (void *)&Reserve;	cmd->CmdLen = 6;
	cmd->Buffer = NULL;	cmd->TransferLen = 0;

	if(scsiCall->Out(cmd)) {
		if(isGerman) {			printf("Fehler beim Reservieren des Druckers, "
				"ÅberprÅfen Sie das PiSCSI-Logfile\n");
		}		else {
			printf("Error when reserving the printer, "
				"check the PiSCSI logfile\n");
		}

		return false;
	}
	if(isGerman) {		printf("Druckvorgang lÑuft ...\n");
	}	else {
		printf("Printing in progress ...\n");
	}

	Print[1] |= lun << 5;	cmd->Cmd = (void *)&Print;	cmd->Buffer = buf;

	length = fread(buf, 1, sizeof(buf), file);
	while(status && length > 0) {
		cmd->Cmd[2] = length >> 16;
		cmd->Cmd[3] = length >> 8;
		cmd->Cmd[4] = length;
		cmd->TransferLen = length;

		status = !scsiCall->Out(cmd);
		if(status) {
			length = fread(buf, 1, sizeof(buf), file);
		}	}

	if(status) {
		SynchronizeBuffer[1] |= lun << 5;		cmd->Cmd = (void *)&SynchronizeBuffer;		cmd->Buffer = NULL;		cmd->TransferLen = 0;

		status = !scsiCall->Out(cmd);
	}

	if(status) {
		if(isGerman) {			printf("Druckvorgang beendet\n");
		}		else {
			printf("Printing finished\n");
		}
	}
	else {
		if(isGerman) {			printf("Fehler beim Drucken, "
				"ÅberprÅfen Sie das PiSCSI-Logfile\n");
		}		else {
			printf("Error when printing, "
				"check the PiSCSI logfile\n");
		}
	}

	Release[1] |= lun << 5;	cmd->Cmd = (void *)&Release;	cmd->Buffer = NULL;	cmd->TransferLen = 0;

	if(scsiCall->Out(cmd)) {
		if(isGerman) {			printf("Fehler beim Freigeben des Druckers, "
				"ÅberprÅfen Sie das PiSCSI-Logfile\n");
		}		else {
			printf("Error when releasing the printer, "
				"check the PiSCSI logfile\n");
		}

		status = false;
	}

	return status;
}