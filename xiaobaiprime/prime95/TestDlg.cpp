// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#include "stdafx.h"
#include "XiaobaiPrime.h"
#include "TestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTestDlg::CTestDlg(CWnd* pParent )
	: CDialog(CTestDlg::IDD, pParent)
{
	
	m_worker = 1;
	m_p = 0;
	
}

char NOTPRIMEERR[] = "This number is not prime, there is no need to test it.";
void DDV_prime (
	CDataExchange* pDX,
	long	p)
{
	if (! isPrime (p)) {
		AfxMessageBox (NOTPRIMEERR, MB_ICONEXCLAMATION);
		pDX->Fail ();
	}
}

void CTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_WORKER_TEXT, c_worker_text);
	DDX_Control(pDX, IDC_WORKER, c_worker);
	DDX_Text(pDX, IDC_WORKER, m_worker);
	DDV_MinMaxUInt(pDX, m_worker, 1, NUM_WORKERS);
	DDX_Text(pDX, IDC_P, m_p);
	
	c_worker_text.EnableWindow (NUM_WORKERS > 1);
	c_worker.EnableWindow (NUM_WORKERS > 1);
	DDV_MinMaxUInt(pDX, m_p, MIN_PRIME,
		       CPU_FLAGS & CPU_FMA3 ? MAX_PRIME_FMA3 :
		       CPU_FLAGS & CPU_SSE2 ? MAX_PRIME_SSE2 : MAX_PRIME);
	DDV_prime(pDX, m_p);
}

BEGIN_MESSAGE_MAP(CTestDlg, CDialog)
	
END_MESSAGE_MAP()

