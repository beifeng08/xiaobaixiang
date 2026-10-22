// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "XiaobaiPrime.h"

#include "afxpriv.h"
#include "ChildFrm.h"
#include "MainFrm.h"
#include "XiaobaiPrimeDoc.h"
#include "XiaobaiPrimeView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_VIEWS	(MAX_NUM_WORKERS+2)	

CXiaobaiPrimeView *Views[MAX_VIEWS] = {0};
char	ThreadTitles[MAX_VIEWS][80] = {0};
gwmutex	VIEW_MUTEX;		
gwmutex	VIEW_LINES_MUTEX;	
int	ViewNum = 2;		
int	NumViews = 0;
int	charHeight = 0;
int	charWidth = 0;

IMPLEMENT_DYNCREATE(CXiaobaiPrimeView, CScrollView)

BEGIN_MESSAGE_MAP(CXiaobaiPrimeView, CScrollView)
	
	ON_COMMAND(USR_SCROLL, OnScroll)
	ON_COMMAND(USR_TITLE, OnTitle)
	ON_COMMAND(USR_ICON, OnIcon)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
		
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CXiaobaiPrimeView::CXiaobaiPrimeView()
{
	int	i;

	BaseTitle[0] = 0;
	Title[0] = 0;
	for (i = 0; i < MAX_VIEW_LINES; i++) Lines[i] = LineData[i];
	NumLines = 0;
	MaxLineSize = 0;
	Lines[0][0] = 0;

	Views[ViewNum] = this;
	NumViews++;

	SetScrollSizes (MM_TEXT, CSize (0, 0));
}

CXiaobaiPrimeView::~CXiaobaiPrimeView()
{
	NumViews--;
}

BOOL CXiaobaiPrimeView::PreCreateWindow(CREATESTRUCT& cs)
{
	
	cs.style &= ~FWS_ADDTOTITLE;	
	return CScrollView::PreCreateWindow(cs);
}

void CXiaobaiPrimeView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CFrameWnd *parent;
	CSize	sz;
	CPoint	pos;
	int	new_scroll_height, new_scroll_width;

	parent = GetParentFrame();

	if (parent != NULL) {

		if (charHeight == 0) getCharSize ();

		sz = GetTotalSize ();

		new_scroll_height = NumLines * charHeight;
		new_scroll_width = MaxLineSize * charWidth;

		pos = GetScrollPosition ();
		pos.y += (new_scroll_height - sz.cy);
		if (pos.y < 0) pos.y = 0;
		sz.cx = new_scroll_width;
		sz.cy = new_scroll_height;
		SetScrollSizes (MM_TEXT, sz);
		ScrollToPosition (pos);
		parent->RecalcLayout ();
	}

	CScrollView::OnUpdate (pSender, lHint, pHint);
}

void CXiaobaiPrimeView::OnDraw(CDC* pDC)
{
	RECT	r;
	CPoint	scroll_offset;
	int	ypos;
	int	first_line, skip_lines, i, j;

	pDC->SetBkMode (TRANSPARENT);
	pDC->SetTextColor (GetSysColor (COLOR_WINDOWTEXT));

	GetClientRect (&r);
	scroll_offset = GetScrollPosition ();

	gwmutex_lock (&VIEW_LINES_MUTEX);
	first_line = Lines[0][0] ? NumLines - 1 : NumLines;
	skip_lines = (r.top + scroll_offset.y) / charHeight;
	if (skip_lines < NumLines) {
		i = first_line - skip_lines;
		j = NumLines - skip_lines;
		ypos = skip_lines * charHeight;
		for ( ; j; i--, j--) {
			pDC->TextOut (0, ypos, Lines[i], (int) strlen (Lines[i]));
			ypos += charHeight;
			if (ypos > r.bottom + scroll_offset.y) break;
		}
	}
	gwmutex_unlock (&VIEW_LINES_MUTEX);
}

#ifdef _DEBUG
void CXiaobaiPrimeView::AssertValid() const
{

}

void CXiaobaiPrimeView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CXiaobaiPrimeDoc* CXiaobaiPrimeView::GetDocument() 
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CXiaobaiPrimeDoc)));
	return (CXiaobaiPrimeDoc*)m_pDocument;
}
#endif 

void CXiaobaiPrimeView::OnEditCopy()
{
	int	i, j;

	CSharedFile sf(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT);

	i = Lines[0][0] ? NumLines - 1 : NumLines;
	for (j = NumLines; j; i--, j--) {
		sf.Write (Lines[i], (UINT) strlen (Lines[i]));
		sf.Write ("\r\n", 2);
	}

    	OpenClipboard ();
	EmptyClipboard ();
	SetClipboardData (CF_TEXT, sf.Detach ());
	CloseClipboard ();
}

int map_thread_num_to_view_num (
	int	thread_num)
{

	if (thread_num == MAIN_THREAD_NUM) {
		if (MERGE_WINDOWS & MERGE_MAINCOMM_WINDOWS) return (1);
		if (MERGE_WINDOWS & MERGE_MAIN_WINDOW) return (2);
	}

	if (thread_num == COMM_THREAD_NUM && MERGE_WINDOWS & MERGE_COMM_WINDOW)
		return (2);

	if (thread_num >= 0 && MERGE_WINDOWS & MERGE_WORKER_WINDOWS)
		return (2);

	return (thread_num+2);
}

CXiaobaiPrimeView *map_thread_num_to_view (
	int	thread_num)
{
	return (Views[map_thread_num_to_view_num (thread_num)]);
}

void create_window (
	int	thread_num)
{
	gwmutex_lock (&VIEW_MUTEX);
	ViewNum = map_thread_num_to_view_num (thread_num);
	if (Views[ViewNum] == NULL) {
		AfxGetApp()->m_pMainWnd->SendMessage (WM_COMMAND, ID_WINDOW_NEW, 0);
		PositionViews (FALSE);
	}
	gwmutex_unlock (&VIEW_MUTEX);
}

void destroy_window (
	int	thread_num)
{
	CXiaobaiPrimeView *view;

	gwmutex_lock (&VIEW_MUTEX);
	view = map_thread_num_to_view (thread_num);
	if (view != NULL) view->GetParentFrame()->DestroyWindow ();
	gwmutex_unlock (&VIEW_MUTEX);
}

void CXiaobaiPrimeView::OnTitle ()
{
	char	buf[120];
	CXiaobaiPrimeApp* pApp = (CXiaobaiPrimeApp *) AfxGetApp();
static	int	was_iconic = TRUE;

	if (EXIT_IN_PROGRESS) return;

	if (BaseTitle[0] && Title[0])
		sprintf (buf, "%s - %s", BaseTitle, Title);
	else if (Title[0])
		strcpy (buf, Title);
	else
		strcpy (buf, BaseTitle);

	GetParent()->SetWindowText (buf);

	if (pApp->m_pMainWnd &&
	    (! WORKERS_ACTIVE ||
	     (this != Views[0] && this != Views[1]))) {
		sprintf (buf, "XiaobaiPrime - %s", Title);
		if (TRAY_ICON) {
			int	i;
			char	buf[1600];
			buf[0] = 0;
			for (i = 2; i < MAX_VIEWS; i++) {
				if (strlen (buf) > sizeof (buf) - 100) break;	
				if (buf[0] && ThreadTitles[i][0]) strcat (buf, "\n");
				strcat (buf, ThreadTitles[i]);
			}
			pApp->TrayMessage (NIM_MODIFY, buf[0] ? buf : "Not running", 0);
		}
		if (pApp->m_pMainWnd->IsIconic ()) {
			pApp->m_pMainWnd->SetWindowText (buf);
			was_iconic = TRUE;
		} else if (was_iconic) {
			pApp->m_pMainWnd->SetWindowText ("XiaobaiPrime");
			was_iconic = FALSE;
		}
	}

	OnUpdate (NULL, 0, NULL);
}

void base_title (
	int	thread_num,
	const char *str)
{
	CXiaobaiPrimeView *view;

	if (EXIT_IN_PROGRESS) return;

	if (MERGE_WINDOWS & MERGE_MAIN_WINDOW &&
	    MERGE_WINDOWS & MERGE_COMM_WINDOW &&
	    (MERGE_WINDOWS & MERGE_WORKER_WINDOWS || NUM_WORKERS == 1))
		str = "";
	else if ((thread_num == MAIN_THREAD_NUM &&
		  MERGE_WINDOWS & MERGE_MAIN_WINDOW) ||
		 (thread_num == COMM_THREAD_NUM &&
		  MERGE_WINDOWS & MERGE_COMM_WINDOW) ||
		 (thread_num >= 0 &&
		  MERGE_WINDOWS & MERGE_WORKER_WINDOWS)) {
		if (MERGE_WINDOWS & MERGE_WORKER_WINDOWS &&
		    NUM_WORKERS > 1)
			str = "Workers";
		else
			str = "Worker";
	}

	gwmutex_lock (&VIEW_MUTEX);
	view = map_thread_num_to_view (thread_num);
	if (view != NULL) view->base_title (str);
	gwmutex_unlock (&VIEW_MUTEX);
}

void CXiaobaiPrimeView::base_title (
	const char *str)
{
	strcpy (BaseTitle, str);
	PostMessage (WM_COMMAND, USR_TITLE, 0);
}

void title (
	int	thread_num,
	const char *str)
{
	CXiaobaiPrimeView *view;
	char	merged_title[160];

	if (EXIT_IN_PROGRESS) return;

	if (thread_num == COMM_THREAD_NUM && MERGE_WINDOWS & MERGE_COMM_WINDOW)
		return;

	strcpy (ThreadTitles[thread_num+2], str);
	if (thread_num == MAIN_THREAD_NUM && MERGE_WINDOWS & MERGE_MAIN_WINDOW)
		thread_num = 0;
	if (thread_num >= 0 && MERGE_WINDOWS & MERGE_WORKER_WINDOWS) {
		int	i;
		merged_title[0] = 0;
		for (i = 2; i < MAX_VIEWS; i++) {
			if (merged_title[0] && ThreadTitles[i][0])
				strcat (merged_title, ",");
			strcat (merged_title, ThreadTitles[i]);
			if (strlen (merged_title) > 80) {
				merged_title[77] = 0;
				strcat (merged_title, "...");
				break;
			}
		}
		str = merged_title[0] ? merged_title : "Not running";
	}

	gwmutex_lock (&VIEW_MUTEX);
	view = map_thread_num_to_view (thread_num);
	if (view != NULL) view->title (str);
	gwmutex_unlock (&VIEW_MUTEX);
}

void CXiaobaiPrimeView::title (
	const char *str)
{
	strcpy (Title, str);
	PostMessage (WM_COMMAND, USR_TITLE, 0);
}

void ChangeIcon (
	int	thread_num,
	int	icon_id)
{
	if (EXIT_IN_PROGRESS) return;

	if (thread_num == COMM_THREAD_NUM && MERGE_WINDOWS & MERGE_COMM_WINDOW)
		return;

	if (icon_id != -1) {
		CXiaobaiPrimeView *view;
		gwmutex_lock (&VIEW_MUTEX);
		view = map_thread_num_to_view (thread_num);
		if (view != NULL) view->ChangeIcon (icon_id);
		gwmutex_unlock (&VIEW_MUTEX);
	}

	if (thread_num == MAIN_THREAD_NUM) {
		CXiaobaiPrimeApp *app;
		static HICON main_icon = 0;
		app = (CXiaobaiPrimeApp *) AfxGetApp();
		if (icon_id == WORKING_ICON)
			main_icon = ICON_WORKING;
		else if (icon_id == IDLE_ICON || main_icon == 0)
			main_icon = ICON_IDLE;
		app->m_pMainWnd->SetIcon (main_icon, 1);
		if (TRAY_ICON) app->TrayMessage (NIM_MODIFY, NULL, main_icon);
	}
}

void CXiaobaiPrimeView::OnIcon ()
{

	if (EXIT_IN_PROGRESS) return;

#ifndef X86_64 
	GetParent()->SetIcon (icon, 1);
#endif

	OnUpdate (NULL, 0, NULL);
}

void CXiaobaiPrimeView::ChangeIcon (
	int	icon_id)
{
	icon = (icon_id == WORKING_ICON) ? ICON_WORKING : ICON_IDLE;
	PostMessage (WM_COMMAND, USR_ICON, 0);
}

void CALLBACK EXPORT TimerCallback (
	HWND	hWnd,		
	UINT	nMsg,		
	UINT_PTR nIDEvent,	
	DWORD	dwTime)		
{
	BlinkIcon (MAIN_THREAD_NUM, -2);
}

void BlinkIcon (
	int	thread_num,
	int	duration)		
					
{
static	int	state = -1;		
static	int	icon = WORKING_ICON;	

	if (state == -1) {
		if (duration < 0) return;
		AfxGetApp()->m_pMainWnd->SetTimer (363, 1000, &TimerCallback);
	}

	if (duration >= 0)
		state = duration;

	else if (duration == -1 || (state && --state == 0)) {
		AfxGetApp()->m_pMainWnd->KillTimer (363);
		state = -1;
		icon = IDLE_ICON;
	}

	icon = (icon == IDLE_ICON) ? WORKING_ICON : IDLE_ICON;
	ChangeIcon (MAIN_THREAD_NUM, icon);
}

void PositionViews (int forceTile)
{
	int	i, vnum;

	gwmutex_lock (&VIEW_MUTEX);
	for (i = vnum = 0; i < MAX_VIEWS; i++)
		if (Views[i] != NULL)
			Views[i]->position (vnum++, i, (BOOL)!!forceTile);
	gwmutex_unlock (&VIEW_MUTEX);
}

static const int P95_WP_MINIMIZED= 1;
static const int P95_WP_MAXIMIZED= 2;

BOOL getSubWindowPlacement(
	CWnd	*pwnd,
	int	vnum)
{
	BOOL	handled = FALSE;
	char	name[16];
	char	rgch[80];

	wsprintf(name, "W%d", vnum);
	rgch[0] = 0;
	IniSectionGetString (INI_FILE, SEC_Windows, name, rgch, sizeof (rgch), NULL);
	if (0 != rgch[0])
	{
		int state = 0;
		WINDOWPLACEMENT wp = {0};
		RECT *prc	= &wp.rcNormalPosition;
		POINT *pptMin= &wp.ptMinPosition;
		POINT *pptMax= &wp.ptMaxPosition;
		int count = sscanf (rgch, "%d %ld %ld %ld %ld %ld %ld %ld %ld",
				    &state, &prc->top, &prc->right, &prc->bottom, &prc->left, &pptMin->x, &pptMin->y, &pptMax->x, &pptMax->y);
		if (9 == count)
		{
			BOOL fMinimized = (0 != (state & P95_WP_MINIMIZED));
			BOOL fMaximized = (0 != (state & P95_WP_MAXIMIZED));

			wp.length = sizeof(wp);
			if (fMinimized)
			{
				wp.showCmd = SW_SHOWMINNOACTIVE;
			}
			else
			{
				wp.showCmd = (fMaximized) ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
			}
			wp.flags = (fMaximized) ? WPF_RESTORETOMAXIMIZED : WPF_SETMINPOSITION;
			if (0 != pwnd->SetWindowPlacement(&wp))
			{
				handled = TRUE;
			}
		}
	}

	return handled;
}

BOOL setSubWindowPlacement(
	CWnd	*pwnd,
	int	vnum)
{
	BOOL	handled = FALSE;
	char	name[16];
	char	rgch[80];
	WINDOWPLACEMENT wp = {0};

	wp.length = sizeof(wp);
	if (0 != pwnd->GetWindowPlacement(&wp))
	{
		RECT r = {0};
		RECT *prc	= &wp.rcNormalPosition;
		POINT *pptMin= &wp.ptMinPosition;
		POINT *pptMax= &wp.ptMaxPosition;
		int state = 0;

		handled = TRUE;

		if (wp.showCmd == SW_SHOWMAXIMIZED)
		{
			state = P95_WP_MAXIMIZED;
		}
		else if (wp.showCmd == SW_SHOWMINIMIZED)
		{
			state = P95_WP_MINIMIZED;
		}
		
		wsprintf (rgch, "%d %ld %ld %ld %ld %ld %ld %ld %ld",
			  state, prc->top, prc->right, prc->bottom, prc->left, pptMin->x, pptMin->y, pptMax->x, pptMax->y);

		wsprintf (name, "W%d", vnum);
		IniSectionWriteString (INI_FILE, SEC_Windows, name, rgch);
	}

	return handled;
}

void SaveViews (void)
{
	int	i, vnum;

	gwmutex_lock (&VIEW_MUTEX);
	for (i = vnum = 0; i < MAX_VIEWS; i++)
	{
		if (Views[i] != NULL)
		{
			setSubWindowPlacement(Views[i]->GetParent(), i);
			Views[i] = NULL;
		}
	}
	gwmutex_unlock (&VIEW_MUTEX);
}

void CXiaobaiPrimeView::position (
	int	vnum,
	int	iview,
	BOOL	forceTile)
{
	CMainFrame *mainframe;
	CRect	frame_rect;
	int	frame_width, frame_height;
	BOOL handled = FALSE;

	if (!forceTile)
	{
		handled = getSubWindowPlacement(GetParent(), iview);
	}
	if (!handled)
	{
		mainframe = (CMainFrame *) AfxGetApp()->m_pMainWnd;
		mainframe->GetClientRect (&frame_rect);
		frame_height = frame_rect.bottom - frame_rect.top;
		frame_width = frame_rect.right - frame_rect.left;

		mainframe->m_wndStatusBar.GetClientRect (&frame_rect);
		frame_height -= frame_rect.bottom - frame_rect.top;

		frame_height -= 2;
		frame_width -= 2;

		GetParent()->MoveWindow (0,
					 vnum * frame_height / NumViews,
					 frame_width,
					 (vnum+1) * frame_height / NumViews -
					  vnum * frame_height / NumViews);
	}
}

void CXiaobaiPrimeView::LineFeed ()
{
	char	*p;
	int	i, len, lines_to_size;

	lines_to_size = 1;
	if (NumLines == MAX_VIEW_LINES) {
		NumLines--;
		if (MaxLineSize == strlen (Lines[NumLines])) {
			MaxLineSize = 0;
			lines_to_size = NumLines;
		}
	}

	p = Lines[NumLines];
	memmove (&Lines[1], &Lines[0], NumLines * sizeof(char *));
	Lines[0] = p;
	*p = 0;

	for (i = 1; i <= lines_to_size; i++) {
		len = (int) strlen (Lines[i]);
		if (len > MaxLineSize) MaxLineSize = len;
	}
}

void CXiaobaiPrimeView::getCharSize ()
{
	if (charHeight == 0) {
		CDC *dc = GetDC ();
		CSize size;
		size = dc->GetTextExtent ("A", 1);
		charHeight = size.cy;
		charWidth = size.cx;
	}
}

void CXiaobaiPrimeView::OnScroll ()
{

	if (charHeight == 0) getCharSize ();
	ScrollWindow (0, -charHeight, NULL, NULL);

	OnUpdate (NULL, 0, NULL);
}

void RealOutputStr (
	int	thread_num,
	const char *str)
{
	CXiaobaiPrimeView *view;
static	int	partial_line_output[MAX_VIEWS] = {FALSE};

	if (EXIT_IN_PROGRESS) return;

	gwmutex_lock (&VIEW_MUTEX);
	view = map_thread_num_to_view (thread_num);

	if (view == NULL);

	else if (!(MERGE_WINDOWS & MERGE_NO_PREFIX) &&
		 !partial_line_output[thread_num+2] &&
		 ((thread_num == MAIN_THREAD_NUM &&
		   MERGE_WINDOWS & MERGE_MAIN_WINDOW) ||
		  (thread_num == MAIN_THREAD_NUM &&
		   MERGE_WINDOWS & MERGE_MAINCOMM_WINDOWS) ||
		  (thread_num == COMM_THREAD_NUM &&
		   MERGE_WINDOWS & MERGE_COMM_WINDOW) ||
		  (thread_num == COMM_THREAD_NUM &&
		   MERGE_WINDOWS & MERGE_MAINCOMM_WINDOWS) ||
		  (thread_num == 0 &&
		   MERGE_WINDOWS & (MERGE_MAIN_WINDOW | MERGE_COMM_WINDOW)) ||
		  (thread_num == 0 &&
		   MERGE_WINDOWS & MERGE_WORKER_WINDOWS &&
		   NUM_WORKERS > 1) ||
		  (thread_num >= 1 &&
		   MERGE_WINDOWS & MERGE_WORKER_WINDOWS))) {
		char	prefix[50];
		if (thread_num == MAIN_THREAD_NUM)
			strcpy (prefix, "[Main thread");
		else if (thread_num == COMM_THREAD_NUM)
			strcpy (prefix, "[Comm thread");
		else if (! (MERGE_WINDOWS & MERGE_WORKER_WINDOWS) ||
			 NUM_WORKERS == 1)
			strcpy (prefix, "[Work thread");
		else
			sprintf (prefix, "[Worker #%d", thread_num+1);
		if (str[0] == '[') {
			strcat (prefix, " ");
			str++;
		} else
			strcat (prefix, "] ");
		view->RealOutputStr (prefix);
		view->RealOutputStr (str);
	}

	 else
		view->RealOutputStr (str);

	partial_line_output[thread_num+2] = (str[strlen(str)-1] != '\n');

	gwmutex_unlock (&VIEW_MUTEX);
}

void CXiaobaiPrimeView::RealOutputStr (
	const char *str)
{
	char	*p;

	gwmutex_lock (&VIEW_LINES_MUTEX);
	p = Lines[0] + strlen (Lines[0]);
	for ( ; *str; str++) {
		if (*str == '\r') continue;

		if (p == Lines[0]) {
			if (NumLines++)
				PostMessage (WM_COMMAND, USR_SCROLL, 0);
		}

		if (*str == '\n') *p = 0, LineFeed (), p = Lines[0];
		else if (p - Lines[0] < 199) *p++ = *str;
	}
	*p = 0;
	gwmutex_unlock (&VIEW_LINES_MUTEX);

	OnUpdate (NULL, 0, NULL);
}

void CXiaobaiPrimeView::OnDestroy()
{
	int	i;

	gwmutex_lock (&VIEW_MUTEX);
	for (i = 0; i < MAX_VIEWS; i++)
		if (Views[i] == this) Views[i] = NULL;
	gwmutex_unlock (&VIEW_MUTEX);

	CScrollView::OnDestroy();
}
