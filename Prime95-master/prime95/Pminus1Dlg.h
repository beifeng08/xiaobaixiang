// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

class CPminus1Dlg : public CDialog
{

public:
	CPminus1Dlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_PMINUS1 };
	CStatic	c_worker_text;
	CEdit	c_worker;
	UINT	m_worker;
	double	m_k;
	UINT	m_b;
	UINT	m_n;
	long	m_c;
	uint64_t m_bound1;
	uint64_t m_bound2;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	DECLARE_MESSAGE_MAP()
};
