// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "XiaobaiPrime.h"
#include "UnreserveDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CUnreserveDlg::CUnreserveDlg(CWnd* pParent )
	: CDialog(CUnreserveDlg::IDD, pParent)
{
	
	m_p = 0;
	
}

void CUnreserveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Text(pDX, IDC_P, m_p);
	
}

BEGIN_MESSAGE_MAP(CUnreserveDlg, CDialog)
	
END_MESSAGE_MAP()

