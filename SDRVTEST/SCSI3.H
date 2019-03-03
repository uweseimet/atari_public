#ifndef __scsi3_h
#define __scsi3_h

#include <portab.h>

typedef struct {
	UWORD peripheralQualifier : 3;
	UWORD deviceType : 5;
	UWORD RMB : 1;
	UWORD deviceTypeModifier : 7;
	UWORD ISOVersion : 2;
	UWORD ECMAVersion : 3;
	UWORD ANSIVersion : 3;
	UWORD AENC : 1;
	UWORD TrmIOP : 1;
	UWORD : 2;
	UWORD responseDataFormat : 4;
	UBYTE additionalLength;
	UBYTE res;
	UWORD : 8;
	UWORD RelAdr : 1;
	UWORD WBus32 : 1;
	UWORD WBus16 : 1;
	UWORD Sync : 1;
	UWORD Linked : 1;
	UWORD : 1;
	UWORD CmdQue : 1;
	UWORD SftRe : 1;
	char vendor[8];
	char product[16];
	char revision[4];
} INQUIRY_DATA;	

typedef struct {
	UWORD PS : 1;
	UWORD : 1;
	UWORD pageCode : 6;
	UWORD pageLength : 8;
	UWORD : 3;
	UWORD DUA : 1;
	UWORD : 4;
	UWORD : 8;
} PAGE_0;

typedef struct {
	UWORD PS : 1;
	UWORD : 1;
	UWORD pageCode : 6;
	UWORD pageLength : 8;
	UWORD tracksPerZone;
	UWORD altSectorsPerZone;
	UWORD altTracksPerZone;
	UWORD altTracksPerLUN;
	UWORD sectorsPerTrack;
	UWORD bytesPerSector;
	UWORD interleave;
	UWORD trackSkewFactor;
	UWORD cylinderSkewFactor;
	UWORD SSEC : 1;
	UWORD HSEC : 1;
	UWORD RMB : 1;
	UWORD SURF : 1;
	UWORD : 4;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
} PAGE_3;

typedef struct {
	UWORD PS : 1;
	UWORD : 1;
	UWORD pageCode : 6;
	UWORD pageLength : 8;
	UBYTE cylinders[3];
	UBYTE heads;
	UBYTE preComp[3];
	UBYTE redWrite[3];
	UWORD stepRate;
	UBYTE landingZone[3];
	UWORD : 6;
	UWORD RPL : 2;
	UWORD rotationalOffset : 8;
	UWORD : 8;
	UWORD rotationRate;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
} PAGE_4;

typedef struct {
	UWORD PS : 1;
	UWORD : 1;
	UWORD pageCode : 6;
	UWORD pageLength : 8;
	UWORD : 5;
	UWORD WCE : 1;
	UWORD MF : 1;
	UWORD RCD : 1;
	UWORD readRetention : 4;
	UWORD writeRetention : 4;
	UWORD disablePrefetch;
	UWORD minimumPrefetch;
	UWORD maximumPrefetch;
	UWORD maximumPrefetchCeiling;
} PAGE_8;

typedef struct {
	UWORD PS : 1;
	UWORD : 1;
	UWORD pageCode : 6;
	UWORD pageLength : 8;
	UWORD dhs : 1;
	UWORD scrub : 1;
	UWORD vsc : 1;
	UWORD ftme : 1;
	UWORD ridi : 1;
	UWORD offtr : 1;
	UWORD : 1;
	UWORD phsk : 1;
	UWORD spinDownTimer : 8;
	UWORD headParkTimer : 8;
	UWORD : 4;
	UWORD seekRetryCount : 4;
} PAGE_47;

typedef struct {
	UWORD PS : 1;
	UWORD : 1;
	UWORD pageCode : 6;
	UWORD pageLength : 8;
	UWORD DIO : 1;
	UWORD DII : 1;
	UWORD FDB : 1;
	UWORD RUEE : 1;
	UWORD FDPE : 1;
	UWORD : 1;
	UWORD DUA : 1;
	UWORD DRT : 1;
	UWORD DDIS : 1;
	UWORD DELDIS : 1;
	UWORD : 1;
	UWORD DP : 1;
	UWORD SSID : 1;
	UWORD SCSIADR : 3;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
} PAGE_57;

typedef struct {
	UBYTE density;
	UBYTE res1[4];
	UBYTE bytesPerSector[3];
	UBYTE formatCode;
	UBYTE cylinders[2];
	UBYTE heads;
	UBYTE redWrite[2];
	UBYTE preComp[2];
	UBYTE landingZone;
	UBYTE stepRate;
	UBYTE flags;
	UBYTE sectorsPerTrack;
} PAGE_RLL;

typedef struct {
	UWORD opcode : 8;
	UWORD lun : 3;
	UWORD vendor : 5;
	UWORD reserved1 : 8;
	UWORD reserved2 : 8;
	UWORD length : 8;
	UWORD : 8;
} CMD_BLK;

typedef struct {
	UWORD opcode : 8;
	UWORD lun : 3;
	UWORD fmtData : 1;
	UWORD cmpLst : 1;
	UWORD defectListFormat : 3;
	UWORD vendor : 8;
	UWORD interleaveMSB : 8;
	UWORD interleaveLSB : 8;
	UWORD : 8;
} FORMAT_BLK;

typedef struct {
	UWORD opcode : 8;
	UWORD lun : 3;
	UWORD flags : 5;
	UWORD PC : 2;
	UWORD pagecode : 6;
	UWORD : 8;
	UWORD length : 8;
	UWORD vu : 1;
	UWORD : 5;
	UWORD flag : 1;
	UWORD link : 1;
} SENSE_BLK;

typedef struct {
	UWORD opcode : 8;
	UWORD lun : 3;
	UWORD flags : 5;
	UWORD PC : 2;
	UWORD pagecode : 6;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD length_lo : 8;
	UWORD vu : 1;
	UWORD : 5;
	UWORD flag : 1;
	UWORD link : 1;
} SENSE_10_BLK;

typedef struct {
	UWORD opcode : 8;
	UWORD lun : 3;
	UWORD pf : 1;
	UWORD reserved : 3;
	UWORD sp : 1;
	UWORD : 8;
	UWORD : 8;
	UWORD length : 8;
	UWORD vu : 1;
	UWORD : 5;
	UWORD flag : 1;
	UWORD link : 1;
} SELECT_BLK;

typedef struct {
	UWORD opcode : 8;
	UWORD lun : 3;
	UWORD pf : 1;
	UWORD reserved : 3;
	UWORD sp : 1;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD length_lo : 8;
	UWORD vu : 1;
	UWORD : 5;
	UWORD flag : 1;
	UWORD link : 1;
} SELECT_10_BLK;

typedef struct {
	UWORD valid : 1;
	UWORD errorCode : 7;
	UWORD segmentNumber : 8;
	UWORD fileMark : 1;
	UWORD EOM : 1;
	UWORD ILI : 1;
	UWORD : 1;
	UWORD senseKey : 4;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
	UWORD : 8;
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

#endif
