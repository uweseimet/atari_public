/*****************************/
/* SCSI Driver/Firmware Test */
/*                           */
/* (C) 2014-2026 Uwe Seimet  */
/*****************************/

#ifndef __util_h
#define __util_h

tHandle GetHandle(tpScsiCall, UWORD *, ULONG *, UWORD *);
bool getCookie(LONG, ULONG *);

#endif
