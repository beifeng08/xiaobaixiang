// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

class CCpuDlg : public CDialog
{

public:
	CCpuDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_CPU };
	UINT	m_hours;
	CString	m_cpu_info;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	DECLARE_MESSAGE_MAP()
};
