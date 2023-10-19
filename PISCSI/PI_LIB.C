/******************************//* PiSCSI client library 2.10 *//*                            *//* (C) 2022-2023 Uwe Seimet   *//******************************/#include <stdio.h>#include <std.h>#include <tos.h>#include <string.h>#include <scsidrv/scsidefs.h>#include "scsi3.h"#include "pi_lib.h"

UWORD getLuns(int *, int);bool getCookie(LONG, ULONG *);


bool isGerman;tpScsiCall scsiCall;tSCSICmd cmd;
tBusInfo busInfo;
tDevInfo devInfo;

SENSE_DATA senseData;

SENSE_BLK Inquiry = {
	0x12, 0x00, 0x00, 0x00, 0x00, (UBYTE)sizeof(INQUIRY_DATA), 0x00, 0x00, 0x00
};

intexecute(bool (*execute)(tpScsiCall, tSCSICmd *, int), int deviceType,
	const char *product, const char *msgEnglish,
	const char *msgGerman){	LONG oldstack = 0;
	LONG result;	bool done = false;
	int status = -1;

	getCookie('SCSI', (ULONG *)&scsiCall);	if(!scsiCall) {
		if(isGerman) {			printf("SCSI-Treiber nicht gefunden\n");
		}		else {			printf("SCSI Driver not found\n");		}		return -1;	}	if(!Super((void *)1L)) {		oldstack = Super(0L);	}

/* Only buses 0 (ACSI) and 1 (SCSI) need to be checked */
	result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result && busInfo.BusNo < 2) {
		result = scsiCall->InquireBus(cInqFirst, busInfo.BusNo, &devInfo);
		while(!result) {
			ULONG maxLen;

			tHandle handle = (tHandle)scsiCall->Open(busInfo.BusNo,
				&devInfo.SCSIId, &maxLen);

			if(((LONG)handle & 0xff000000L) != 0xff000000L) {
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

				for(i = 0; i < luns; i++) {
					int lun = lunList[i];

					Inquiry.lun = lun;

/* LUN in high byte of flags in case 32 LUNs are supported */
					if(*(cmd.Handle) & 0x40) {
						cmd.Flags = (lun << 8) | 0x40;
					}

					memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

					result = scsiCall->In(&cmd);
					if(!result) {
/* Find the required PiSCSI device, based on type and optional name */
						if(inquiryData.deviceType == deviceType &&
							(!product || !strncmpi(inquiryData.product, product, strlen(product)))) {
							char name[29];

							name[28] = 0;
							strncpy(name, inquiryData.vendor, 28);

							if(isGerman) {
								printf("%s gefunden: %s %ld.%d, '%s'\n",
									msgGerman, busInfo.BusName, devInfo.SCSIId.lo, lun, name);
							}
							else {
								printf("Found %s: %s %ld.%d, '%s'\n",
									msgEnglish, busInfo.BusName, devInfo.SCSIId.lo, lun, name);
							}

							status = execute(scsiCall, &cmd, lun) ? 0 : -1;

							done = true;
							break;
						}
					}

					lun++;
				}

				scsiCall->Close(handle);
			}

			if(done) {
				break;
			}

			result = scsiCall->InquireBus(cInqNext, busInfo.BusNo, &devInfo);
		}

		if(done) {
			break;
		}
			
		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}
	if(oldstack) {		Super((void *)oldstack);	}
	if(!done) {
		if(isGerman) {
			printf("%s nicht gefunden\n", msgGerman);
		}	
		else {
			printf("%s not found\n", msgEnglish);
		}
	}
	return status;}


UWORD
getLuns(int *lunList, int maxLuns) {
	UBYTE ReportLuns[] = { 0xa0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x00, 0, 0 };

	ULONG buf[66];
	UWORD luns;
	LONG result = -1;
	int index;
	int i;

	/* REPORT LUNS requires a device with support for all commands */
	if(*(cmd.Handle) & cAllCmds) {
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
	if(result) {
		for(i = 0; i < 8; i++) {
			lunList[i] = i;
		}

		return 8;
	}

	luns = (UWORD)(buf[0] / 8);

	index = 0;
	for(i = 0; i < luns && i < sizeof(buf) - 8 && index < maxLuns; i++) {
		int lun = (int)buf[2 * i + 3];
		/* Whether more than 8 LUNs are supported depends on the bus
		   and the SCSI Driver. With HDDRIVER 12 for Falcon/TT SCSI
		   up to 32 LUNs are supported. */
		if(lun < (*(cmd.Handle) & 0x40) ? 32 : 8) {
			lunList[index++] = lun;
		}
	}

	return index;
}


void
setLanguage()
{
  LONG old_stack = 0;
  WORD lang;
  ULONG dummy;
  SYSHDR *syshdr;

  if(!Super((void *)1l)) {
  	old_stack = Super(0l);
	}

  syshdr = *(SYSHDR **)0x4f2;
  syshdr = syshdr->os_base;
  lang = syshdr->os_palmode >> 1;

	if(getCookie('_AKP', &dummy)) {
		lang = (WORD)(dummy >> 8);
	}

  if(old_stack) {
  	Super((void *)old_stack);
	}

	switch(lang) {
		case 1:
		case 8:		lang = 1;
							break;
		case 2:
		case 7:		lang = 2;
							break;
		default:	lang = 0;
							break;
	}

  isGerman = lang == 1;
}

LONGcookieptr(){	return *((LONG *)0x5a0);}boolgetCookie(LONG cookie, ULONG *p_value){	LONG *cookiejar = (LONG *)Supexec(cookieptr);	if(!cookiejar) return false;	do {		if(cookiejar[0] == cookie) {			if (p_value) *p_value = (ULONG)cookiejar[1];			return true;		}		else			cookiejar = &(cookiejar[2]);	} while(cookiejar[-2]);	return false;}