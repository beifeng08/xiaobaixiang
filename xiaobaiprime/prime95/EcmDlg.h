// EcmDlg.h : header file
//
//  Copyright 2000-2021 Mersenne Research, Inc. All rights reserved.
//

/////////////////////////////////////////////////////////////////////////////
// CEcmDlg dialog

class CEcmDlg : public CDialog
{

public:
	CEcmDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_ECM };
	CStatic	c_worker_text;
	CEdit	c_worker;
	UINT	m_worker;
	double	m_k;
	UINT	m_b;
	UINT	m_n;
	long	m_c;
	uint64_t m_bound1;
	uint64_t m_bound2;
	UINT	m_num_curves;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	DECLARE_MESSAGE_MAP()
};
