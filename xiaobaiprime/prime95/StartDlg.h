// StartDlg.h : header file
//
//  Copyright 1995-2021 Mersenne Research, Inc. All rights reserved.
//

/////////////////////////////////////////////////////////////////////////////
// CStartDlg dialog

class CStartDlg : public CDialog
{

public:
	CStartDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_WORKER_START };
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
