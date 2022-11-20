/******************************//* RaSCSI client library 1.01 *//*                            *//* (C) 2022 Uwe Seimet        *//******************************/

#ifndef __ra_lib
#define __ra_lib

extern bool isGerman;

int execute(bool (*)(tpScsiCall, tSCSICmd *, int), int, const char *,
	const char *, const char *);
void setLanguage(void);

#endif
