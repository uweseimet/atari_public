/*****************************/
/* SCSI Driver/Firmware Test */
/*                           */
/* (C) 2014-2026 Uwe Seimet  */
/*****************************/

#ifndef __util_h
#define __util_h

tpScsiCall GetScsiDriver(const char *);
tHandle GetHandle(tpScsiCall, UWORD *, ULONG *, UWORD *);
bool Inquiry(tpScsiCall, tpSCSICmd, UWORD);
bool getCookie(LONG, ULONG *);

#endif
