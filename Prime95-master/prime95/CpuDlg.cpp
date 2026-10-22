// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "Prime95.h"
#include "CpuDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCpuDlg::CCpuDlg(CWnd* pParent )
	: CDialog(CCpuDlg::IDD, pParent)
{
	
	m_hours = 0;
	m_cpu_info = _T("");
	
}

void CCpuDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Text(pDX, IDC_HOURS, m_hours);
	DDV_MinMaxUInt(pDX, m_hours, 1, 24);
	DDX_Text(pDX, IDC_CPU_INFO, m_cpu_info);
	
}

BEGIN_MESSAGE_MAP(CCpuDlg, CDialog)
	
END_MESSAGE_MAP()

