// TestDlg.h : header file
//
//  Copyright 1995-2021 Mersenne Research, Inc. All rights reserved.
//

/////////////////////////////////////////////////////////////////////////////
// CTestDlg dialog

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
