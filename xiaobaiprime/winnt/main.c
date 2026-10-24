/* Copyright 1995-2023 Mersenne Research, Inc. */
/* Author:  George Woltman */
/* Email: woltman@alum.mit.edu */

#include <windows.h> 
#include <winnls.h>
#include <tchar.h>
#include "service.h"
#include "main.h"
#include "SysCorePrime.h"
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int DEBUGGING = FALSE;

VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, 
        NO_ERROR,              
        3000))                 
        return;

    if (!ReportStatusToSCMgr(
        SERVICE_RUNNING,       
        NO_ERROR,              
        0))                    
        return;

    LaunchWorkers (ALL_WORKERS, TRUE);

}

VOID ServiceStop()
{
	SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_NORMAL);
	raiseAllWorkersPriority ();
	stop_workers_for_escape ();
}

void GetIniSettings()
{
	nameAndReadIniFiles (-1);
	initCommCode ();

	IniGetString (INI_FILE, "ServiceName", SZSERVICENAME, sizeof (SZSERVICENAME), "NTPrimeService");
	IniGetString (INI_FILE, "DisplayName", SZSERVICEDISPLAYNAME, sizeof (SZSERVICEDISPLAYNAME), "Prime Service");
}
