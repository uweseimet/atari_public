/**********************************/
/* SCSI Driver/Firmware Test 3.00 */
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
void printFeatures(UWORD, const char *);
void printPageHeader(UBYTE *, int, const char *, int);
void printPages(UBYTE *, int, int);
void printPage1(UBYTE *, int);
void printPage2(UBYTE *, int);
void printPage3(UBYTE *, int);
void printPage4(UBYTE *, int);
void printPage5(UBYTE *, int);
void printPage7(UBYTE *, int);
void printPage8(UBYTE *, int);
void printPage10(UBYTE *, int);
void printPage12(UBYTE *, int);
void printPage15(UBYTE *, int);
void printPage16(UBYTE *, int);
void printPages17_20(UBYTE *, int, int);
void printPage0(UBYTE *, int, int);
void printRawData(UBYTE *, int, int, const char *);
void checkRoot(UBYTE *, UBYTE *, ULONG);
void initBuffer(UBYTE *, ULONG);
LONG callInWithLun(tpSCSICmd, UWORD);
LONG execute(UWORD, const char *, bool);
char *DULongToString(const D_ULONG *);
void printStatus(LONG);
void printSenseData(void);
void printExpectedSenseData(SENSE_DATA *, UWORD, UWORD);
void print(const char *, ...);
void printStatusError(LONG status);
void printDeviceError(UWORD, const char *, ...);
void printDriverError(UWORD, const char *, ...);

extern tSCSICmd cmd;
extern tpScsiCall scsiCall;
extern SENSE_DATA localSenseData;
extern const char *DEVICE_TYPES[];
extern UWORD scsiDriverErrors;
extern UWORD deviceErrors;

#endif
