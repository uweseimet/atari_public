/*************************//* RaSCSI client library *//*                       *//* (C) 2022 Uwe Seimet   *//*************************/
#ifndef __ra_lib
#define __ra_lib

extern bool isGerman;

int execute(bool (*)(tpScsiCall, tSCSICmd *, int), int, const char *,
	const char *, const char *, const char *);
void setLanguage(void);

#endif
