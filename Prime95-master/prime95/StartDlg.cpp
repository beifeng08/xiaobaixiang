// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "Prime95.h"
#include "StartDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CStartDlg::CStartDlg(CWnd* pParent )
	: CDialog(CStartDlg::IDD, pParent)
{
	
	m_worker = 1;
	m_all_workers = 1;
	
}

void CStartDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Check(pDX, IDC_RADIO1, m_all_workers);
	DDX_Control(pDX, IDC_WORKER_TEXT, c_worker_text);
	DDX_Control(pDX, IDC_WORKER, c_worker);
	DDX_Text(pDX, IDC_WORKER, m_worker);
	DDV_MinMaxUInt(pDX, m_worker, 1, WORKERS_ACTIVE > NUM_WORKERS ? WORKERS_ACTIVE : NUM_WORKERS);
	
	c_worker_text.EnableWindow (!m_all_workers);
	c_worker.EnableWindow (!m_all_workers);
}

BEGIN_MESSAGE_MAP(CStartDlg, CDialog)
	
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedRadio1)
	
END_MESSAGE_MAP()

void CStartDlg::OnBnClickedRadio1()
{
	UpdateData ();
	c_worker_text.EnableWindow (!m_all_workers);
	c_worker.EnableWindow (!m_all_workers);
}

