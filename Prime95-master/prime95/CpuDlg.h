// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

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
