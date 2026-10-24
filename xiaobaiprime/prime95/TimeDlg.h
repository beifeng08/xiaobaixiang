// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

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
