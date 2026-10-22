// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

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
