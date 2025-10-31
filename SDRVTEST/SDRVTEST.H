/***********************************/
/* SCSI Driver/Firmware Test 2.63ž */
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
  unsigned int bootpref;
  char reserved1[4];
  unsigned char language;
  unsigned char keyboard;
  unsigned char datetime;
  char separator;
  unsigned char bootdelay;
  char reserved2[3];
  unsigned int vmode;
  unsigned char scsiid;
  char reserved_for_us;
} NVM;

typedef struct {
	UWORD busNo;
	UWORD id;
	ULONG maxLen;
	UWORD features;
	char busName[20];
	char deviceBusName[20];
} DEVICEINFO;

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

typedef int bool;
#define true TRUE
#define false FALSE

#define NVMSIZE 18
#define NVMSIZE_MILAN 224

#define EUNDEV -15L
#define ENHNDL -35L
#define EACCDN -36L

void testCheckDev(UWORD, UWORD);
void testUnitReady(void);
UWORD testInquiry(void);
void testOpenClose(UWORD, UWORD, ULONG);
void testSenseBuffer(void);
void testRequestSense(void);
void testReadCapacity(ULONG *);
void testRead(UWORD, ULONG, UBYTE *, UBYTE *, UBYTE *);
void testSeek(void);
void testReadLong(void);
void testModeSense(void);
void testReportLuns(void);
void testReadFormatCapacities(void);
void testGetConfiguration(void);
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
char *DULongToString(const D_ULONG *);
void print(const char *, ...);
void printError(LONG);
LONG printSenseData(void);
void printExpectedSenseData(SENSE_DATA *, UWORD, UWORD);
LONG execute(const char *, bool);

extern tSCSICmd cmd;
extern tpScsiCall scsiCall;
extern SENSE_DATA senseData;

#endif
