// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "Prime95.h"
#include "TimeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimeDlg::CTimeDlg(CWnd* pParent )
	: CDialog(CTimeDlg::IDD, pParent)
{
	
	m_p = 0;
	m_iter = 0;
	
}

void CTimeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Text(pDX, IDC_P, m_p);
	DDV_MinMaxUInt(pDX, m_p, MIN_PRIME,
		       CPU_FLAGS & CPU_FMA3 ? MAX_PRIME_FMA3 :
		       CPU_FLAGS & CPU_SSE2 ? MAX_PRIME_SSE2 : MAX_PRIME);
	DDX_Text(pDX, IDC_EDIT1, m_iter);
	DDV_MinMaxUInt(pDX, m_iter, 1, 1000);
	
}

BEGIN_MESSAGE_MAP(CTimeDlg, CDialog)
	
END_MESSAGE_MAP()

