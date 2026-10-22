// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

class CWelcomeDlg : public CDialog
{

public:
	CWelcomeDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_WELCOME };
		
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	virtual void OnCancel();
	
	DECLARE_MESSAGE_MAP()
};
