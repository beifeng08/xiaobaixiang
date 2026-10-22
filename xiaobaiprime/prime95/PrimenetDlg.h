// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "hyperlink.h"

class PrimenetDlg : public CDialog
{

public:
	PrimenetDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_PRIMENET };
	BOOL	m_primenet;
	CHyperLink m_new_user_link;
	CString	m_userid;
	CString	m_compid;
	CButton	c_connection;
	BOOL	m_dialup;
	CString	m_proxyhost;
	UINT	m_proxyport;
	CString	m_proxyuser;
	CString	m_proxypassword;
	BOOL	m_debug;
	
	protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	afx_msg void OnPrimenet();
	afx_msg void OnConnection();
	
	DECLARE_MESSAGE_MAP()
};

class PrimenetConnectionDlg : public CDialog
{

public:
	PrimenetConnectionDlg(CWnd* pParent = NULL);   

	enum { IDD = IDD_PRIMENET_CONNECTION };
	BOOL	m_dialup;
	CString	m_proxyhost;
	CStatic	c_proxyport_text;
	CEdit	c_proxyport;
	UINT	m_proxyport;
	CStatic	c_proxyuser_text;
	CEdit	c_proxyuser;
	CString	m_proxyuser;
	CStatic	c_proxypassword_text;
	CEdit	c_proxypassword;
	CString	m_proxypassword;
	BOOL	m_debug;
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	
protected:

	afx_msg void OnEnChangeProxyHost();
	
	DECLARE_MESSAGE_MAP()
};
