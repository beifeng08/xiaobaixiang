// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

class CUnreserveDlg : public CDialog
{

public:
	CUnreserveDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_UNRESERVE };
	UINT	m_p;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	DECLARE_MESSAGE_MAP()
};
