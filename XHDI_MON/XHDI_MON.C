/****************************/
/* XHDI_MON 1.21            */
/*                          */
/* (C) 1999-2017 Uwe Seimet */
/****************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <portab.h>


#define ERROR -1
#define EDRVNR -2
#define EWRPRO -13
#define E_CHNG -14
#define EINVFN -32
#define EACCDN -36
#define EDRIVE -46


typedef int bool;
#define true TRUE
#define false FALSE


bool con = false;
bool aux = false;
bool prt = false;


typedef LONG cdecl (*cookieVal)(UWORD opcode,...);

extern LONG cdecl dispatch(UWORD opcode, ...);
LONG result(LONG);
void prres(LONG);
void prflags(ULONG);
void prbpb(BPB *);
void sysprintf(const char *);
static int setcookie(long, ULONG, ULONG *);

UWORD cdecl XHGetVersion(void);
LONG cdecl XHInqTarget(UWORD, UWORD, ULONG *, ULONG *, char *);
LONG cdecl XHReserve(UWORD, UWORD, UWORD, UWORD);
LONG cdecl XHLock(UWORD, UWORD, UWORD, UWORD);
LONG cdecl XHStop(UWORD, UWORD, UWORD, UWORD);
LONG cdecl XHEject(UWORD, UWORD, UWORD, UWORD);
ULONG cdecl XHDrvMap(void);
LONG cdecl XHInqDev(UWORD, UWORD *, UWORD *, ULONG *, BPB *);
LONG cdecl XHInqDriver(UWORD, char *, char *, char *, UWORD *, UWORD *);
LONG cdecl XHNewCookie(void *);
LONG cdecl XHReadWrite(UWORD, UWORD, UWORD, ULONG, UWORD, void *);
LONG cdecl XHInqTarget2(UWORD, UWORD, ULONG *, ULONG *, char *, UWORD);
LONG cdecl XHInqDev2(UWORD, UWORD *, UWORD *, ULONG *, BPB *, ULONG *, char *);
LONG cdecl XHDriverSpecial(ULONG, ULONG, UWORD, void *);
LONG cdecl XHGetCapacity(UWORD, UWORD, ULONG *, ULONG *);
LONG cdecl XHMediumChanged(UWORD, UWORD);
LONG cdecl XHMiNTInfo(UWORD, void *);
LONG cdecl XHDOSLimits(UWORD, ULONG);
LONG cdecl XHLastAccess(UWORD, UWORD, ULONG *);
LONG cdecl XHReaccess(UWORD, UWORD);

typedef struct {
	void *func;
	ULONG len;
} DISPATCHER;

DISPATCHER dispatcher[] = {
	{ XHGetVersion, 0 },
	{ XHInqTarget, 16 },
	{ XHReserve, 8 },
	{ XHLock, 8 },
	{ XHStop, 8 },
	{ XHEject, 8 },
	{ XHDrvMap, 0 },
	{ XHInqDev, 18 },
	{ XHInqDriver, 22 },
	{ XHNewCookie, 4 },
	{ XHReadWrite, 16 },
	{ XHInqTarget2, 18 },
	{ XHInqDev2, 26 },
	{ XHDriverSpecial, 14 },
	{ XHGetCapacity, 12 },
	{ XHMediumChanged, 4 },
	{ XHMiNTInfo, 6 },
	{ XHDOSLimits, 6 },
	{ XHLastAccess, 8 },
	{ XHReaccess, 4 }
};

cookieVal xhdi;


typedef void (*out)(WORD);
extern void conout(WORD);
extern void auxout(WORD);
extern void prtout(WORD);


void
main(WORD argc, const char *argv[])
{
	if(argc > 1) {
		con = !strcmp(argv[1], "--con");
		aux = !strcmp(argv[1], "--aux");
		prt = !strcmp(argv[1], "--prt");

		if(!con && !aux && !prt) {
			printf("\nIllegal output channel argument, XHDI_MON not installed");
			return;
		}
	}

	if(!setcookie('XHDI', (ULONG)dispatch, (ULONG *)&xhdi)) {
		printf("\nXHDI compatible driver not found, XHDI_MON not installed");
		return;
	}

	printf("\n\x1b\x70XHDI_MON V1.21\x1b\x71");
	printf("\n½ 1999-2017 Uwe Seimet\n");

	Ptermres(_PgmSize, 0);
}


UWORD
cdecl XHGetVersion()
{
	UWORD res;
	char str[256];

	sysprintf("XHGetVersion");
	res = (UWORD)xhdi(0);
	sprintf(str, "-> %X.%02X", res >> 8, res & 0xff);
	sysprintf(str);

	return res;
}

LONG
cdecl XHInqTarget(UWORD major, UWORD minor, ULONG *block_size,
	ULONG *device_flags, char *product_name)
{
	char s[32];
	char str[256];
	LONG res;

	sprintf(str, "XHInqTarget    major %d  minor %d", major, minor);
	sysprintf(str);
	res = xhdi(1, major, minor, block_size, device_flags, product_name);
	if(res) prres(res);
	else {
		sprintf(str, "-> 0");

		if(block_size) {
			sprintf(s, "  block_size %ld", *block_size);
			strcat(str, s);
		}

		if(device_flags) {
			prflags(*device_flags);
		}

		if(product_name) {
			sprintf(s, "  product_name '%s'", product_name);
			strcat(str, s);
		}

		sysprintf(str);
	}

	return res;
}

LONG
cdecl XHReserve(UWORD major, UWORD minor, UWORD do_reserve, UWORD key)
{
	char str[256];

	sprintf(str, "XHReserve    major %d  minor %d  do_reserve %d"
		"  key  %d", major, minor, do_reserve, key);
	sysprintf(str);
	return result(xhdi(2, major, minor, do_reserve, key));
}

LONG
cdecl XHLock(UWORD major, UWORD minor, UWORD do_lock, UWORD key)
{
	char str[256];

	sprintf(str, "XHLock    major %d  minor %d  do_lock %d"
		"  key %d", major, minor, do_lock, key);
	sysprintf(str);
	return result(xhdi(3, major, minor, do_lock, key));
}

LONG
cdecl XHStop(UWORD major, UWORD minor, UWORD do_stop, UWORD key)
{
	char str[256];

	sprintf(str, "XHStop    major %d  minor %d  do_stop %d"
		"  key %d", major, minor, do_stop, key);
	sysprintf(str);
	return result(xhdi(4, major, minor, do_stop, key));
}

LONG
cdecl XHEject(UWORD major, UWORD minor, UWORD do_eject, UWORD key)
{
	char str[256];

	sprintf(str, "XHEject    major %d  minor %d  do_eject %d"
		"  key %d", major, minor, do_eject, key);
	sysprintf(str);
	return result(xhdi(5, major, minor, do_eject, key));
}

ULONG
cdecl XHDrvMap()
{
	int i;
	ULONG ret;
	char str[256];

	sysprintf("XHDrvMap");
	ret = xhdi(6);
	strcpy(str, "-> ");
	for(i = 0; i < 32; i++) {
		if(ret & (1L << i)) {
			char s[2];

			if(i < 26) sprintf(s, "%c", i + 'A');
			else sprintf(s, "%c", i - 26 + '0');

			strcat(str, s);
		}
	}
	sysprintf(str);

	return ret;
}

LONG
cdecl XHInqDev(UWORD bios_device, UWORD *major, UWORD *minor,
	ULONG *start_sector, BPB *bpb)
{
	char s[32];
	LONG res;
	char str[256];

	sprintf(str, "XHInqDev    bios_device %d", bios_device);
	sysprintf(str);
	res = xhdi(7, bios_device, major, minor, start_sector, bpb);
	if(res && res != EDRVNR) prres(res);
	else {
		sprintf(str, res != EDRVNR ? "-> 0" : "-> EDRVNR");

		if(major) {
			sprintf(s, "  major %d", *major);
			strcat(str, s);
		}

		if(minor) {
			sprintf(s, "  minor %d", *minor);
			strcat(str, s);
		}

		if(start_sector && res != EDRVNR) {
			sprintf(s, "  start_sector %ld", *start_sector);
			strcat(str, s);
		}
		if(bpb && res != EDRVNR) {
			sprintf(s, "  bpb $%p", bpb);
			strcat(str, s);
		}

		sysprintf(str);

		if(res != EDRVNR) prbpb(bpb);
	}

	return res;
}

LONG
cdecl XHInqDriver(UWORD bios_device, char *name, char *version,
	char *company, UWORD *ahdi_version, UWORD *maxIPL)
{
	char s[32];
	LONG res;
	char str[256];

	sprintf(str, "XHInqDriver    bios_device %d", bios_device);
	sysprintf(str);
	res = xhdi(8, bios_device, name, version, company, ahdi_version,
		maxIPL);
	prres(res);
	if(!res) {
		str[0] = 0;

		if(name) {
			sprintf(s, "  name '%s'", name);
			strcat(str, s);
		}

		if(version) {
			sprintf(s, "  version '%s'", version);
			strcat(str, s);
		}

		if(company) {
			sprintf(s, "  company '%s'", company);
			strcat(str, s);
		}

		if(ahdi_version) {
			sprintf(s, "  ahdi_version '%s'", ahdi_version);
			strcat(str, s);
		}

		if(maxIPL) {
			sprintf(s, "  maxIPL %d", maxIPL);
			strcat(str, s);
		}

		if(strlen(str)) sysprintf(str);
	}

	return res;
}

LONG
cdecl XHNewCookie(void *newcookie)
{
	char str[256];

	sprintf(str, "XHNewCookie    $%p", newcookie);
	sysprintf(str);
	xhdi = newcookie;
	return 0;
}

LONG
cdecl XHReadWrite(UWORD major, UWORD minor, UWORD rwflag, ULONG recno,
	UWORD count, void *buf)
{
	char str[256];

	sprintf(str, "XHReadWrite    major %d  minor %d  rwflag %d  "
		"recno %ld  count %d  buf $%p", major, minor, rwflag, recno, count, buf);
	sysprintf(str);
	return result(xhdi(10, major, minor, rwflag, recno, count, buf));
}

LONG cdecl
XHInqTarget2(UWORD major, UWORD minor, ULONG *block_size,
	ULONG *device_flags, char *product_name, UWORD stringlen)
{
	char s[32];
	LONG res;
	char str[256];

	sprintf(str, "XHInqTarget2    major %d  minor %d  stringlen %d",
	major, minor, stringlen);
	sysprintf(str);
	res = xhdi(11, major,minor, block_size,device_flags, product_name,
		stringlen);
	if(res) prres(res);
	else {
		sprintf(str, "-> 0");

		if(block_size) {
			sprintf(s, "  block_size %ld", *block_size);
			strcat(str, s);
		}

		if(device_flags) {
			prflags(*device_flags);
		}

		if(product_name) {
			sprintf(s, "  product_name '%s'", product_name);
			strcat(str, s);
		}

		sysprintf(str);
	}

	return res;
}

LONG
cdecl XHInqDev2(UWORD bios_device, UWORD *major, UWORD *minor,
	ULONG *start_sector, BPB *bpb, ULONG *blocks, char *partid)
{
	char s[32];
	LONG res;
	char str[256];

	sprintf(str, "XHInqDev2    bios_device %d", bios_device);
	sysprintf(str);
	res = xhdi(12, bios_device, major, minor, start_sector, bpb, blocks,
		partid);
	if(res && res != EDRVNR) prres(res);
	else {
		sprintf(str, res != EDRVNR ? "-> 0" : "-> EDRVNR");

		if(major) {
			sprintf(s, "  major %d", *major);
			strcat(str, s);
		}

		if(minor) {
			sprintf(s, "  minor %d", *minor);
			strcat(str, s);
		}

		if(start_sector && res != EDRVNR) {
			sprintf(s, "  start_sector %ld", *start_sector);
			strcat(str, s);
		}

		if(bpb && res != EDRVNR) {
			sprintf(s, "  bpb $%p", bpb);
			strcat(str, s);
		}

		if(partid && res != EDRVNR) {
			sprintf(s, "  partid '%s'", partid);
			strcat(str, s);
		}

		sysprintf(str);

		if(res != EDRVNR) prbpb(bpb);
	}

	return res;
}

LONG
cdecl XHDriverSpecial(ULONG key1, ULONG key2, UWORD subopcode, void *data)
{
	char str[256];

	if(key1 == (ULONG)'USHD' && (key2 == 0x13497800L || key2 == 0x2690454L))
		sprintf(str, "XHDriverSpecial    [HDDRIVER]  subopcode %d"
			"  data $%p", subopcode, data);
	else
		sprintf(str, "XHDriverSpecial    key1 %ld  key2 %ld  subopcode %d"
			"  data $%p", key1, key2, subopcode, data);
	sysprintf(str);
	return result(xhdi(13, key1, key2, subopcode, data));
}

LONG
cdecl XHGetCapacity(UWORD major, UWORD minor, ULONG *blocks, ULONG *bs)
{
	char s[32];
	LONG res;
	char str[256];

	sprintf(str, "XHGetCapacity    major %d  minor %d", major,minor);
	sysprintf(str);
	res = xhdi(14, major, minor, blocks, bs);
	prres(res);
	if(!res) {
		str[0] = 0;

		if(blocks) {
			sprintf(s, "  blocks %ld", *blocks);
			strcat(str, s);
		}

		if(bs) {
			sprintf(s, "  blocksize %ld", *bs);
			strcat(str, s);
		}

		sysprintf(str);
	}

	return res;
}

LONG
cdecl XHMediumChanged(UWORD major, UWORD minor)
{
	char str[256];

	sprintf(str, "XHMediumChanged    major %d  minor %d", major, minor);
	sysprintf(str);
	return result(xhdi(15, major, minor));
}

LONG
cdecl XHMiNTInfo(UWORD opcode, void *data)
{
	char str[256];

	sprintf(str, "XHMiNTInfo    opcode %d  data $%p", opcode, data);
	sysprintf(str);
	return result(xhdi(16, opcode, data));
}

LONG
cdecl XHDOSLimits(UWORD which, ULONG limit)
{
	char s[16];
	LONG res;
	char str[256];

	sprintf(str, "XHDOSLimits    ");

	switch(which) {
		case 0:
			strcat(str, "XH_DL_SECSIZ");
			break;
		case 1:
		 	strcat(str, "XH_DL_MINFAT");
			break;
		case 2:
			strcat(str, "XH_DL_MAXFAT");
			break;
		case 3:
			strcat(str, "XH_DL_MINSPC");
			break;
		case 4:
			strcat(str, "XH_DL_MAXSPC");
			break;
		case 5:
			strcat(str, "XH_DL_CLUSTS");
			break;
		case 6:
			strcat(str, "XH_DL_MAXSEC");
			break;
		case 7:
			strcat(str, "XH_DL_DRIVES");
			break;
		case 8:
			strcat(str, "XH_DL_CLSIZB");
			break;
		case 9:
			strcat(str, "XH_DL_RDLEN");
			break;
		case 12:
			strcat(str, "XH_DL_CLUSTS12");
			break;
		case 13:
			strcat(str, "XH_DL_CLUSTS32");
			break;
		case 14:
			strcat(str, "XH_DL_BFLAGS");
			break;
		default:
			{
				sprintf(s, "%d", which);
				strcat(str, s);
			}
			break;
	}

	sprintf(s, "  limit %ld", limit);
	strcat(str, s);

	sysprintf(str);

	res = xhdi(17, which, limit);

	sprintf(str, "-> %ld", res);
	sysprintf(str);

	return res;
}

LONG
cdecl XHLastAccess(UWORD major, UWORD minor, ULONG *ms)
{
	char s[32];
	LONG res;
	char str[256];

	sprintf(str, "XHLastAccess    major %d  minor %d", major, minor);
	sysprintf(str);
	res = xhdi(18, major, minor, ms);
	prres(res);
	sprintf(s, "  ms %ld", res, ms);
	strcat(str, s);
	sysprintf(str);

	return res;
}

LONG
cdecl XHReaccess(UWORD major, UWORD minor)
{
	char str[256];

	sprintf(str, "XHReaccess    major %d  minor %d", major, minor);
	sysprintf(str);
	return result(xhdi(19, major, minor));
}

void
sysprintf(const char* s)
{
	if(!con && !aux && !prt) {
		gemdos(0x1069, s);
	}
	else {
		size_t i;
		size_t len;
	  out channel;

		if(con) channel = conout;
		else if(aux) channel = auxout;
		else channel = prtout;

		len = strlen(s);
		for(i = 0; i < len; i++) {
			channel(s[i]);
		}

		channel(0x0d);
		channel(0x0a);
	}
}


LONG
result(LONG res)
{
	prres(res);

	return res;
}

void
prres(LONG res)
{
	char str[256];

	strcpy(str, "-> ");

	switch((WORD)res) {
		case ERROR:
			strcat(str, "ERROR");
			break;
		case EDRVNR:
			strcat(str, "EDRVNR");
			break;
		case EWRPRO:
			strcat(str, "EWRPRO");
			break;
		case E_CHNG:
			strcat(str, "E_CHNG");
			break;
		case EINVFN:
			strcat(str, "EINVFN");
			break;
		case EACCDN:
			strcat(str, "EACCDN");
			break;
		case EDRIVE:
			strcat(str, "EDRIVE");
			break;
		default:
			sprintf(str, "-> %ld", res);
			break;
	}

	sysprintf(str);
}

void
prflags(ULONG flags)
{
	char str[256];

	strcat(str, "  device_flags");
	if(flags & 0xe000000fL) {
		if(flags & 0x01) strcat(str, "  XH_TARGET_STOPPABLE");
		if(flags & 0x02) strcat(str, "  XH_TARGET_REMOVABLE");
		if(flags & 0x04) strcat(str, "  XH_TARGET_LOCKABLE");
		if(flags & 0x08) strcat(str, "  XH_TARGET_EJECTABLE");
		if(flags & 0x20000000L) strcat(str, "  XH_TARGET_LOCKED");
		if(flags & 0x40000000L) strcat(str, "  XH_TARGET_STOPPED");
		if(flags & 0x80000000L) strcat(str, "  XH_TARGET_RESERVED");
	}
	else {
		char s[16];

		sprintf(s, "  %ld", flags);
		strcat(str, s);
	}
}

void
prbpb(BPB *bpb)
{
	char str[256];

	if(bpb && bpb->recsiz) {
		sprintf(str, "  recsiz %u  clsiz %u  clsizb %u  rdlen %u",
			bpb->recsiz, bpb->clsiz, bpb->clsizb, bpb->rdlen);
		sysprintf(str);
		sprintf(str, "  fsiz %u  fatrec %u  datrec %u  numcl %u",
			bpb->fsiz, bpb->fatrec, bpb->datrec, bpb->numcl);
		sysprintf(str);
		sprintf(str, "  bflags %u", bpb->bflags);
		sysprintf(str);
	}
}

static long
cookieptr()
{
	return *((long *)0x5a0);
}


static int
setcookie(long cookie, ULONG newValue, ULONG *oldValue)
{
	long *cookiejar = (long *)Supexec(cookieptr);

	if(!cookiejar) return 0;

	do {
		if(cookiejar[0] == cookie) {
			*oldValue = (ULONG)cookiejar[1];
			if(newValue) cookiejar[1] = newValue;
			return 1;
		}
		else
			cookiejar = &(cookiejar[2]);
	} while(cookiejar[-2]);

	return 0;
}
