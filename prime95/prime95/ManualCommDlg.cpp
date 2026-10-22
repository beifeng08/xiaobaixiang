// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "XiaobaiPrime.h"
#include "ManualCommDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CManualCommDlg::CManualCommDlg(CWnd* pParent )
	: CDialog(CManualCommDlg::IDD, pParent)
{
	
	m_manual_comm = FALSE;
	m_comm_now = FALSE;
	m_new_dates = FALSE;
	
}

void CManualCommDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_NOW, c_comm_now);
	DDX_Check(pDX, IDC_MANUAL, m_manual_comm);
	DDX_Check(pDX, IDC_NOW, m_comm_now);
	DDX_Check(pDX, IDC_COMPLETION, m_new_dates);
	
}

BEGIN_MESSAGE_MAP(CManualCommDlg, CDialog)
	
	ON_BN_CLICKED(IDC_MANUAL, OnManual)
	
END_MESSAGE_MAP()

void CManualCommDlg::OnManual() 
{
	UpdateData ();	
}
