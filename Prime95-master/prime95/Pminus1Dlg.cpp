// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "Prime95.h"
#include "Pminus1Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPminus1Dlg::CPminus1Dlg(CWnd* pParent )
	: CDialog(CPminus1Dlg::IDD, pParent)
{
	
	m_worker = 1;
	m_k = 1.0;
	m_b = 2;
	m_n = 1277;
	m_c = -1;
	m_bound1 = 1000000;
	m_bound2 = 0;
	
}

void CPminus1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_WORKER_TEXT, c_worker_text);
	DDX_Control(pDX, IDC_WORKER, c_worker);
	DDX_Text(pDX, IDC_WORKER, m_worker);
	DDV_MinMaxUInt(pDX, m_worker, 1, NUM_WORKERS);
	DDX_Text(pDX, IDC_P4, m_k);
	DDX_Text(pDX, IDC_P1, m_b);
	DDV_MinMaxUInt (pDX, m_b, 2, 1000000000);
	DDX_Text(pDX, IDC_P5, m_n);
	DDV_MinMaxUInt (pDX, m_n, 1, CPU_FLAGS & CPU_SSE2 ? MAX_PRIME_SSE2 : MAX_PRIME);
	DDX_Text(pDX, IDC_P6, m_c);
	DDX_Text(pDX, IDC_P2, m_bound1);
	DDX_Text(pDX, IDC_P3, m_bound2);
	
	c_worker_text.EnableWindow (NUM_WORKERS > 1);
	c_worker.EnableWindow (NUM_WORKERS > 1);
}

BEGIN_MESSAGE_MAP(CPminus1Dlg, CDialog)
	
END_MESSAGE_MAP()

