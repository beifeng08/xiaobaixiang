// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "Prime95.h"
#include "PrimenetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PrimenetDlg::PrimenetDlg(CWnd* pParent )
	: CDialog(PrimenetDlg::IDD, pParent)
{
	
	m_primenet = FALSE;
	m_userid = _T("");
	m_compid = _T("");
	m_dialup = FALSE;
	m_proxyhost = _T("");
	m_proxyport = 8080;
	m_proxyuser = _T("");
	m_proxypassword = _T("");
	m_debug = 0;
	
}

void PrimenetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Check(pDX, IDC_PRIMENET, m_primenet);
	DDX_Control(pDX, IDC_LINK, m_new_user_link);
	DDX_Text(pDX, IDC_USERID, m_userid);
	DDV_MaxChars(pDX, m_userid, 20);
	DDX_Text(pDX, IDC_COMPID, m_compid);
	DDV_MaxChars(pDX, m_compid, 20);
	DDX_Control(pDX, IDC_CONNECTION, c_connection);
	
	c_connection.EnableWindow (m_primenet);
}

BEGIN_MESSAGE_MAP(PrimenetDlg, CDialog)
	
	ON_BN_CLICKED(IDC_PRIMENET, OnPrimenet)
	ON_BN_CLICKED(IDC_CONNECTION, OnConnection)
	
END_MESSAGE_MAP()

BOOL PrimenetDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_new_user_link.SetURL(_T("http://v5www.mersenne.org/update/"));
	
	return TRUE;  
}

void PrimenetDlg::OnPrimenet() 
{
	UpdateData ();		
}

void PrimenetDlg::OnConnection() 
{
	PrimenetConnectionDlg dlg;

	dlg.m_dialup = m_dialup;
	dlg.m_proxyhost = m_proxyhost;
	dlg.m_proxyport = m_proxyport;
	dlg.m_proxyuser = m_proxyuser;
	dlg.m_proxypassword = m_proxypassword;
	dlg.m_debug = m_debug;
	if (dlg.DoModal () == IDOK) {
		m_dialup = dlg.m_dialup;
		m_proxyhost = dlg.m_proxyhost;
		m_proxyport = dlg.m_proxyport;
		m_proxyuser = dlg.m_proxyuser;
		m_proxypassword = dlg.m_proxypassword;
		m_debug = dlg.m_debug;
	}
}

PrimenetConnectionDlg::PrimenetConnectionDlg(CWnd* pParent )
	: CDialog(PrimenetConnectionDlg::IDD, pParent)
{
	
	m_dialup = FALSE;
	m_proxyhost = _T("");
	m_proxyport = 8080;
	m_proxyuser = _T("");
	m_proxypassword = _T("");
	m_debug = 0;
	
}

void PrimenetConnectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Check(pDX, IDC_DIALUP, m_dialup);
	DDX_Text(pDX, IDC_PROXYHOST, m_proxyhost);
	DDX_Control(pDX, IDC_PROXYPORT_TEXT, c_proxyport_text);
	DDX_Control(pDX, IDC_PROXYPORT, c_proxyport);
	DDX_Text(pDX, IDC_PROXYPORT, m_proxyport);
	DDV_MinMaxUInt(pDX, m_proxyport, 0, 65535);
	DDX_Control(pDX, IDC_PROXYUSER_TEXT, c_proxyuser_text);
	DDX_Control(pDX, IDC_PROXYUSER, c_proxyuser);
	DDX_Text(pDX, IDC_PROXYUSER, m_proxyuser);
	DDX_Control(pDX, IDC_PROXYPASSWORD_TEXT, c_proxypassword_text);
	DDX_Control(pDX, IDC_PROXYPASSWORD, c_proxypassword);
	DDX_Text(pDX, IDC_PROXYPASSWORD, m_proxypassword);
	DDX_Check(pDX, IDC_DEBUG, m_debug);
	
	c_proxyport_text.EnableWindow (m_proxyhost[0]);
	c_proxyport.EnableWindow (m_proxyhost[0]);
	c_proxyuser_text.EnableWindow (m_proxyhost[0]);
	c_proxyuser.EnableWindow (m_proxyhost[0]);
	c_proxypassword_text.EnableWindow (m_proxyhost[0]);
	c_proxypassword.EnableWindow (m_proxyhost[0]);
}

BEGIN_MESSAGE_MAP(PrimenetConnectionDlg, CDialog)
	
	ON_EN_CHANGE(IDC_PROXYHOST, &PrimenetConnectionDlg::OnEnChangeProxyHost)
	
END_MESSAGE_MAP()

void PrimenetConnectionDlg::OnEnChangeProxyHost() 
{
	UpdateData ();		
}

