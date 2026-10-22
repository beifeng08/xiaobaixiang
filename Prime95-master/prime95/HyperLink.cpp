// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "HyperLink.h"

#include "atlconv.h"    

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TOOLTIP_ID 1

CHyperLink::CHyperLink()
{
    m_hLinkCursor       = NULL;                 
    m_crLinkColour      = RGB(  0,   0, 238);   
    m_crVisitedColour   = RGB( 85,  26, 139);   
    m_crHoverColour     = RGB(255,   0,   0);   
    m_bOverControl      = FALSE;                
    m_bVisited          = FALSE;                
    m_nUnderline        = ulHover;              
    m_bAdjustToFit      = TRUE;                 
    m_strURL.Empty();
    m_nTimerID          = 100;
}

CHyperLink::~CHyperLink()
{
    m_UnderlineFont.DeleteObject();
}

BOOL CHyperLink::DestroyWindow() 
{
    KillTimer(m_nTimerID);
	
	return CStatic::DestroyWindow();
}

BOOL CHyperLink::PreTranslateMessage(MSG* pMsg) 
{
    m_ToolTip.RelayEvent(pMsg);
    return CStatic::PreTranslateMessage(pMsg);
}

void CHyperLink::PreSubclassWindow() 
{
    
    DWORD dwStyle = GetStyle();
    ::SetWindowLong(GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);
    
    if (m_strURL.IsEmpty())
        GetWindowText(m_strURL);

    CString strWndText;
    GetWindowText(strWndText);
    if (strWndText.IsEmpty()) 
    {
        ASSERT(!m_strURL.IsEmpty());    
        SetWindowText(m_strURL);
    }

	CFont* pFont = GetFont();
	if (!pFont)
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (hFont == NULL)
			hFont = (HFONT) GetStockObject(ANSI_VAR_FONT);
		if (hFont)
			pFont = CFont::FromHandle(hFont);
	}
	ASSERT(pFont->GetSafeHandle());

    LOGFONT lf;
    pFont->GetLogFont(&lf);
	m_StdFont.CreateFontIndirect(&lf);
    lf.lfUnderline = (BYTE) TRUE;
    m_UnderlineFont.CreateFontIndirect(&lf);

    PositionWindow();        
    SetDefaultCursor();      
    SetUnderline();

    CRect rect; 
    GetClientRect(rect);
    m_ToolTip.Create(this);
    m_ToolTip.AddTool(this, m_strURL, rect, TOOLTIP_ID);

    CStatic::PreSubclassWindow();
}

BEGIN_MESSAGE_MAP(CHyperLink, CStatic)
    
    ON_WM_CTLCOLOR_REFLECT()
    ON_WM_SETCURSOR()
    ON_WM_MOUSEMOVE()
    ON_WM_TIMER()
    ON_CONTROL_REFLECT(STN_CLICKED, OnClicked)
    ON_WM_ERASEBKGND()
    
END_MESSAGE_MAP()

void CHyperLink::OnClicked()
{
    m_bOverControl = FALSE;
    HINSTANCE result = GotoURL(m_strURL, SW_SHOW);
    m_bVisited = (result > (HINSTANCE) HINSTANCE_ERROR);
    if (!m_bVisited)
    {
        MessageBeep(MB_ICONEXCLAMATION);     
        ReportError((LONG64) result);
    }
    else 
        SetVisited();                        
}

HBRUSH CHyperLink::CtlColor(CDC* pDC, UINT nCtlColor) 
{
    ASSERT(nCtlColor == CTLCOLOR_STATIC);

    if (m_bOverControl)
        pDC->SetTextColor(m_crHoverColour);
    else if (m_bVisited)
        pDC->SetTextColor(m_crVisitedColour);
    else
        pDC->SetTextColor(m_crLinkColour);

    pDC->SetBkMode(TRANSPARENT);
    return (HBRUSH)GetStockObject(NULL_BRUSH);
}

void CHyperLink::OnMouseMove(UINT nFlags, CPoint point) 
{
    if (!m_bOverControl)        
    {
        m_bOverControl = TRUE;

        if (m_nUnderline == ulHover)
            SetFont(&m_UnderlineFont);
        Invalidate();

        SetTimer(m_nTimerID, 100, NULL);
    }
    CStatic::OnMouseMove(nFlags, point);
}

void CHyperLink::OnTimer(UINT_PTR nIDEvent) 
{
    CPoint p(GetMessagePos());
    ScreenToClient(&p);

    CRect rect;
    GetClientRect(rect);
    if (!rect.PtInRect(p))
    {
        m_bOverControl = FALSE;
        KillTimer(m_nTimerID);

        if (m_nUnderline != ulAlways)
            SetFont(&m_StdFont);
        rect.bottom+=10;
        InvalidateRect(rect);
    }
    
	CStatic::OnTimer(nIDEvent);
}

BOOL CHyperLink::OnSetCursor(CWnd* , UINT , UINT ) 
{
    if (m_hLinkCursor)
    {
        ::SetCursor(m_hLinkCursor);
        return TRUE;
    }
    return FALSE;
}

BOOL CHyperLink::OnEraseBkgnd(CDC* pDC) 
{
    CRect rect;
    GetClientRect(rect);
    pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

    return TRUE;
}

void CHyperLink::SetURL(CString strURL)
{
    m_strURL = strURL;

    if (::IsWindow(GetSafeHwnd())) {
        PositionWindow();
        m_ToolTip.UpdateTipText(strURL, this, TOOLTIP_ID);
    }
}

CString CHyperLink::GetURL() const
{ 
    return m_strURL;   
}

void CHyperLink::SetColours(COLORREF crLinkColour, COLORREF crVisitedColour,
                            COLORREF crHoverColour ) 
{ 
    m_crLinkColour    = crLinkColour; 
    m_crVisitedColour = crVisitedColour;

	if (crHoverColour == -1)
		m_crHoverColour = ::GetSysColor(COLOR_HIGHLIGHT);
	else
		m_crHoverColour = crHoverColour;

    if (::IsWindow(m_hWnd))
        Invalidate(); 
}

COLORREF CHyperLink::GetLinkColour() const
{ 
    return m_crLinkColour; 
}

COLORREF CHyperLink::GetVisitedColour() const
{
    return m_crVisitedColour; 
}

COLORREF CHyperLink::GetHoverColour() const
{
    return m_crHoverColour;
}

void CHyperLink::SetVisited(BOOL bVisited ) 
{ 
    m_bVisited = bVisited; 

    if (::IsWindow(GetSafeHwnd()))
        Invalidate(); 
}

BOOL CHyperLink::GetVisited() const
{ 
    return m_bVisited; 
}

void CHyperLink::SetLinkCursor(HCURSOR hCursor)
{ 
    m_hLinkCursor = hCursor;
    if (m_hLinkCursor == NULL)
        SetDefaultCursor();
}

HCURSOR CHyperLink::GetLinkCursor() const
{
    return m_hLinkCursor;
}

void CHyperLink::SetUnderline(int nUnderline )
{
    if (m_nUnderline == nUnderline)
        return;

    if (::IsWindow(GetSafeHwnd()))
    {
        if (nUnderline == ulAlways)
            SetFont(&m_UnderlineFont);
        else
            SetFont(&m_StdFont);

        Invalidate(); 
    }

    m_nUnderline = nUnderline;
}

int CHyperLink::GetUnderline() const
{ 
    return m_nUnderline; 
}

void CHyperLink::SetAutoSize(BOOL bAutoSize )
{
    m_bAdjustToFit = bAutoSize;

    if (::IsWindow(GetSafeHwnd()))
        PositionWindow();
}

BOOL CHyperLink::GetAutoSize() const
{ 
    return m_bAdjustToFit; 
}

void CHyperLink::PositionWindow()
{
    if (!::IsWindow(GetSafeHwnd()) || !m_bAdjustToFit) 
        return;

    CRect WndRect, ClientRect;
    GetWindowRect(WndRect);
    GetClientRect(ClientRect);

    ClientToScreen(ClientRect);

    CWnd* pParent = GetParent();
    if (pParent)
    {
        pParent->ScreenToClient(WndRect);
        pParent->ScreenToClient(ClientRect);
    }

    CString strWndText;
    GetWindowText(strWndText);

    CDC* pDC = GetDC();
    CFont* pOldFont = pDC->SelectObject(&m_UnderlineFont);
    CSize Extent = pDC->GetTextExtent(strWndText);
    pDC->SelectObject(pOldFont);
    ReleaseDC(pDC);

    Extent.cx += WndRect.Width() - ClientRect.Width(); 
    Extent.cy += WndRect.Height() - ClientRect.Height(); 

    DWORD dwStyle = GetStyle();

    if (dwStyle & SS_CENTERIMAGE)
        WndRect.DeflateRect(0, (WndRect.Height() - Extent.cy)/2);
    else
        WndRect.bottom = WndRect.top + Extent.cy;

    if (dwStyle & SS_CENTER)   
        WndRect.DeflateRect((WndRect.Width() - Extent.cx)/2, 0);
    else if (dwStyle & SS_RIGHT) 
        WndRect.left  = WndRect.right - Extent.cx;
    else 
        WndRect.right = WndRect.left + Extent.cx;

    SetWindowPos(NULL, WndRect.left, WndRect.top, WndRect.Width(), WndRect.Height(), SWP_NOZORDER);
}

void CHyperLink::SetDefaultCursor()
{
    if (m_hLinkCursor == NULL)                
    {
        
        CString strWndDir;
        GetWindowsDirectory(strWndDir.GetBuffer(MAX_PATH), MAX_PATH);
        strWndDir.ReleaseBuffer();

        strWndDir += _T("\\winhlp32.exe");
        
        HMODULE hModule = LoadLibrary(strWndDir);
        if (hModule) {
            HCURSOR hHandCursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
            if (hHandCursor)
                m_hLinkCursor = CopyCursor(hHandCursor);
        }
        FreeLibrary(hModule);
    }
}

LONG CHyperLink::GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata)
{
    HKEY hkey;
    LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);

    if (retval == ERROR_SUCCESS) {
        long datasize = MAX_PATH;
        TCHAR data[MAX_PATH];
        RegQueryValue(hkey, NULL, data, &datasize);
        lstrcpy(retdata,data);
        RegCloseKey(hkey);
    }

    return retval;
}

void CHyperLink::ReportError(LONG64 nError)
{
    CString str;
    switch (nError) {
        case 0:                       str = "The operating system is out\nof memory or resources."; break;
        case SE_ERR_PNF:              str = "The specified path was not found."; break;
        case SE_ERR_FNF:              str = "The specified file was not found."; break;
        case ERROR_BAD_FORMAT:        str = "The .EXE file is invalid\n(non-Win32 .EXE or error in .EXE image)."; break;
        case SE_ERR_ACCESSDENIED:     str = "The operating system denied\naccess to the specified file."; break;
        case SE_ERR_ASSOCINCOMPLETE:  str = "The filename association is\nincomplete or invalid."; break;
        case SE_ERR_DDEBUSY:          str = "The DDE transaction could not\nbe completed because other DDE transactions\nwere being processed."; break;
        case SE_ERR_DDEFAIL:          str = "The DDE transaction failed."; break;
        case SE_ERR_DDETIMEOUT:       str = "The DDE transaction could not\nbe completed because the request timed out."; break;
        case SE_ERR_DLLNOTFOUND:      str = "The specified dynamic-link library was not found."; break;
        case SE_ERR_NOASSOC:          str = "There is no application associated\nwith the given filename extension."; break;
        case SE_ERR_OOM:              str = "There was not enough memory to complete the operation."; break;
        case SE_ERR_SHARE:            str = "A sharing violation occurred. ";
        default:                      str.Format(_T("Unknown Error (%d) occurred."), nError); break;
    }
    str = "Unable to open hyperlink:\n\n" + str;
    AfxMessageBox(str, MB_ICONEXCLAMATION | MB_OK);
}

HINSTANCE CHyperLink::GotoURL(LPCTSTR url, int showcmd)
{
    TCHAR key[MAX_PATH + MAX_PATH];

    HINSTANCE result = ShellExecute(NULL, _T("open"), url, NULL,NULL, showcmd);

    if (result <= (HINSTANCE) HINSTANCE_ERROR) {

        if (GetRegKey(HKEY_CLASSES_ROOT, _T(".htm"), key) == ERROR_SUCCESS) {
            lstrcat(key, _T("\\shell\\open\\command"));

            if (GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS) {
                TCHAR *pos;
                pos = _tcsstr(key, _T("\"%1\""));
                if (pos == NULL) {                     
                    pos = _tcsstr(key, _T("%1"));      
                    if (pos == NULL)                   
                        pos = key+lstrlen(key)-1;
                    else
                        *pos = '\0';                   
                }
                else
                    *pos = '\0';                       

                lstrcat(pos, _T(" "));
                lstrcat(pos, url);

                USES_CONVERSION;
                result = (HINSTANCE) (LONG64) WinExec(T2A(key),showcmd);
            }
        }
    }

    return result;
}

