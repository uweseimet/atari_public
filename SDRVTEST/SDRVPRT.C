/***********************************/
/* SCSI Driver/Firmware Test 2.70ž */
/*                                 */
/* (C) 2014-2025 Uwe Seimet        */
/***********************************/


#include <assert.h>
#include <string.h>
#include <stdio.h>
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
printStatus(LONG status)
{
	printError(6, "Request failed with status %ld\n", status);
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
	printError(6, "Request was not correctly rejected\n");
	if(senseData->errorClass) {
		print("        Expected: Sense Key $%02X (got $%02X),"
			" ASC $%02X (got $%02X)\n",
			senseKey, senseData->senseKey, addSenseCode, senseData->addSenseCode);
	}
}


void
printError(UWORD blanks, const char *msg, ...)
{
	va_list args;
	char s[161];

	switch(blanks) {
		case 4:	print("    ");
						break;

		case 6:	print("      ");
						break;

		case 10:	print("          ");
							break;

		default:	assert(false);
							break;
	}

	print("ERROR: ");

	va_start(args, msg);
	vsprintf(s, msg, args);
	va_end(args);
	printf(s);
	fprintf(out, s);
}
