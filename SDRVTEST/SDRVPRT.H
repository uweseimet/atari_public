/***********************************/
/* SCSI Driver/Firmware Test 2.70ž */
/*                                 */
/* (C) 2014-2025 Uwe Seimet        */
/***********************************/


#ifndef __sdrvprt_h
#define __sdrvprt_h

void print(const char *, ...);
void printStatus(LONG);
LONG printSenseData(void);
void printExpectedSenseData(SENSE_DATA *, UWORD, UWORD);
void printError(UWORD, const char *, ...);

#endif
