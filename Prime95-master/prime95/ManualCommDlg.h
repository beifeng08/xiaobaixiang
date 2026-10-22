// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

class CManualCommDlg : public CDialog
{

public:
	CManualCommDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_MANUAL_COMM };
	CButton	c_comm_now;
	BOOL	m_manual_comm;
	BOOL	m_comm_now;
	BOOL	m_new_dates;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	afx_msg void OnManual();
	
	DECLARE_MESSAGE_MAP()
};
