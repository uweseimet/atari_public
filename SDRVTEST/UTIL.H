/***********************************/
/* SCSI Driver/Firmware Test 3.00ž */
/*                                 */
/* (C) 2014-2026 Uwe Seimet        */
/***********************************/

#ifndef __util_h
#define __util_h

UWORD ScanBuses(tBusInfo *, tpScsiCall);
WORD SortBuses(const void *, const void *);
bool getCookie(LONG, ULONG *);

#endif
