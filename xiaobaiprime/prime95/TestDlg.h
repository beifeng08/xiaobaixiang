// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

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
