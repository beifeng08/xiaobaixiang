// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

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
