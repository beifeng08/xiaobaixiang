// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#define MAX_THREAD_HANDLES	1024
HANDLE	THREAD_HANDLES[MAX_THREAD_HANDLES] = {0};
int	NUM_THREAD_HANDLES = 0;

void clearThreadHandleArray (void)
{
	NUM_THREAD_HANDLES = 0;
}

void setOsThreadPriority (
	int	priority)		
{
	HANDLE	h;
	int	pri_class, thread_pri;

	h = GetCurrentThread ();
	if (NUM_THREAD_HANDLES < MAX_THREAD_HANDLES)
		THREAD_HANDLES[NUM_THREAD_HANDLES++] = h;

	if (priority <= 1) pri_class = NORMAL_PRIORITY_CLASS, thread_pri = THREAD_PRIORITY_IDLE;
	if (priority == 2) pri_class = IDLE_PRIORITY_CLASS, thread_pri = THREAD_PRIORITY_LOWEST;
	if (priority == 3) pri_class = IDLE_PRIORITY_CLASS, thread_pri = THREAD_PRIORITY_BELOW_NORMAL;
	if (priority == 4) pri_class = BELOW_NORMAL_PRIORITY_CLASS, thread_pri = THREAD_PRIORITY_LOWEST;
	if (priority == 5) pri_class = BELOW_NORMAL_PRIORITY_CLASS, thread_pri = THREAD_PRIORITY_BELOW_NORMAL;
	if (priority == 6) pri_class = NORMAL_PRIORITY_CLASS, thread_pri = THREAD_PRIORITY_LOWEST;
	if (priority == 7) pri_class = NORMAL_PRIORITY_CLASS, thread_pri = THREAD_PRIORITY_BELOW_NORMAL;
	if (priority == 8) pri_class = ABOVE_NORMAL_PRIORITY_CLASS, thread_pri = THREAD_PRIORITY_LOWEST;
	if (priority >= 9) pri_class = ABOVE_NORMAL_PRIORITY_CLASS, thread_pri = THREAD_PRIORITY_BELOW_NORMAL;

	SetPriorityClass (GetCurrentProcess (), pri_class);
	SetThreadPriority (h, thread_pri);

	SetThreadPriorityBoost (h, TRUE);
}

void registerThreadTermination (void)
{
	int	i;
	HANDLE	h;

	h = GetCurrentThread ();
	for (i = 0; i < NUM_THREAD_HANDLES; i++) {
		if (THREAD_HANDLES[i] != h) break;
		THREAD_HANDLES[i] = THREAD_HANDLES[--NUM_THREAD_HANDLES];
		break;
	}
}

void raiseAllWorkersPriority (void)
{
	int	i;

	SetPriorityClass (GetCurrentProcess (), NORMAL_PRIORITY_CLASS);

	for (i = 0; i < NUM_THREAD_HANDLES; i++) {
		SetThreadPriority (THREAD_HANDLES[i], THREAD_PRIORITY_ABOVE_NORMAL);
		SetThreadAffinityMask (THREAD_HANDLES[i], -1);
	}
}

#if 0 

#include <psapi.h>
#include <tlhelp32.h>

void checkPauseListCallback (void)
{
	OSVERSIONINFO	osver;
	BOOL		bFlag;
	LPDWORD		lpdwPIDs = NULL;
	DWORD		dwSize, dwSize2, dwIndex;
	char		szFileName[ MAX_PATH ];

	osver.dwOSVersionInfoSize = sizeof( osver );
	if ( !GetVersionEx( &osver ) ) return;

	if ( osver.dwPlatformId == VER_PLATFORM_WIN32_NT ) {

		dwSize2 = 256 * sizeof (DWORD);
		do {
			if (lpdwPIDs) {
				HeapFree (GetProcessHeap(), 0, lpdwPIDs);
				dwSize2 *= 2;
			}
			lpdwPIDs = (LPDWORD)
				HeapAlloc (GetProcessHeap(), 0, dwSize2);
			if (lpdwPIDs == NULL) goto ntdone;
			if (!EnumProcesses (lpdwPIDs, dwSize2, &dwSize)) goto ntdone;
		} while (dwSize == dwSize2);

		dwSize /= sizeof (DWORD);

		for (dwIndex = 0; dwIndex < dwSize; dwIndex++) {
			HMODULE		hMod;
			HANDLE		hProcess;

			hProcess = OpenProcess(
				PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				FALSE, lpdwPIDs[ dwIndex ] );
			if (!hProcess) continue;

			if (!EnumProcessModules (hProcess, &hMod, sizeof (hMod), &dwSize2)) {
				CloseHandle (hProcess);
				continue;
			}

			if (!GetModuleFileNameEx (hProcess, hMod, szFileName, sizeof (szFileName))) {
				CloseHandle (hProcess);
				continue;
			}

			CloseHandle (hProcess);

			isInPauseList (szFileName);
		}
ntdone:		if (lpdwPIDs) HeapFree (GetProcessHeap(), 0, lpdwPIDs);
		return;
	}

	else if (osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		HANDLE		hSnapShot = NULL;
		PROCESSENTRY32	procentry;

		hSnapShot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
		if (hSnapShot == INVALID_HANDLE_VALUE) {
			hSnapShot = NULL;
			goto done95;
		}

		procentry.dwSize = sizeof (PROCESSENTRY32);
		bFlag = Process32First (hSnapShot, &procentry);

		while (bFlag) {

			isInPauseList (procentry.szExeFile);

			procentry.dwSize = sizeof(PROCESSENTRY32);
			bFlag = Process32Next (hSnapShot, &procentry);
		}

done95:		if (hSnapShot) CloseHandle (hSnapShot);
		return;
	}

	return;
}

#else

#include <tlhelp32.h>
#include <psapi.h>

void checkPauseListCallback (void)
{

	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap != INVALID_HANDLE_VALUE) {

		pe32.dwSize = sizeof (PROCESSENTRY32);
		if (Process32First (hProcessSnap, &pe32)) do {
			isInPauseList (pe32.szExeFile);
		} while (Process32Next (hProcessSnap, &pe32));

		CloseHandle( hProcessSnap );
	}

	{
	LPDWORD		lpdwPIDs = NULL;
	DWORD		dwSize, dwSize2, dwIndex;
	char		szFileName[ MAX_PATH ];

	dwSize2 = 256 * sizeof (DWORD);
	do {
		if (lpdwPIDs) {
			HeapFree (GetProcessHeap(), 0, lpdwPIDs);
			dwSize2 *= 2;
		}
		lpdwPIDs = (LPDWORD)
			HeapAlloc (GetProcessHeap(), 0, dwSize2);
		if (lpdwPIDs == NULL) goto ntdone;
		if (!EnumProcesses (lpdwPIDs, dwSize2, &dwSize))
			goto ntdone;
	} while (dwSize == dwSize2);

	dwSize /= sizeof (DWORD);

	for (dwIndex = 0; dwIndex < dwSize; dwIndex++) {
		HMODULE	hMod;
		HANDLE	hProcess;

		hProcess = OpenProcess (
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE, lpdwPIDs[ dwIndex ] );
		if (!hProcess) continue;

		if (!EnumProcessModules (hProcess, &hMod, sizeof (hMod), &dwSize2)) {
			CloseHandle (hProcess);
			continue;
		}

		if (!GetModuleFileNameEx (hProcess, hMod, szFileName, sizeof (szFileName))) {
			CloseHandle (hProcess);
			continue;
		}

		CloseHandle (hProcess);

		isInPauseList (szFileName);
	}
ntdone:	if (lpdwPIDs) HeapFree (GetProcessHeap(), 0, lpdwPIDs);
	}
}

#endif

double get_load_average (void)
{
	return (-1.0);
}

void mallocFreeForOS () {
}

