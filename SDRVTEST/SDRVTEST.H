/***********************************/
/* SCSI Driver/Firmware Test 3.00ž */
/*                                 */
/* (C) 2014-2025 Uwe Seimet        */
/***********************************/


#ifndef __sdrvtest_h
#define __sdrvtest_h

typedef struct {
	ULONG hi;
	ULONG lo;
} D_ULONG;

typedef struct {
	UWORD valid : 1;
	UWORD errorClass : 3;
	UWORD errorCode : 4;
	UWORD segmentNumber : 8;
	UWORD fileMark : 1;
	UWORD EOM : 1;
	UWORD ILI : 1;
	UWORD : 1;
	UWORD senseKey : 4;
	UWORD information1 : 8;
	UWORD information2 : 8;
	UWORD information3 : 8;
	UWORD information4 : 8;
	UWORD addSenseLength : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD addSenseCode : 8;
	UWORD addSenseCodeQualifier : 8;
	UWORD fieldReplUnitCode : 8;
	UWORD sksv : 1;
	UWORD senseKeySpecific0 : 7;
	UWORD senseKeySpecific1 : 8;
	UWORD senseKeySpecific2 : 8;
} SENSE_DATA;

bool runTest(UWORD, const char *, UWORD, UWORD, UWORD);
void testCheckDev(UWORD, UWORD);
ULONG testReportLuns(void);
void testUnitReady(UWORD);
UWORD testInquiry(UWORD, UWORD);
void testOpenClose(UWORD, UWORD, ULONG);
void testSenseBuffer(UWORD);
void testRequestSense(UWORD, UWORD);
void testReadCapacity(UWORD, ULONG *);
void testRead(UWORD, UWORD, UWORD, ULONG, UBYTE *, UBYTE *, UBYTE *);
void testSeek(UWORD);
void testReadLong(UWORD);
void testModeSense(UWORD);
void testReadFormatCapacities(UWORD);
void testGetConfiguration(UWORD);
void printDevice(UWORD, UWORD, UWORD, const char *);
void printFeatures(UWORD);
void printPageHeader(UBYTE *, int, const char *, int);
void printPages(UBYTE *, int *, int, int);
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
LONG printSenseData(void);
void printExpectedSenseData(SENSE_DATA *, UWORD, UWORD);
void print(const char *, ...);
void printDeviceError(UWORD, const char *, ...);
void printDriverError(UWORD, const char *, ...);

extern tSCSICmd cmd;
extern tpScsiCall scsiCall;
extern SENSE_DATA localSenseData;

#endif
