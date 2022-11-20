/*
   Sample implementation of a custom SCSI command with opcode 0xc8.
   With this command other computers can read data from the main
   memory of a TT or Falcon.
   The 12 byte command block looks like this:

   Byte  Meaning

	  0    Opcode 0xc8
  	1    Reserved
	  2    Memory address (MSB)
	  3    Memory address
 	  4    Memory address
    5    Memory address (LSB)	
	  6    Transfer length (MSB)
	  7    Transfer length
 	  8    Transfer length
	  9    Transfer length (LSB)
	  10   Reserved
	  11   Reserved

   The most relevant parts of this sample program are the
   implementations of TSel() and TCmd().
   The code can be compiled without modifications using Pure C.

   (C) 2000-2021 Uwe Seimet
*/


#include <string.h>
#include <tos.h>

/* From Steffen Engel's SCSI Driver bindings */
#include <scsidrv/scsidefs.h>


tpScsiCall scsiCall;

int installHandler(void);
int cdecl GetCookie(long, unsigned long *);
int cdecl TSel(WORD, UWORD, UWORD);
int cdecl TCmd(WORD, BYTE *);

tTargetHandler targetHandler =
	{ NULL, TSel, TCmd, NULL, NULL, NULL, NULL, NULL, NULL, NULL };


int
main()
{
	GetCookie('SCSI', (ULONG *)&scsiCall);
	if(!scsiCall) {
		Cconws("No SCSI Driver available\n");
		return -1;
	}

	if(!installHandler()) {
		Cconws("Target interface code couldn't be installed\n");
		return -1;
	}
	else {
		Cconws("Target interface code successfully installed\n");
	}

	Ptermres(_PgmSize, 0);

	return 0;
}


int
installHandler()
{
	tBusInfo busInfo;
	int installed = FALSE;

	long result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result) {
		if(busInfo.Features & cTarget) {
			if(!scsiCall->Install(busInfo.BusNo, &targetHandler)) {
				installed = TRUE;
			}
		}

		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	return installed;
}


long
cookieptr()
{
	return *((long *)0x5a0);
}


int cdecl
GetCookie(long cookie, unsigned long *p_value)
{
	long *cookiejar = (long *)Supexec(cookieptr);

	if(!cookiejar) {
		return FALSE;
	}

	do {
		if(cookiejar[0] == cookie) {
			if (p_value) *p_value = (unsigned long)cookiejar[1];
			return TRUE;
		}
		else
			cookiejar = &(cookiejar[2]);
	} while(cookiejar[-2]);

	return FALSE;
}


#pragma warn -par
int cdecl
TSel(WORD bus, UWORD CSB, UWORD CSD)
{
/* Accept selection, refuse reselection */
	return !(CSB & 0x04);
}
#pragma warn +par


int cdecl
TCmd(WORD bus, BYTE *cmd)
{
	BYTE *adr;
	ULONG len;

/* Check for the custom SCSI opcode 0xc8 */
	UBYTE *c = (UBYTE *)cmd;
	if(c[0] != 0xc8) {
		return FALSE;
	}

/* Initialize SCSI status for REQUEST SENSE to OK */
	memset(scsiCall->ReqData, 0, sizeof(tReqData));

/* Get memory address from SCSI command block bytes 2-5 */
	adr = (BYTE *)(((LONG)c[2] << 24) + ((LONG)c[3] << 16) +
		((LONG)c[4] << 8) + (LONG)c[5]);

/* Get transfer length from SCSI command block bytes 6-9 */
	len = ((LONG)c[6] << 24) + ((LONG)c[7] << 16) +
		((LONG)c[8] << 8) + (LONG)c[9];

/* Send data and then the status and message bytes */
	if(!scsiCall->SendData(bus, adr, len)) {
		scsiCall->SendStatus(bus, 0);
		scsiCall->SendMsg(bus, 0);
	}

	return TRUE;
}
