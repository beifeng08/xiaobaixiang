// MainFrm.cpp : implementation of the CMainFrame class
// Copyright 1995-2023 Mersenne Research, Inc.  All rights reserved
//

#include "stdafx.h"
#include "XiaobaiPrime.h"

#include "MainFrm.h"
#include "XiaobaiPrimeDoc.h"
#include "XiaobaiPrimeView.h"

#include <winreg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const UINT WM_TASKBARCREATED = ::RegisterWindowMessage(_T("TaskbarCreated"));

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ENDSESSION()
	ON_WM_ACTIVATEAPP()
	
	ON_WM_SYSCOMMAND()
	
	ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(IDM_TRAY_OPEN, OnTrayOpenWindow)
	ON_COMMAND(IDM_STOP_CONTINUE, OnStopContinue)
	ON_COMMAND(ID_WINDOW_TILE_HORZ, OnTile)
	ON_COMMAND(ID_WINDOW_POSITION, OnPosition)
	ON_MESSAGE(WM_POWERBROADCAST, OnPower)
	ON_MESSAGE(MYWM_TRAYMESSAGE, OnTrayMessage)
	ON_REGISTERED_MESSAGE(WM_TASKBARCREATED, OnTaskBarCreated)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

CMainFrame::CMainFrame()
{
	
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	
	return CMDIFrameWnd::PreCreateWindow(cs);
}

void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      
	}

	return 0;
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif 

LRESULT CMainFrame::OnPower(WPARAM uID, LPARAM uMouseMsg)
{

	if (uID == PBT_APMPOWERSTATUSCHANGE) {
		if (is_timed_event_active (TE_BATTERY_CHECK)) test_battery ();
	}
	return 0;
}

LRESULT CMainFrame::OnTrayMessage(WPARAM uID, LPARAM uMouseMsg)
{
	if (uID == 352 && uMouseMsg == WM_LBUTTONDBLCLK) {
		if (IsWindowVisible())
			ShowWindow(FALSE);	
		else {
			ShowWindow(TRUE);	
			SetForegroundWindow();
		}
	}

	if (uID == 352 && uMouseMsg == WM_RBUTTONUP) {
		POINT	point;
		HMENU	hMenu, hSubMenu;
		MENUITEMINFO m;

		GetCursorPos (&point);		
		hMenu = LoadMenu (AfxGetApp()->m_hInstance,
				  MAKEINTRESOURCE (IDR_TRAYMENU));
		hSubMenu = GetSubMenu (hMenu, 0);
		m.cbSize = sizeof (m);
		if (!IsIconic ()) {
			m.fMask = MIIM_STRING;
			m.fType = MFT_STRING;
			m.dwTypeData = "Minimize Window";
			m.cch = (UINT) strlen (m.dwTypeData);
			SetMenuItemInfo (hSubMenu, IDM_TRAY_OPEN, FALSE, &m);
		}
		if (!WORKERS_ACTIVE) {
			m.fMask = MIIM_STRING;
			m.fType = MFT_STRING;
			m.dwTypeData = "Continue";
			m.cch = (UINT) strlen (m.dwTypeData);
			SetMenuItemInfo (hSubMenu, IDM_STOP_CONTINUE, FALSE, &m);
		}
		if ((WORKERS_ACTIVE && WORKERS_STOPPING) ||
		    (!WORKERS_ACTIVE &&
		     !USE_PRIMENET && WORKTODO_COUNT == 0)) {
			m.fMask = MIIM_STATE;
			m.fState = MFS_DISABLED;
			SetMenuItemInfo (hSubMenu, IDM_STOP_CONTINUE, FALSE, &m);
		}
		SetMenuDefaultItem (hSubMenu, IDM_TRAY_OPEN, FALSE);
		SetForegroundWindow ();	
		TrackPopupMenu (hSubMenu,
			TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
			point.x, point.y, 0, m_hWnd, NULL);
		PostMessage (WM_NULL, 0, 0);
		DestroyMenu (hMenu);
	}

	return 0;
}

LRESULT CMainFrame::OnTaskBarCreated (WPARAM wp, LPARAM lp)
{
	if (!HIDE_ICON)
		((CXiaobaiPrimeApp *)AfxGetApp())->TrayMessage (NIM_ADD, "XiaobaiPrime", 0);
	return 0;
}

void CMainFrame::OnTrayOpenWindow()
{
	if (IsIconic ())
		PostMessage (WM_SYSCOMMAND, SC_RESTORE, 0L);
	else
		PostMessage (WM_SYSCOMMAND, SC_MINIMIZE, 0L);
}

void CMainFrame::OnStopContinue()
{
	if (WORKERS_ACTIVE)
		PostMessage (WM_COMMAND, IDM_STOP, 0);
	else
		PostMessage (WM_COMMAND, IDM_CONTINUE, 0);
}

BOOL CMainFrame::DestroyWindow() 
{
	if (TRAY_ICON) ((CXiaobaiPrimeApp *)AfxGetApp())->TrayMessage (NIM_DELETE, NULL, 0);
	return CMDIFrameWnd::DestroyWindow();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
static	int first_sizing = 0;

	if (nType == SIZE_MINIMIZED) {
		if (TRAY_ICON) ShowWindow (FALSE);
		if (HIDE_ICON) ShowWindow (FALSE);
	}
	CMDIFrameWnd::OnSize(nType, cx, cy);

	if (nType != SIZE_MINIMIZED && first_sizing < 2) {
		first_sizing++;
		PostMessage (WM_COMMAND, ID_WINDOW_POSITION, 0);
	}
}

LRESULT CMainFrame::WindowProc (UINT message, WPARAM wParam, LPARAM lParam)
{
	WM_ENDSESSION_LPARAM = (LONG) lParam;
	return CMDIFrameWnd::WindowProc(message, wParam, lParam);
}

static const TCHAR szAfxOldWndProc[] = _T("AfxOldWndProc423");
BOOL CALLBACK EnumProc (HWND hWnd, LPARAM lParam)
{
	
	WNDPROC oldWndProc = (WNDPROC)::GetProp (hWnd, szAfxOldWndProc);
	if (oldWndProc!=NULL) {
		SetWindowLongPtr (hWnd, GWLP_WNDPROC, (LONG_PTR) oldWndProc);
		RemoveProp (hWnd, szAfxOldWndProc);
	}
	return TRUE;
}

void CMainFrame::OnEndSession (BOOL bEnding)
{
	CMDIFrameWnd::OnEndSession (bEnding);
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD hTask) 
{
	CMDIFrameWnd::OnActivateApp(bActive, hTask);
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == SC_CLOSE && ! IniSectionGetInt (INI_FILE, SEC_Windows, KEY_ExitOnX, 0) && (TRAY_ICON || HIDE_ICON)) {
		SendMessage (WM_SYSCOMMAND, SC_MINIMIZE, 0);
		return;
	}
	CMDIFrameWnd::OnSysCommand (nID, lParam);
}

void CMainFrame::OnTile()
{

	MDITile (MDITILE_HORIZONTAL);

	PositionViews (TRUE);
}

void CMainFrame::OnPosition()
{

	MDITile (MDITILE_HORIZONTAL);

	PositionViews (FALSE);
}

