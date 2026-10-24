// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

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
