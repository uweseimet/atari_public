/*************************************//* RaSCSI Realtime clock client 1.01 *//*                                   *//* (C) 2022 Uwe Seimet               *//*************************************/

#define VERSION "1.01"
#include <stdio.h>#include <std.h>#include <tos.h>
#include <scsidrv/scsidefs.h>#include "ra_lib.h"


bool getTime(tpScsiCall, tSCSICmd *, int);


typedef struct {	UBYTE reserved[6];	UBYTE majorVersion;	UBYTE minorVersion;	UBYTE year;	UBYTE month;	UBYTE day;	UBYTE hour;	UBYTE minute;	UBYTE second;} _datetime;

UBYTE ModeSense6[] = { 0x1a, 0x08, 0x20, 0x00, sizeof(_datetime), 0x00 };intmain(){
	setLanguage();

	if(isGerman) {
		printf("\n\x1b\x70RaSCSI Echtzeituhr-Client V" VERSION "\x1b\x71\n");	}	else {
		printf("\n\x1b\x70RaSCSI realtime clock client V" VERSION "\x1b\x71\n");
	}	printf("½ 2022 Uwe Seimet\n");
	return execute(getTime, 3, "Host Services",
		"RaSCSI host services", "RaSCSI-Host-Dienste");}

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

	if(isGerman) {		printf("Fehler beim Lesen von Datum und Uhrzeit von den "
			"RaSCSI-Host-Diensten\n");
	}
	else {
		printf("Error when reading date and time from RaSCSI host services\n");
	}

	return false;}