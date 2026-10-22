// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "Prime95.h"
#include "WelcomeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CWelcomeDlg::CWelcomeDlg(CWnd* pParent )
	: CDialog(CWelcomeDlg::IDD, pParent)
{
	
}

void CWelcomeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
}

BEGIN_MESSAGE_MAP(CWelcomeDlg, CDialog)
	
END_MESSAGE_MAP()

void CWelcomeDlg::OnCancel() 
{
	
	CDialog::OnCancel();
}
