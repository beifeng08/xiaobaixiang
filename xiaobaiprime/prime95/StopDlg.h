// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

class CStopDlg : public CDialog
{

public:
	CStopDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_WORKER_STOP };
	CStatic	c_worker_text;
	CEdit	c_worker;
	UINT	m_worker;
	BOOL	m_all_workers;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedRadio1();
};
