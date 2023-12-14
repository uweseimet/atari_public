/******************************//* PiSCSI client library 3.00 *//*                            *//* (C) 2022-2023 Uwe Seimet   *//******************************/

#ifndef __pi_lib
#define __pi_lib

#include <std.h>
#include <scsidrv/scsidefs.h>
#include "jsmn/jsmn.h"


#define MAX_TRANSFER_LENGTH 65535L


/* Input and output are either binary, JSON or text format protobuf data.
	 Inout and output format do *not* have to be identical. */
enum protobufFormat {
	binary = 0x01,
	json = 0x02,
	text = 0x04
};


extern bool isGerman;



int parseJson(const char *, jsmntok_t **);
int findJsonNode(const char *, jsmntok_t *t, int,	const char *);
char *getJsonString(const char *, jsmntok_t *);
	
bool executeCommand(int, const char *, UWORD, int, char *, UWORD);
bool executeOperation(tpScsiCall, tSCSICmd *, int);
bool execute(bool (*)(tpScsiCall, tSCSICmd *, int), int, const char *,
	const char *[]);
int parseJson(const char *, jsmntok_t **);

int printLocalized(const char *[]);
bool getCookie(LONG, ULONG *);

void setLanguage(void);

#endif
