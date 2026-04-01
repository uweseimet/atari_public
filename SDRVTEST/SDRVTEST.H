/**********************************/
/* SCSI Driver/Firmware Test 3.01 */
/*                                */
/* (C) 2014-2026 Uwe Seimet       */
/**********************************/


#ifndef __sdrvtest_h
#define __sdrvtest_h

typedef struct {
	ULONG hi;
	ULONG lo;
} D_ULONG;

bool runTest(UWORD, UWORD, UWORD);
void testCheckDev(UWORD, UWORD);
ULONG testReportLuns(void);
void testUnitReady(UWORD);
UWORD testInquiry(UWORD, UWORD, UWORD);
void testOpenClose(UWORD, UWORD, ULONG);
void testSenseBuffer(UWORD);
void testRequestSense(UWORD, UWORD);
void testReadCapacity(UWORD, ULONG *);
void testRead(UWORD, UWORD, UWORD, ULONG, UBYTE *, UBYTE *, UBYTE *);
void testSeek(UWORD);
void testReadLong(UWORD);
void testModeSense(UWORD);
void testModeSelect(UWORD);
void testReadFormatCapacities(UWORD);
void testGetConfiguration(UWORD);
void print(const char *, ...);
void printFeatures(UWORD, const char *);
void printDriverError(UWORD, const char *, ...);

extern tSCSICmd cmd;
extern tpScsiCall scsiCall;
extern SENSE_DATA localSenseData;
extern const char *DEVICE_TYPES[];
extern UWORD scsiDriverErrors;
extern UWORD deviceErrors;

#endif
