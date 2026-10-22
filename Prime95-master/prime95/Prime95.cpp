// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "Prime95.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "Prime95Doc.h"
#include "Prime95View.h"
#include "hyperlink.h"

#include <aclapi.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HANDLE	g_hMutexInst = NULL;
LONG	g_MutexNum = 0;
BOOL CALLBACK MyEnumProc (
	HWND	hwnd,	
	LPARAM	lParam)	
{

	if (GetWindowLongPtr (hwnd, GWLP_USERDATA) != g_MutexNum)
		return (TRUE);

	* (HWND *) lParam = hwnd;
	return (FALSE);
}

BEGIN_MESSAGE_MAP(CPrime95App, CWinApp)
	
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

CPrime95App::CPrime95App()
{
	EnableHtmlHelp ();

}

CPrime95App theApp;

BOOL CPrime95App::InitInstance()
{
	int	orig_cmdShow;
	int	named_ini_files = -1;
	int	torture_test = 0;
	char	*p;

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance ();

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_MDITYPE,
		RUNTIME_CLASS(CPrime95Doc),
		RUNTIME_CLASS(CChildFrame),       
		RUNTIME_CLASS(CPrime95View));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	gwmutex_init (&VIEW_MUTEX);
	gwmutex_init (&VIEW_LINES_MUTEX);

	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	orig_cmdShow = m_nCmdShow;
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	{
		char	buf[256];
		GetModuleFileName (NULL, buf, sizeof (buf));
		strrchr (buf, '\\')[1] = 0;
		_chdir (buf);
	}

	StopCheckRoutine = stopCheck;
	OutputBothRoutine = OutputBoth;

	for (p = m_lpCmdLine; *p == '//' || *p == '-'; ) {
		p++;
		switch (*p++) {

		case 'A':
		case 'a':
			named_ini_files = 0;
			while (isspace (*p)) p++;
			while (isdigit (*p)) {
				named_ini_files = named_ini_files * 10 + (*p - '0');
				p++;
			}
			break;

		case 'T':
		case 't':
			torture_test = 1;
			break;

		case 'W':
		case 'w':
			{
			char	buf[256];
			char	*bufp = buf;
			while (isspace (*p)) p++;
			while (*p && !isspace (*p)) *bufp++ = *p++;
			*bufp = 0;
			_chdir (buf);
			}
			break;
		}

		while (isspace (*p)) p++;
	}

	{
		char	buf[256];
		char	*p;
		DWORD mutex_error_code;
		PSID pEveryoneSID = NULL, pAdminSID = NULL;
		PACL pACL = NULL;
		PSECURITY_DESCRIPTOR pSD = NULL;
		EXPLICIT_ACCESS ea[1];
		SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
		SECURITY_ATTRIBUTES sa;

		_getcwd (buf, 255);
		for (p = buf; *p; p++)
			g_MutexNum = g_MutexNum * 17 + *p;
		g_MutexNum += named_ini_files;
		sprintf (buf, "Global\\GIMPS%ld", g_MutexNum);

		if (! AllocateAndInitializeSid (
				&SIDAuthWorld, 1, SECURITY_WORLD_RID,
				0, 0, 0, 0, 0, 0, 0, &pEveryoneSID))
			goto simple_mutex;

		ZeroMemory (&ea, sizeof (EXPLICIT_ACCESS));
		ea[0].grfAccessPermissions = EVENT_ALL_ACCESS;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance= NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

		if (SetEntriesInAcl (1, ea, NULL, &pACL) != ERROR_SUCCESS)
			goto simple_mutex;

		pSD = (PSECURITY_DESCRIPTOR)
			LocalAlloc (LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
		if (pSD == NULL) goto simple_mutex;
		if (! InitializeSecurityDescriptor (pSD, SECURITY_DESCRIPTOR_REVISION))
			goto simple_mutex;

		if (! SetSecurityDescriptorDacl (pSD, TRUE, pACL, FALSE))
			goto simple_mutex;

		sa.nLength = sizeof (SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = pSD;
		sa.bInheritHandle = FALSE;

		g_hMutexInst = CreateMutex (
			&sa,   
			FALSE, 
			buf);  
		if (g_hMutexInst == NULL)
simple_mutex:	 	g_hMutexInst = CreateMutex (
				NULL,  
				FALSE, 
				buf+7);  
		mutex_error_code = GetLastError ();

		if (pEveryoneSID) FreeSid (pEveryoneSID);
		if (pACL) LocalFree (pACL);
	        if (pSD) LocalFree (pSD);

		if (g_hMutexInst == NULL)
			return 0;

		if (mutex_error_code == ERROR_ALREADY_EXISTS) {
			HWND	hwndPrevInst = 0;

			Sleep (750);

			EnumWindows (&MyEnumProc, (LPARAM) &hwndPrevInst);

			if (hwndPrevInst) {
				ShowWindow (hwndPrevInst, SW_HIDE);
				ShowWindow (hwndPrevInst, SW_SHOWMINIMIZED);
				ShowWindow (hwndPrevInst, SW_SHOWNORMAL);
			}
			CloseHandle (g_hMutexInst);
			g_hMutexInst = NULL;
			return 0;
		}

		SetWindowLongPtr (m_pMainWnd->m_hWnd, GWLP_USERDATA, (LONG_PTR) g_MutexNum);
	}

	ICON_IDLE = LoadIcon (IDI_YELLOW_ICON);
	ICON_WORKING = LoadIcon (IDR_MAINFRAME);

	nameAndReadIniFiles (named_ini_files);
	initCommCode ();

	m_pMainWnd->SetWindowText ("Prime95");

	WINDOWPLACEMENT wp;
	m_pMainWnd->GetWindowPlacement (&wp);
	int left = IniSectionGetInt (INI_FILE, SEC_Windows, KEY_Left, 0);
	int top = IniSectionGetInt (INI_FILE, SEC_Windows, KEY_Top, 0);
	int right = IniSectionGetInt (INI_FILE, SEC_Windows, KEY_Right, 0);
	int bottom = IniSectionGetInt (INI_FILE, SEC_Windows, KEY_Bottom, 0);
	if (right + left + top + bottom != 0) {
		wp.rcNormalPosition.left = left;
		wp.rcNormalPosition.top = top;
		wp.rcNormalPosition.right = right;
		wp.rcNormalPosition.bottom = bottom;
	}
	wp.showCmd = HIDE_ICON ? SW_HIDE : SW_SHOWMINIMIZED;
	m_pMainWnd->SetWindowPlacement (&wp);

	if (TRAY_ICON) TrayMessage (NIM_ADD, "Prime95", 0);

	if (torture_test) {
		m_pMainWnd->ShowWindow (orig_cmdShow);
		m_pMainWnd->PostMessage (WM_COMMAND, USR_TORTURE, 0);
	}

	else if (STRESS_TESTER == 99) {
		m_pMainWnd->ShowWindow (orig_cmdShow);
		m_pMainWnd->PostMessage (WM_COMMAND, USR_WELCOME, 0);
	}

	else if (STRESS_TESTER) {
		m_pMainWnd->ShowWindow (orig_cmdShow);
		m_pMainWnd->PostMessage (WM_COMMAND, IDM_TORTURE, 0);
	}

	else if (USE_PRIMENET || WELL_BEHAVED_WORK || WORKTODO_COUNT) {
		m_pMainWnd->PostMessage (WM_COMMAND, IDM_CONTINUE, 0);
	}

	else if (!HIDE_ICON) {
		m_pMainWnd->ShowWindow (orig_cmdShow);
		ChangeIcon (MAIN_THREAD_NUM, IDLE_ICON);
	}

	return TRUE;
}

int CPrime95App::ExitInstance() 
{
	if( NULL != g_hMutexInst ) {
		CloseHandle( g_hMutexInst );
	}
	
	return CWinApp::ExitInstance();
}

void CPrime95App::TrayMessage (UINT message, LPCSTR prompt, HICON icon)
{
	NOTIFYICONDATA tnd;

	switch (message) {
	case NIM_ADD :
		WINDOWS95_TRAY_ADD = 0;
		tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		break;
	case NIM_MODIFY :
		tnd.uFlags = 0;
		if (prompt != NULL) tnd.uFlags |= NIF_TIP;
		if (icon) tnd.uFlags |= NIF_ICON;
		break;
	case NIM_DELETE :
		tnd.uFlags = 0;
	}

	if (prompt)
		truncated_strcpy (tnd.szTip, sizeof (tnd.szTip), prompt);
	else
		lstrcpy (tnd.szTip, "Prime95");

	tnd.uID = 352;
	tnd.cbSize = sizeof(tnd);
	tnd.hWnd = m_pMainWnd->m_hWnd;
	tnd.uCallbackMessage = MYWM_TRAYMESSAGE;
	tnd.hIcon = icon ? icon : ICON_IDLE;

	if (Shell_NotifyIcon(message, &tnd)) return;

	if (message == NIM_ADD)
		WINDOWS95_TRAY_ADD = 1;
}

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };
	CHyperLink m_web_site_link;
	CString m_app_string;
	
	protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:
	
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnStnClickedLink();
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_LINK, m_web_site_link);
	DDX_Text(pDX, IDC_APP_STRING, m_app_string);
	
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_web_site_link.SetURL(_T("http://mersenne.org/"));
	
	return TRUE;  
}

void CPrime95App::OnAppAbout()
{
	CAboutDlg aboutDlg;
	char	app_string[120];

	generate_application_string (app_string);
	aboutDlg.m_app_string = app_string;
	aboutDlg.DoModal();
}

int	EXIT_IN_PROGRESS = 0;

LONG	WM_ENDSESSION_LPARAM = 0;
int	WINDOWS95_TRAY_ADD = 0;

HICON	ICON_IDLE = 0;			
HICON	ICON_WORKING = 0;		

void CAboutDlg::OnStnClickedLink()
{
    
}
