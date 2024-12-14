/**************************************/

#define VERSION "2.20"

#include <scsidrv/scsidefs.h>


const char *BANNER[] = {
	"\n\x1b\x70SCSI2Pi realtime clock client V" VERSION "\x1b\x71",
	"\n\x1b\x70SCSI2Pi Echtzeituhr-Client V" VERSION "\x1b\x71"
const char *HOST_SERVICES[] = {
	"SCSI2Pi host services",
	"SCSI2Pi-Host-Dienste"
};

const char *ERROR_RTC[] = {
	"Error when reading date and time from SCSI2Pi host services",
	"Fehler beim Lesen von Datum und Uhrzeit von den "
	"SCSI2Pi-Host-Diensten"
};


typedef struct {


UBYTE ModeSense6[] = { 0x1a, 0x08, 0x20, 0x00, sizeof(_datetime), 0x00 };

bool getTime(tpScsiCall, tSCSICmd *, int);

	setLanguage();

	printLocalized(BANNER);
	printf("� 2022-2023 Uwe Seimet\n");

	return execute(getTime, 3, "Host Services", HOST_SERVICES);
}

bool
getTime(tpScsiCall scsiCall, tSCSICmd *cmd, int lun)
{
	_datetime dateTime;
	ModeSense6[1] |= lun << 5;
	cmd->Buffer = &dateTime;

	if(!scsiCall->In(cmd)) {
		ULONG year = dateTime.year + 1900;
		if(isGerman) {
		}
		else {
			printf("Setting date and time to %02ld/%02d/%04ld %02d:%02d:%02ld\n",
		}


		return true;
	}

	return printLocalized(ERROR_RTC);
}