// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

class CBenchmarkDlg : public CDialog
{

public:
	CBenchmarkDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_BENCHMARK };
	CComboBox c_bench_type;
	int	m_bench_type;

	CStatic	c_minFFT_text;
	CEdit	c_minFFT;
	UINT	m_minFFT;
	CStatic	c_maxFFT_text;
	CEdit	c_maxFFT;
	UINT	m_maxFFT;
	CButton	c_errchk;
	BOOL	m_errchk;
	CButton	c_negacyclic;
	BOOL	m_negacyclic;
	CButton	c_limit_FFT_sizes;
	BOOL	m_limit_FFT_sizes;

	CStatic	c_bench_cores_text;
	CEdit	c_bench_cores;
	CString	m_bench_cores;
	CButton	c_hyperthreading;
	BOOL	m_hyperthreading;

	CStatic	c_bench_workers_text;
	CEdit	c_bench_workers;
	CString	m_bench_workers;
	CButton	c_all_FFT_impl;
	BOOL	m_all_FFT_impl;
	CStatic	c_bench_time_text;
	CEdit	c_bench_time;
	UINT	m_bench_time;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeBenchType();
	afx_msg void OnCbnKillfocusBenchType();
	afx_msg void OnEnKillfocusMinFFT();
	afx_msg void OnEnKillfocusMaxFFT();
	afx_msg void OnBnClickedAllFFTImpl();
	
	DECLARE_MESSAGE_MAP()
};
