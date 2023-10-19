/*******************************//* PiSCSI shutdown client 2.10 *//*                             *//* (C) 2022-2023 Uwe Seimet    *//*******************************/

#define VERSION "2.10"
#include <stdio.h>#include <std.h>#include <tos.h>
#include <string.h>#include <scsidrv/scsidefs.h>#include "pi_lib.h"


bool shutDown(tpScsiCall, tSCSICmd *, int);


/* true: shut down PiSCSI only, otherwise shut down the Pi */
bool piscsiOnly;
bool reboot;

UBYTE StartStopUnit[] = { 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00 };intmain(int argc, char *argv[]){
	setLanguage();

	if(isGerman) {
		printf("\n\x1b\x70PiSCSI Shutdown-Client V" VERSION "\x1b\x71\n");	}	else {
		printf("\n\x1b\x70PiSCSI shutdown client V" VERSION "\x1b\x71\n");	}	printf("½ 2022-2023 Uwe Seimet\n");
	piscsiOnly = argc > 1 && !strcmpi(argv[1], "piscsi");
	reboot = argc > 1 && !strcmpi(argv[1], "reboot");

	return execute(shutDown, 3, "Host Services",
		"PiSCSI host services", "PiSCSI-Host-Dienste");
}

bool
shutDown(tpScsiCall scsiCall, tSCSICmd *cmd, int lun)
{
	StartStopUnit[1] |= lun << 5;	StartStopUnit[4] |= (!piscsiOnly || reboot) ? 0x02 : 0x00;	StartStopUnit[4] |= reboot ? 0x01 : 0x00;	cmd->Cmd = (void *)&StartStopUnit;	cmd->CmdLen = 6;
	cmd->Buffer = NULL;
	cmd->TransferLen = 0;

	if(!scsiCall->In(cmd)) {
		if(isGerman) {
			if(piscsiOnly) {
				printf("PiSCSI wurde angehalten\n");
			}
			else if(reboot) {
				printf("Der Pi wird neu gestartet\n");
			}			else {
				printf("Der Pi wird heruntergefahren\n");
			}		}		else {
			if(piscsiOnly) {
				printf("PiSCSI has been stopped\n");
			}			else if(reboot) {
				printf("The Pi is being restarted\n");
			}			else {
				printf("The Pi is being shut down\n");
			}		}
		return true;
	}

	if(isGerman) {
		if(piscsiOnly) {			printf("Fehler beim Anhalten von PiSCSI\n");
		}		else {
			printf("Fehler beim Herunterfahren des Pi\n");
		}	}	else {
		if(piscsiOnly) {
			printf("Error when stopping PiSCSI\n");
		}		else {
			printf("Error when shutting down the Pi\n");
		}	}
	return false;}