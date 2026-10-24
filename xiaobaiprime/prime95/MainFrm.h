// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

const int MYWM_TRAYMESSAGE = WM_APP + 100;

#define ID_WINDOW_POSITION              0xE136

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNCREATE(CMainFrame)
public:
	CMainFrame();

public:

public:

	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
	virtual BOOL DestroyWindow();
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	
	afx_msg LRESULT OnPower( WPARAM, LPARAM );
	afx_msg LRESULT OnTrayMessage( WPARAM, LPARAM );
	LRESULT OnTaskBarCreated (WPARAM, LPARAM);

public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	CStatusBar  m_wndStatusBar;
protected:  

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnTrayOpenWindow();
	afx_msg void OnStopContinue();
	afx_msg void OnTile();
	afx_msg void OnPosition();
	
	DECLARE_MESSAGE_MAP()
public:
};

