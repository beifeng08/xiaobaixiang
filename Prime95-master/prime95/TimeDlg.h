// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

class CTimeDlg : public CDialog
{

public:
	CTimeDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_TIME };
	UINT	m_p;
	UINT	m_iter;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	DECLARE_MESSAGE_MAP()
};
