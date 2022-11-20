#ifndef __util_h
#define __util_h

int getCookie(long, void *);
void setCookie(long, unsigned long);
long cdecl nfId(const char *);
long cdecl nfCall(long, ...);
int cdecl nfDetect(void);

#endif