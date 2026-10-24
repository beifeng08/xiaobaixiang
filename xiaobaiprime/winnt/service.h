// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#ifndef _SERVICE_H
#define _SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#define SZAPPNAME            "NTPrime"

extern char SZSERVICENAME[32];

extern char SZSERVICEDISPLAYNAME[32];

#define SZDEPENDENCIES       ""

VOID ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
VOID ServiceStop();

BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

void AddToMessageLog(LPTSTR lpszMsg);

#ifdef __cplusplus
}
#endif

#endif
