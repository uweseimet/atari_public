/*******************************/

#define VERSION "2.20"



const char *BANNER[] = {
	"\n\x1b\x70SCSI2Pi printer client V" VERSION "\x1b\x71",
	"\n\x1b\x70SCSI2Pi Drucker-Client V" VERSION "\x1b\x71"

const char *SCSI_PRINTER[] = {
	"SCSI printer",
	"SCSI-Drucker"
};

const char *ERROR_RESERVE[] = {
	"Error when reserving the printer, "
	"check the Pi logfile",
	"Fehler beim Reservieren des Druckers, "
	"�berpr�fen Sie das Pi-Logfile"
};

const char *ERROR_RELEASE[] = {
	"Error when releasing the printer, "
	"check the Pi logfile",
	"Fehler beim Freigeben des Druckers, "
	"�berpr�fen Sie das Pi-Logfile"
};
	
const char *ERROR_PRINTING[] = {
	"Error when printing, check the Pi logfile",
	"Fehler beim Drucken, �berpr�fen Sie das Pi-Logfile"
};

const char *INFO_PROGRESS[] = {
	"Printing in progress ...",
	"Druckvorgang l�uft ..."
};

const char *INFO_FINISHED[] = {
	"Printing finished",
	"Druckvorgang beendet"
};


UBYTE Reserve[] = { 0x16, 0x00, 0x00, 0x00, 0x00, 0x00 };

bool print(tpScsiCall, tSCSICmd *, int);


FILE *file;

	const char *filename;
	bool status;

	setLanguage();

	printLocalized(BANNER);
	printf("� 2022-2023 Uwe Seimet\n");

	if(argc < 2) {
		return -1;
	}

	filename = argv[1];

	file = fopen(filename, "rb");
	if(!file) {
		if(isGerman) {
		}
			printf("File not found: %s\n", filename);
		}

		return -1;
	}

	status = execute(print, 2, NULL, SCSI_PRINTER);
	fclose(file);

	return status;
}

bool
print(tpScsiCall scsiCall, tSCSICmd *cmd, int lun)
{
	/* The printer emulation of PiSCSI <= 23.11.01
	   only supports up to 4096 bytes per transfer */
	BYTE buf[4096];
	size_t length;
	bool status = true;

	Reserve[1] |= lun << 5;
	cmd->Buffer = NULL;

	if(scsiCall->Out(cmd)) {
		return printLocalized(ERROR_RESERVE);
	}

	printLocalized(INFO_PROGRESS);

	Print[1] |= lun << 5;

	length = fread(buf, 1, sizeof(buf), file);
	while(status && length > 0) {
		cmd->Cmd[2] = length >> 16;
		cmd->Cmd[3] = length >> 8;
		cmd->Cmd[4] = length;
		cmd->TransferLen = length;

		status = !scsiCall->Out(cmd);
		if(status) {
			length = fread(buf, 1, sizeof(buf), file);
		}

	if(status) {
		SynchronizeBuffer[1] |= lun << 5;

		status = !scsiCall->Out(cmd);
	}

	if(status) {
		printLocalized(INFO_FINISHED);
	}
	else {
		printLocalized(ERROR_PRINTING);
	}

	Release[1] |= lun << 5;

	if(scsiCall->Out(cmd)) {
		return printLocalized(ERROR_RELEASE);
	}

	return status;
}