// HyperLink.h : header file
//
//
// HyperLink static control. Will open the default browser with the given URL
// when the user clicks on the link.
//
// Copyright Chris Maunder, 1997-1999 (cmaunder@mail.com)
// Feel free to use and distribute. May not be sold for profit. 

// 2/29/00 -- P. Shaffer standard font mod.

#if !defined(AFX_HYPERLINK_H__D1625061_574B_11D1_ABBA_00A0243D1382__INCLUDED_)
#define AFX_HYPERLINK_H__D1625061_574B_11D1_ABBA_00A0243D1382__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 

class CHyperLink : public CStatic
{

public:
    CHyperLink();
    virtual ~CHyperLink();

public:
    enum UnderLineOptions { ulHover = -1, ulNone = 0, ulAlways = 1};

public:
    void SetURL(CString strURL);
    CString GetURL() const;

    void SetColours(COLORREF crLinkColour, COLORREF crVisitedColour, 
                    COLORREF crHoverColour = -1);
    COLORREF GetLinkColour() const;
    COLORREF GetVisitedColour() const;
    COLORREF GetHoverColour() const;

    void SetVisited(BOOL bVisited = TRUE);
    BOOL GetVisited() const;

    void SetLinkCursor(HCURSOR hCursor);
    HCURSOR GetLinkCursor() const;

    void SetUnderline(int nUnderline = ulHover);
    int  GetUnderline() const;

    void SetAutoSize(BOOL bAutoSize = TRUE);
    BOOL GetAutoSize() const;

	public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();
	protected:
    virtual void PreSubclassWindow();
	
public:
    HINSTANCE GotoURL(LPCTSTR url, int showcmd);
protected:
    void ReportError(LONG64 nError);
    LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata);
    void PositionWindow();
    void SetDefaultCursor();

protected:
    COLORREF m_crLinkColour, m_crVisitedColour;     
    COLORREF m_crHoverColour;                       
    BOOL     m_bOverControl;                        
    BOOL     m_bVisited;                            
    int      m_nUnderline;                          
    BOOL     m_bAdjustToFit;                        
    CString  m_strURL;                              
    CFont    m_UnderlineFont;                       
    CFont    m_StdFont;                             
    HCURSOR  m_hLinkCursor;                         
    CToolTipCtrl m_ToolTip;                         
    UINT     m_nTimerID;

protected:
    
    afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    
    afx_msg void OnClicked();
    DECLARE_MESSAGE_MAP()
};

#endif 
