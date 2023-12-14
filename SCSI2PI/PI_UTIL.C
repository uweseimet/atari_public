/*******************************//* SCSI2Pi client library 3.00 *//*                             *//* (C) 2022-2023 Uwe Seimet    *//*******************************/
#define JSMN_STATIC
#include <stdio.h>
#include <tos.h>#include <string.h>#include "pi_lib.h"


bool isGerman;


int
printLocalized(const char *messages[])
{
	printf(isGerman ? messages[1] : messages[0]);
	printf("\n");

	return -1;
}

void
setLanguage()
{
  LONG old_stack = 0;
  WORD lang;
  ULONG dummy;
  SYSHDR *syshdr;

  if (!Super((void *)1l)) {
  	old_stack = Super(0l);
	}

  syshdr = *(SYSHDR **)0x4f2;
  syshdr = syshdr->os_base;
  lang = syshdr->os_palmode >> 1;

	if (getCookie('_AKP', &dummy)) {
		lang = (WORD)(dummy >> 8);
	}

  if (old_stack) {
  	Super((void *)old_stack);
	}

	switch(lang) {
		case 1:
		case 8:		lang = 1;
							break;
		case 2:
		case 7:		lang = 2;
							break;
		default:	lang = 0;
							break;
	}

  isGerman = lang == 1;
}

LONGcookieptr(){	return *((LONG *)0x5a0);}boolgetCookie(LONG cookie, ULONG *p_value){	LONG *cookiejar = (LONG *)Supexec(cookieptr);	if (!cookiejar) {
		return false;	}
	do {		if (cookiejar[0] == cookie) {			if (p_value) {
				*p_value = (ULONG)cookiejar[1];			}			return true;		}		else			cookiejar = &(cookiejar[2]);	} while(cookiejar[-2]);	return false;}