/***********************************/
/* SCSI Driver/Firmware Test 2.63ž */
/*                                 */
/* (C) 2014-2025 Uwe Seimet        */
/***********************************/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tos.h>
#include <scsidrv/scsidefs.h>
#include "sdrvtest.h"
#include "sdrvprt.h"

FILE *out;


void
print(const char *msg, ...)
{
	va_list args;
	char s[161];

	va_start(args, msg);
	vsprintf(s, msg, args);
	va_end(args);
	printf(s);
	fprintf(out, s);
}


void
printError(LONG status)
{
	print("      ERROR: Request failed with status %ld\n", status);
	printSenseData();
}


LONG
printSenseData()
{
	if(senseData.errorClass) {
		print("      Sense Key $%02X, ASC $%02X, ASCQ $%02X\n",
			senseData.senseKey, senseData.addSenseCode, senseData.addSenseCodeQualifier);
		
		if(senseData.valid) {
			const LONG information = (senseData.information1 << 24) |
				(senseData.information2 << 16) | (senseData.information3 << 8) |
				senseData.information4;
			print("      ILI: %d, Information: %ld\n", senseData.ILI, information);

			return information;
		}
	}

	return 0;
}


void
printExpectedSenseData(SENSE_DATA *senseData, UWORD senseKey, UWORD addSenseCode)
{
	print("      ERROR: Request was not correctly rejected\n");
	if(senseData->errorClass) {
		print("        Expected: Sense Key $%02X (got $%02X),"
			" ASC $%02X (got $%02X)\n",
			senseKey, senseData->senseKey, addSenseCode, senseData->addSenseCode);
	}
}
