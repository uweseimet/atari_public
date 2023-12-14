/********************************//* SCSI2Pi shutdown client 2.20 *//*                              *//* (C) 2022-2023 Uwe Seimet     *//********************************/

#define VERSION "2.20"
#include <stdio.h>#include <std.h>#include <tos.h>
#include <string.h>#include <scsidrv/scsidefs.h>#include "pi_lib.h"


const char *BANNER[] = {
	"\n\x1b\x70SCSI2Pi shutdown client V" VERSION "\x1b\x71",
	"\n\x1b\x70SCSI2Pi Shutdown-Client V" VERSION "\x1b\x71"};
const char *HOST_SERVICES[] = {
	"SCSI2Pi host services",
	"SCSI2Pi-Host-Dienste"
};

const char *STOP[] = {
	"SCSI2Pi has been stopped",
	"SCSI2Pi wurde angehalten"
};

const char *REBOOT[] = {
	"The Pi is being restarted",
	"Der Pi wird neu gestartet"
};

const char *SHUTDOWN[] = {
	"The Pi is being shut down",
	"Der Pi wird heruntergefahren"
};

const char *ERROR_STOP[] = {
	"Error when stopping SCSI2Pi",
	"Fehler beim Anhalten von SCSI2Pi"
};

const char *ERROR_SHUTDOWN[] = {
	"Error when shutting down the Pi",
	"Fehler beim Herunterfahren des Pi"
};


UBYTE StartStopUnit[] = { 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* true: shut down SCSI2Pi only, otherwise shut down the Pi */
bool s2pOnly;
bool reboot;


bool shutDown(tpScsiCall, tSCSICmd *, int);
intmain(int argc, char *argv[]){
	setLanguage();

	printLocalized(BANNER);
	printf("½ 2022-2023 Uwe Seimet\n");

	s2pOnly = argc > 1 && !strcmpi(argv[1], "scsi2pi");
	reboot = argc > 1 && !strcmpi(argv[1], "reboot");

	return execute(shutDown, 3, "Host Services", HOST_SERVICES);
}

bool
shutDown(tpScsiCall scsiCall, tSCSICmd *cmd, int lun)
{
	StartStopUnit[1] |= lun << 5;	StartStopUnit[4] |= (!s2pOnly || reboot) ? 0x02 : 0x00;	StartStopUnit[4] |= reboot ? 0x01 : 0x00;	cmd->Cmd = (void *)&StartStopUnit;	cmd->CmdLen = 6;
	cmd->Buffer = NULL;
	cmd->TransferLen = 0;

	if(!scsiCall->In(cmd)) {
		if(s2pOnly) {
			printLocalized(STOP);
		}		else if(reboot) {
			printLocalized(REBOOT);
		}		else {
			printLocalized(SHUTDOWN);
		}
		return true;
	}

	return printLocalized(s2pOnly ? ERROR_STOP : ERROR_SHUTDOWN);}