// ResourcesDlg.cpp : implementation file
//
// Copyright 1995-2023 Mersenne Research, Inc.  All rights reserved
//

#include "stdafx.h"
#include "XiaobaiPrime.h"
#include "ResourcesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define round_to_tenth(a)	((round((a) * 10.0)) / 10.0)

CResourcesDlg::CResourcesDlg(CWnd* pParent )
	: CDialog(CResourcesDlg::IDD, pParent)
{
	
	m_disk = 0.0;
	m_day_memory = 0.0;
	m_night_memory = 0.0;
	m_start_time = _T("");
	m_end_time = _T("");
	m_upload_bandwidth = 0.0;
	m_upload_start = _T("");
	m_upload_end = _T("");
	m_download_mb = 0;
	m_can_upload = 0;
	
}

void CResourcesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Text(pDX, IDC_DISK, m_disk);
	DDV_MinMaxFloat(pDX, m_disk, (float) 0.0, (float) 1000.0);
	DDX_Text(pDX, IDC_START_TIME, m_start_time);
	DDX_Text(pDX, IDC_END_TIME, m_end_time);
	DDX_Text(pDX, IDC_DAY_MEMORY, m_day_memory);
	DDV_MinMaxFloat(pDX, m_day_memory, 0.0, (float) round_to_tenth (0.9 * physical_memory () / 1024.0));
	DDX_Text(pDX, IDC_NIGHT_MEMORY, m_night_memory);
	DDV_MinMaxFloat(pDX, m_night_memory, 0.0, (float) round_to_tenth (0.9 * physical_memory () / 1024.0));
	DDX_Text(pDX, IDC_UPLOAD_BANDWIDTH, m_upload_bandwidth);
	DDV_MinMaxFloat(pDX, m_upload_bandwidth, (float) 0.05, (float) 10000.0);
	DDX_Text(pDX, IDC_UPLOAD_START, m_upload_start);
	DDV_MaxChars(pDX, m_upload_start, 8);
	DDX_Text(pDX, IDC_UPLOAD_END, m_upload_end);
	DDV_MaxChars(pDX, m_upload_end, 8);
	DDX_Text(pDX, IDC_DOWNLOAD_MB, m_download_mb);
	DDV_MinMaxUInt(pDX, m_download_mb, 0, 999999);
	DDX_Control(pDX, IDC_UPLOAD_BANDWIDTH_TEXT, c_upload_bandwidth_text);
	DDX_Control(pDX, IDC_UPLOAD_BANDWIDTH, c_upload_bandwidth);
	DDX_Control(pDX, IDC_UPLOAD_START_TEXT, c_upload_start_text);
	DDX_Control(pDX, IDC_UPLOAD_START, c_upload_start);
	DDX_Control(pDX, IDC_UPLOAD_END_TEXT, c_upload_end_text);
	DDX_Control(pDX, IDC_UPLOAD_END, c_upload_end);
	
	c_upload_bandwidth_text.EnableWindow (m_can_upload);
	c_upload_bandwidth.EnableWindow (m_can_upload);
	c_upload_start_text.EnableWindow (m_can_upload);
	c_upload_start.EnableWindow (m_can_upload);
	c_upload_end_text.EnableWindow (m_can_upload);
	c_upload_end.EnableWindow (m_can_upload);
}

BEGIN_MESSAGE_MAP(CResourcesDlg, CDialog)
	
	ON_BN_CLICKED(IDC_ADVANCED, OnAdvanced)
	
END_MESSAGE_MAP()

void CResourcesDlg::OnAdvanced()
{
	CResourcesAdvancedDlg dlg;
	char	buf[512];

	UpdateData ();		
	IniGetString (INI_FILE, "ProofResiduesDir", buf, sizeof (buf), NULL);
	dlg.m_temp_dir = buf;
	IniGetString (INI_FILE, "ProofArchiveDir", buf, sizeof (buf), NULL);
	dlg.m_archive_dir = buf;
	dlg.m_emergency_mem = (float) round_to_tenth (IniGetInt (INI_FILE, "MaxEmergencyMemory", 1024) / 1024.0);
	dlg.m_priority = PRIORITY;
	dlg.m_cert_cpu = IniGetInt (INI_FILE, "CertDailyCPULimit", 10);
	dlg.m_hyper_tf = HYPERTHREAD_TF;
	dlg.m_hyper_ll = HYPERTHREAD_LL;
	dlg.m_can_upload = m_can_upload;
	dlg.m_can_download = !!(IniGetInt (INI_FILE, "CertWork", 1));
	if (dlg.DoModal () == IDOK) {
		int	restart = FALSE;

		IniWriteString (INI_FILE, "ProofResiduesDir", (const char *) dlg.m_temp_dir);
		IniWriteString (INI_FILE, "ProofArchiveDir", (const char *) dlg.m_archive_dir);

		IniWriteInt (INI_FILE, "MaxEmergencyMemory", (long) (dlg.m_emergency_mem * 1024.0));

		if (PRIORITY != dlg.m_priority) {
			PRIORITY = dlg.m_priority;
			IniWriteInt (INI_FILE, "Priority", PRIORITY);
			restart = TRUE;
		}

		IniWriteInt (INI_FILE, "CertDailyCPULimit", dlg.m_cert_cpu);

		if (dlg.m_hyper_tf != HYPERTHREAD_TF) {
			HYPERTHREAD_TF = dlg.m_hyper_tf;
			IniWriteInt (INI_FILE, "HyperthreadTF", HYPERTHREAD_TF);
			restart = TRUE;
		}
		if (dlg.m_hyper_ll != HYPERTHREAD_LL) {
			HYPERTHREAD_LL = dlg.m_hyper_ll;
			IniWriteInt (INI_FILE, "HyperthreadLL", HYPERTHREAD_LL);
			restart = TRUE;
		}

		if (restart) stop_workers_for_restart ();
	}
}

CResourcesAdvancedDlg::CResourcesAdvancedDlg(CWnd* pParent )
	: CDialog(CResourcesAdvancedDlg::IDD, pParent)
{
	
	m_temp_dir = _T("");
	m_archive_dir = _T("");
	m_emergency_mem = 0.0;
	m_priority = 0;
	m_cert_cpu = 0;
	m_hyper_tf = FALSE;
	m_hyper_ll = FALSE;
	m_can_upload = 0;
	m_can_download = 0;
	
}

void CResourcesAdvancedDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	float	max_emergency_mem = (float) round_to_tenth (0.25 * physical_memory () / 1024.0);
	if (max_emergency_mem < 1.0) max_emergency_mem = 1.0;

	DDX_Text(pDX, IDC_TEMP_DIR, m_temp_dir);
	DDX_Control(pDX, IDC_ARCHIVE_DIR_TEXT, c_archive_dir_text);
	DDX_Control(pDX, IDC_ARCHIVE_DIR, c_archive_dir);
	DDX_Text(pDX, IDC_ARCHIVE_DIR, m_archive_dir);
	DDX_Text(pDX, IDC_EMERGENCY_MEM, m_emergency_mem);
	DDV_MinMaxFloat(pDX, m_emergency_mem, 0.0, max_emergency_mem);
	DDX_Text(pDX, IDC_PRIORITY, m_priority);
	DDV_MinMaxUInt(pDX, m_priority, 1, 10);
	DDX_Control(pDX, IDC_CERT_CPU_TEXT, c_cert_cpu_text);
	DDX_Control(pDX, IDC_CERT_CPU, c_cert_cpu);
	DDX_Text(pDX, IDC_CERT_CPU, m_cert_cpu);
	if (IniGetInt (INI_FILE, "CertWork", 1)) DDV_MinMaxUInt(pDX, m_cert_cpu, 1, 100);
	else DDV_MinMaxUInt(pDX, m_cert_cpu, 0, 100);
	DDX_Check(pDX, IDC_HYPER_TF, m_hyper_tf);
	DDX_Check(pDX, IDC_HYPER_LL, m_hyper_ll);
	DDX_Control(pDX, IDC_HYPER_TF, c_hyper_tf);
	DDX_Control(pDX, IDC_HYPER_LL, c_hyper_ll);
	
	c_archive_dir_text.EnableWindow (m_can_upload);
	c_archive_dir.EnableWindow (m_can_upload);
	c_cert_cpu_text.EnableWindow (m_can_download);
	c_cert_cpu.EnableWindow (m_can_download);
	c_hyper_tf.EnableWindow (HW_NUM_CORES != HW_NUM_THREADS);
	c_hyper_ll.EnableWindow (HW_NUM_CORES != HW_NUM_THREADS);
}

BEGIN_MESSAGE_MAP(CResourcesAdvancedDlg, CDialog)
	
END_MESSAGE_MAP()

