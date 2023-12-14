/**************************************//* SCSI2Pi Realtime clock client 2.20 *//*                                    *//* (C) 2022-2023 Uwe Seimet           *//**************************************/

#define VERSION "2.20"
#include <stdio.h>#include <std.h>#include <tos.h>
#include <scsidrv/scsidefs.h>#include "pi_lib.h"


const char *BANNER[] = {
	"\n\x1b\x70SCSI2Pi realtime clock client V" VERSION "\x1b\x71",
	"\n\x1b\x70SCSI2Pi Echtzeituhr-Client V" VERSION "\x1b\x71"};
const char *HOST_SERVICES[] = {
	"SCSI2Pi host services",
	"SCSI2Pi-Host-Dienste"
};

const char *ERROR_RTC[] = {
	"Error when reading date and time from SCSI2Pi host services",
	"Fehler beim Lesen von Datum und Uhrzeit von den "
	"SCSI2Pi-Host-Diensten"
};


typedef struct {	UBYTE reserved[6];	UBYTE majorVersion;	UBYTE minorVersion;	UBYTE year;	UBYTE month;	UBYTE day;	UBYTE hour;	UBYTE minute;	UBYTE second;} _datetime;


UBYTE ModeSense6[] = { 0x1a, 0x08, 0x20, 0x00, sizeof(_datetime), 0x00 };

bool getTime(tpScsiCall, tSCSICmd *, int);
intmain(){
	setLanguage();

	printLocalized(BANNER);
	printf("½ 2022-2023 Uwe Seimet\n");

	return execute(getTime, 3, "Host Services", HOST_SERVICES);
}

bool
getTime(tpScsiCall scsiCall, tSCSICmd *cmd, int lun)
{
	_datetime dateTime;
	ModeSense6[1] |= lun << 5;	cmd->Cmd = (void *)&ModeSense6;	cmd->CmdLen = 6;
	cmd->Buffer = &dateTime;	cmd->TransferLen = ModeSense6[4];

	if(!scsiCall->In(cmd)) {
		ULONG year = dateTime.year + 1900;		ULONG month = dateTime.month + 1;		ULONG second = dateTime.second / 2;
		if(isGerman) {			printf("Setze Datum und Uhrzeit auf %02d.%02ld.%04ld %02d:%02d:%02ld\n",				dateTime.day, month, year, dateTime.hour, dateTime.minute,				second);
		}
		else {
			printf("Setting date and time to %02ld/%02d/%04ld %02d:%02d:%02ld\n",				month, dateTime.day, year, dateTime.hour, dateTime.minute,				second);
		}
		Settime(((year - 1980) << 25) |			(month << 21) |			((ULONG)dateTime.day << 16) |			((ULONG)dateTime.hour << 11) |			((ULONG)dateTime.minute << 5) |			second);

		return true;
	}

	return printLocalized(ERROR_RTC);
}