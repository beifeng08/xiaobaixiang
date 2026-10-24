// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

class CTestDlg : public CDialog
{

public:
	CTestDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_TEST };
	CStatic	c_worker_text;
	CEdit	c_worker;
	UINT	m_worker;
	UINT	m_p;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	DECLARE_MESSAGE_MAP()
};
