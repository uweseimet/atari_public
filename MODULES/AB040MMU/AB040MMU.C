/***************************************/
/* Afterburner040 PMMU fix module 1.01 */
/*                                     */
/* (C) 2022-2025 Uwe Seimet            */
/***************************************/

#define VERSION "1.01"

#include <tos.h>
#include "../modstart.h"

int getCookie(long, unsigned long *);
extern long execute(void);


int
main()
{
	unsigned long mch;
	unsigned long cpu;

	Cconws("\r\nAfterburner040 PMMU fix V" VERSION "\r\n");	Cconws("½ 2022-2023 Uwe Seimet\r\n");

	getCookie('_MCH', (unsigned long *)&mch);
	getCookie('_CPU', (unsigned long *)&cpu);

/* Note that the Afterburner's _CPU cookie value is 20 */
	if((mch >> 16) != 3 || cpu != 20) {
		Cconws("\r\nAfterburner040 PMMU fix requires a Falcon with Afterburner");

		if(isHddriverModule()) {
			Pterm0();
		}

		return -1;
	}

	Supexec(execute);

	Cconws("\r\nAfterburner040 PMMU fix successfully applied");

	if(isHddriverModule()) {
		Pterm0();
	}

	return 0;
}


long
cookieptr()
{
	return *((long *)0x5a0);
}


int
getCookie(long cookie, unsigned long *p_value)
{
	long *cookiejar = (long *)Supexec(cookieptr);

	if(!cookiejar) {
		return 0;
	}

	do {
		if(cookiejar[0] == cookie) {
			if (p_value) *p_value = (unsigned long)cookiejar[1];
			return 1;
		}
		else
			cookiejar = &(cookiejar[2]);
	} while(cookiejar[-2]);

	return 0;
}
