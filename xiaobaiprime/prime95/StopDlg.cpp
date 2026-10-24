// StopDlg.cpp : implementation file
//
//  Copyright 1995-2023 Mersenne Research, Inc. All rights reserved.
//

#include "stdafx.h"
#include "XiaobaiPrime.h"
#include "StopDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CStopDlg::CStopDlg(CWnd* pParent )
	: CDialog(CStopDlg::IDD, pParent)
{
	
	m_worker = 1;
	m_all_workers = 1;
	
}

void CStopDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Check(pDX, IDC_RADIO1, m_all_workers);
	DDX_Control(pDX, IDC_WORKER_TEXT, c_worker_text);
	DDX_Control(pDX, IDC_WORKER, c_worker);
	DDX_Text(pDX, IDC_WORKER, m_worker);
	DDV_MinMaxUInt(pDX, m_worker, 1, WORKERS_ACTIVE);
	
	c_worker_text.EnableWindow (!m_all_workers);
	c_worker.EnableWindow (!m_all_workers);
}

BEGIN_MESSAGE_MAP(CStopDlg, CDialog)
	
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedRadio1)
	
END_MESSAGE_MAP()

void CStopDlg::OnBnClickedRadio1()
{
	UpdateData ();
	c_worker_text.EnableWindow (!m_all_workers);
	c_worker.EnableWindow (!m_all_workers);
}

