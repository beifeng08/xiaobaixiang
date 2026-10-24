// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

class CResourcesDlg : public CDialog
{

public:
	CResourcesDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_RESOURCES };
	float	m_disk;
	float	m_day_memory;
	float	m_night_memory;
	CString	m_start_time;
	CString	m_end_time;
	float	m_upload_bandwidth;
	CString m_upload_start;
	CString m_upload_end;
	UINT	m_download_mb;
	BOOL	m_can_upload;
	CStatic	c_upload_bandwidth_text;
	CEdit	c_upload_bandwidth;
	CStatic	c_upload_start_text;
	CEdit	c_upload_start;
	CStatic	c_upload_end_text;
	CEdit	c_upload_end;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	afx_msg void OnAdvanced();
	
	DECLARE_MESSAGE_MAP()
};

class CResourcesAdvancedDlg : public CDialog
{

public:
	CResourcesAdvancedDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_ADVANCED_RESOURCES };
	CString	m_temp_dir;
	CStatic	c_archive_dir_text;
	CEdit	c_archive_dir;
	CString	m_archive_dir;
	float	m_emergency_mem;
	UINT	m_priority;
	CStatic	c_cert_cpu_text;
	CEdit	c_cert_cpu;
	UINT	m_cert_cpu;
	CButton c_hyper_tf;
	BOOL	m_hyper_tf;
	CButton c_hyper_ll;
	BOOL	m_hyper_ll;
	BOOL	m_can_upload;
	BOOL	m_can_download;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	DECLARE_MESSAGE_MAP()
};
