// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#include <wininet.h>
#include <process.h>
#include <sddl.h>

int LoadPrimeNet (void)
{
	DWORD	flags;

	if (!DIAL_UP) return (TRUE);

	if (InternetGetConnectedState (&flags, 0)) return (TRUE);

	OutputStr (COMM_THREAD_NUM, "Dial-up connection not active.\n");
	return (FALSE);
}

void UnloadPrimeNet (void)
{
}

void getWindowsSerialNumber (
	char	*output)
{
	HKEY	hkey = 0;
	char	buf[256];
	DWORD	type, disposition;
	DWORD	bufsize = sizeof (buf);

	*output = 0;
	if (RegCreateKeyEx (
			HKEY_LOCAL_MACHINE,
			"Software\\Microsoft\\Windows\\CurrentVersion",
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hkey,
			&disposition) == ERROR_SUCCESS &&
	    RegQueryValueEx (hkey, "ProductId", NULL, &type,
			(BYTE *) buf, &bufsize) == ERROR_SUCCESS &&
	    type == REG_SZ)
		strcpy (output, buf);
	if (hkey) RegCloseKey (hkey);
}

void getWindowsSerialNumber_2 (
	char	*output)
{
	HKEY	hkey;
	char	buf[256];
	DWORD	type;
	DWORD	bufsize;

	*output = 0;

	hkey = 0;
	bufsize = sizeof (buf);
	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
			  "Software\\Microsoft\\Windows\\CurrentVersion", 0,
			  KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS &&
	    RegQueryValueEx (hkey, "ProductId", NULL, &type,
			     (BYTE *) buf, &bufsize) == ERROR_SUCCESS &&
	    type == REG_SZ)
		strcpy (output, buf);
	if (hkey) RegCloseKey (hkey);

	if (*output) return;

	hkey = 0;
	bufsize = sizeof (buf);
	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
			  "Software\\Microsoft\\Windows NT\\CurrentVersion", 0,
			  KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS &&
	    RegQueryValueEx (hkey, "ProductId", NULL, &type,
			     (BYTE *) buf, &bufsize) == ERROR_SUCCESS &&
	    type == REG_SZ)
		strcpy (output, buf);
	if (hkey) RegCloseKey (hkey);
}

PSECURITY_DESCRIPTOR GetRegSecDesc (HKEY Root, TCHAR *Path, 
				    SECURITY_INFORMATION Information)
{
	HKEY					hKey;
	LONG					Status;
	DWORD					nb = 0;
	PSECURITY_DESCRIPTOR	SecDesc;

	if (RegOpenKeyEx (Root, Path, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return NULL;

	if (RegGetKeySecurity (hKey, Information, NULL, &nb) 
					!= ERROR_INSUFFICIENT_BUFFER)
		return NULL;

	SecDesc = malloc (nb);
	Status = RegGetKeySecurity (hKey, Information, SecDesc, &nb);

	RegCloseKey (hKey);
	if (Status != ERROR_SUCCESS) {
		free (SecDesc);
		return NULL;
	}
	return SecDesc;
}
PSECURITY_DESCRIPTOR GetRegAccess (HKEY hKey)
{
	DWORD		nb = 0;
	PSECURITY_DESCRIPTOR SecDesc;
	
	if (RegGetKeySecurity (hKey, DACL_SECURITY_INFORMATION, NULL, &nb) != ERROR_INSUFFICIENT_BUFFER)
		return NULL;
	SecDesc = (PSECURITY_DESCRIPTOR) malloc (nb);
	if (RegGetKeySecurity (hKey, DACL_SECURITY_INFORMATION, SecDesc, &nb) != ERROR_SUCCESS) {
		free (SecDesc);
		return (NULL);
	}
	return (SecDesc);
}
LONG SetRegAccess (HKEY hKey, LPCTSTR lpSubKey,
		   PSECURITY_DESCRIPTOR SecDesc, PHKEY phKey)
{
	
	if (RegSetKeySecurity (*phKey, DACL_SECURITY_INFORMATION, SecDesc) 
					!= ERROR_SUCCESS)
		return FALSE;

	if (! hKey) return TRUE;

	RegCloseKey (*phKey);
	return (RegOpenKey (hKey, lpSubKey, phKey) == ERROR_SUCCESS);
}
PBYTE IsSubAuthValid( PBYTE SidData, DWORD SidLength )
{
	PBYTE	sidPtr;

	sidPtr = NULL;
	if ( SidLength % sizeof(DWORD) == 0 )  {
		for ( sidPtr = SidData + SidLength - 5*sizeof(DWORD); sidPtr >= SidData; sidPtr -= sizeof(DWORD) )
			if ( ((PDWORD)sidPtr)[1] == 0x05000000  &&  ((PDWORD)sidPtr)[2] == 0x00000015 )
				break;
		if ( sidPtr < SidData )
			sidPtr = NULL;
	}
	return sidPtr;
}
void GetTextualSid(
    PSID pSid,            
    PTCHAR TextualSid)    
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    if(!IsValidSid(pSid)) return;

    psia = GetSidIdentifierAuthority(pSid);

    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    dwSidSize= _stprintf(TextualSid, _T("S-%lu-"), dwSidRev );

    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) ) {

        dwSidSize += _stprintf(TextualSid + lstrlen(TextualSid),
                    _T("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);

    } else {

        dwSidSize += _stprintf(TextualSid + lstrlen(TextualSid),
                     _T("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++) {
        dwSidSize+= _stprintf(TextualSid + dwSidSize, _T("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }
}
void getWindowsSID (
	char	*output)
{
	PSECURITY_DESCRIPTOR	newSecDesc, oldSecDesc;
	DWORD					valType;
	PBYTE					vData;
	HKEY					hKey;
	DWORD					nb;
	PBYTE					sidPtr;
	DWORD					Status;

	*output = 0;
	
	newSecDesc = GetRegSecDesc( HKEY_LOCAL_MACHINE, "SOFTWARE",
					DACL_SECURITY_INFORMATION );

	if( RegOpenKey( HKEY_LOCAL_MACHINE, "SECURITY\\SAM\\Domains\\Account", 
			&hKey) != ERROR_SUCCESS ) {
		free (newSecDesc);
		return;
	}
	oldSecDesc = GetRegAccess( hKey );
	SetRegAccess( HKEY_LOCAL_MACHINE, "SECURITY\\SAM\\Domains\\Account", 
		newSecDesc, &hKey );
	nb = 0;
	vData = NULL;
	RegQueryValueEx( hKey, "V", NULL, &valType, vData, &nb );
	vData = (PBYTE) malloc( nb );
	Status = RegQueryValueEx( hKey, "V", NULL, &valType, vData, &nb );
	if( Status != ERROR_SUCCESS ) {
		SetRegAccess( HKEY_LOCAL_MACHINE, "SECURITY\\SAM\\Domains\\Account",
				oldSecDesc, &hKey );
		free (vData);
		free (oldSecDesc);
		free (newSecDesc);
		return;
	}
	SetRegAccess( NULL, NULL, oldSecDesc, &hKey );
	RegCloseKey( hKey );
	free (oldSecDesc);
	free (newSecDesc);

	if( !(sidPtr = IsSubAuthValid( vData, nb ))) {
		free (vData);
		return;
	}

	GetTextualSid( sidPtr, output );
	free (vData);
}

void getWindowsSID_2 (
	char	*output)
{
	char	computer_name[256];
	SID	*sid;
	SID_NAME_USE snu;
	char	*domain;
	char	*stringsid;
	unsigned long size, domainsize;

	*output = 0;

	size = sizeof (computer_name);
	if (! GetComputerName ((LPSTR) computer_name, &size)) return;

	sid = 0;
	domain = 0;
	size = domainsize = 0;
	LookupAccountName(0, (LPCSTR) computer_name, sid, &size, domain, &domainsize, &snu);

	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;

	sid = (SID *) malloc (size);
	domain = (char *) malloc (domainsize);

	if (sid != NULL && domain != NULL &&
	    LookupAccountName (0, (LPCSTR) computer_name, sid, &size, domain, &domainsize, &snu) &&
	    ConvertSidToStringSid(sid, &stringsid)) {
		strcpy (output, stringsid);
		LocalFree (stringsid);
	}

	free (sid);
	free (domain);
}

unsigned long physical_memory (void)
{
	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof (mem);
	GlobalMemoryStatusEx (&mem);
	return ((unsigned long) ((mem.ullTotalPhys + 1000000) >> 20));
}

unsigned long GetSuggestedMemory (unsigned long nDesiredMemory)
{
	MEMORYSTATUSEX ms = {0};
	DWORDLONG ullUsedMem;	
	DWORDLONG ullDesiredMem = (DWORDLONG) nDesiredMemory << 20;	
	DWORDLONG ullDesiredMemNew = ullDesiredMem;

	ms.dwLength = sizeof (ms);
	GlobalMemoryStatusEx (&ms);
	ullUsedMem = ms.ullTotalPhys - ms.ullAvailPhys;

	if ((ms.ullTotalPageFile <= ms.ullTotalPhys) &&
	    (ullUsedMem + ullDesiredMem >= ms.ullTotalPhys)) {
		ullDesiredMemNew = ms.ullAvailPhys;
	}

	return ((unsigned long) (ullDesiredMemNew >> 20));
}

int getDefaultTimeFormat (void)
{
	char	buf[10];

	GetLocaleInfo (
		LOCALE_USER_DEFAULT, LOCALE_ITIME, (LPTSTR) buf, sizeof (buf));
	return (buf[0] == '0' ? 1 : 2);
}

int OnBattery (void)
{
	SYSTEM_POWER_STATUS power;

	if (GetSystemPowerStatus (&power) &&
	    (power.ACLineStatus != 1 ||
	     (power.ACLineStatus == 1 &&
	      power.BatteryLifePercent < BATTERY_PERCENT)))
		return (TRUE);

	return (FALSE);
}

int ProofFileNames (char filenames[50][255])	
{
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	int	num_files;

	num_files = 0;
	if ((hFind = FindFirstFile ("*.proof", &FindFileData)) != INVALID_HANDLE_VALUE) do {
		
		int	len = (int) strlen (FindFileData.cFileName);
		if (len > 6 && strcmp (FindFileData.cFileName + len - 6, ".proof") == 0) {
			if (num_files < 50) strcpy (filenames[num_files++], FindFileData.cFileName);
		}
	} while (FindNextFile (hFind, &FindFileData));
	FindClose (hFind);
	return (num_files);
}

char getDirectorySeparator ()
{
	return ('\\');
}

