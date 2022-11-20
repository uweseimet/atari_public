/*******************************//* RaSCSI shutdown client 1.01 *//*                             *//* (C) 2022 Uwe Seimet         *//*******************************/

#define VERSION "1.01"
#include <stdio.h>#include <std.h>#include <tos.h>
#include <string.h>#include <scsidrv/scsidefs.h>#include "ra_lib.h"


bool shutDown(tpScsiCall, tSCSICmd *, int);


/* true: shut down RaSCSI only, otherwise shut down the Pi */
bool rascsiOnly;
bool reboot;

UBYTE StartStopUnit[] = { 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00 };intmain(int argc, char *argv[]){
	setLanguage();

	if(isGerman) {
		printf("\n\x1b\x70RaSCSI Shutdown-Client V" VERSION "\x1b\x71\n");	}	else {
		printf("\n\x1b\x70RaSCSI shutdown client V" VERSION "\x1b\x71\n");	}	printf("½ 2022 Uwe Seimet\n");
	rascsiOnly = argc > 1 && !strcmpi(argv[1], "rascsi");
	reboot = argc > 1 && !strcmpi(argv[1], "reboot");

	return execute(shutDown, 3, "Host Services",
		"RaSCSI host services", "RaSCSI-Host-Dienste");
}

bool
shutDown(tpScsiCall scsiCall, tSCSICmd *cmd, int lun)
{
	StartStopUnit[1] |= lun << 5;	StartStopUnit[4] |= (!rascsiOnly || reboot) ? 0x02 : 0x00;	StartStopUnit[4] |= reboot ? 0x01 : 0x00;	cmd->Cmd = (void *)&StartStopUnit;	cmd->CmdLen = 6;
	cmd->Buffer = NULL;
	cmd->TransferLen = 0;

	if(!scsiCall->In(cmd)) {
		if(isGerman) {
			if(rascsiOnly) {
				printf("RaSCSI wurde angehalten\n");
			}
			else if(reboot) {
				printf("Der Pi wird neu gestartet\n");
			}			else {
				printf("Der Pi wird heruntergefahren\n");
			}		}		else {
			if(rascsiOnly) {
				printf("RaSCSI has been stopped\n");
			}			else if(reboot) {
				printf("The Pi is being restarted\n");
			}			else {
				printf("The Pi is being shut down\n");
			}		}
		return true;
	}

	if(isGerman) {
		if(rascsiOnly) {			printf("Fehler beim Anhalten von RaSCSI\n");
		}		else {
			printf("Fehler beim Herunterfahren des Pi\n");
		}	}	else {
		if(rascsiOnly) {
			printf("Error when stopping RaSCSI\n");
		}		else {
			printf("Error when shutting down the Pi\n");
		}	}
	return false;}