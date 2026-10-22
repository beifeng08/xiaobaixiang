// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifdef _WIN64
#define X86_64
#endif

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		
#include "hwloc.h"		
#include "gmp.h"		

#include "common.h"
#include "cpuid.h"
#include "gwnum.h"
#include "gwbench.h"
#include "gwini.h"
#include "gwutil.h"
#include "commona.h"
#include "commonb.h"
#include "commonc.h"
#include "comm95b.h"
#include "comm95c.h"
#include "primenet.h"

class CPrime95App : public CWinApp
{
public:
	CPrime95App();
	void TrayMessage (UINT, LPCSTR, HICON);

	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	
	afx_msg void OnAppAbout();
		
	DECLARE_MESSAGE_MAP()
};

extern HANDLE g_hMutexInst;

extern HICON ICON_IDLE;			
extern HICON ICON_WORKING;		

extern int EXIT_IN_PROGRESS;		

extern LONG WM_ENDSESSION_LPARAM;	
extern int WINDOWS95_TRAY_ADD;		
					
extern gwmutex VIEW_MUTEX;		
extern gwmutex VIEW_LINES_MUTEX;	

UINT threadDispatch (LPVOID);
int canModifyServices ();
void Service95 ();
