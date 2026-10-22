// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

class CPreferencesDlg : public CDialog
{

public:
	CPreferencesDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_PREFERENCES };
	CStatic	c_modem_text;
	CEdit	c_modem;
	CStatic	c_work_text;
	CEdit	c_work;
	CStatic	c_end_dates_text;
	CEdit	c_end_dates;
	CStatic	c_network_text;
	CEdit	c_network;
	UINT	m_iter;
	UINT	m_disk_write_time;
	UINT	m_backup;
	BOOL	m_noise;
	UINT	m_retry;
	UINT	m_r_iter;
	UINT	m_work;
	float	m_end_dates;
	UINT	m_modem;
	BOOL	m_battery;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	DECLARE_MESSAGE_MAP()
};
