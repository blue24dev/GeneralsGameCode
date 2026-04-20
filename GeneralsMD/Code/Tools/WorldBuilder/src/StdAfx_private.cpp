//MODDD - new file, to include dependencies needed in order to start some things from copies of the MFC library source code
// (todo: header stuff)

#include "StdAfx_private.h"

// MFC dependency: 'FILECORE.CPP'

// ---------------

AFX_STATIC inline BOOL IsDirSep(TCHAR ch)
{
	return (ch == '\\' || ch == '/');
}

#ifndef _AFX_NO_OLE_SUPPORT

HRESULT AFX_COM::CreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
	REFIID riid, LPVOID* ppv)
{
	// find the object's class factory
	LPCLASSFACTORY pf = NULL;
	HRESULT hRes = GetClassObject(rclsid, IID_IClassFactory, (LPVOID*)&pf);
	if (FAILED(hRes))
		return hRes;

	// call it to create the instance
	ASSERT(pf != NULL);
	hRes = pf->CreateInstance(pUnkOuter, riid, ppv);

	// let go of the factory
	pf->Release();
	return hRes;
}

HRESULT AFX_COM::GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	*ppv = NULL;
	HINSTANCE hInst = NULL;

	// find server name for this class ID

	CString strCLSID = AfxStringFromCLSID(rclsid);
	CString strServer;
	if (!AfxGetInProcServer(strCLSID, strServer))
		return REGDB_E_CLASSNOTREG;

	// try to load it
	hInst = LoadLibrary(strServer);
	if (hInst == NULL)
		return REGDB_E_CLASSNOTREG;

#pragma warning(disable:4191)
	// get its entry point
	HRESULT (STDAPICALLTYPE* pfn)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
	pfn = (HRESULT (STDAPICALLTYPE*)(REFCLSID rclsid, REFIID riid, LPVOID* ppv))
		GetProcAddress(hInst, "DllGetClassObject");
#pragma warning(default:4191)

	// call it, if it worked
	if (pfn != NULL)
		return pfn(rclsid, riid, ppv);
	return CO_E_ERRORINDLL;
}

CString AFXAPI AfxStringFromCLSID(REFCLSID rclsid)
{
	TCHAR szCLSID[256];
	wsprintf(szCLSID, _T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
		rclsid.Data1, rclsid.Data2, rclsid.Data3,
		rclsid.Data4[0], rclsid.Data4[1], rclsid.Data4[2], rclsid.Data4[3],
		rclsid.Data4[4], rclsid.Data4[5], rclsid.Data4[6], rclsid.Data4[7]);
	return szCLSID;
}

BOOL AFXAPI AfxGetInProcServer(LPCTSTR lpszCLSID, CString& str)
{
	HKEY hKey = NULL;
	BOOL b = FALSE;
	if (RegOpenKey(HKEY_CLASSES_ROOT, _T("CLSID"), &hKey) == ERROR_SUCCESS)
	{
		HKEY hKeyCLSID = NULL;
		if (RegOpenKey(hKey, lpszCLSID, &hKeyCLSID) == ERROR_SUCCESS)
		{
			HKEY hKeyInProc = NULL;
			if (RegOpenKey(hKeyCLSID, _T("InProcServer32"), &hKeyInProc) ==
				ERROR_SUCCESS)
			{
				LPTSTR lpsz = str.GetBuffer(_MAX_PATH);
				DWORD dwSize = _MAX_PATH * sizeof(TCHAR);
				DWORD dwType;
				LONG lRes = ::RegQueryValueEx(hKeyInProc, _T(""),
					NULL, &dwType, (BYTE*)lpsz, &dwSize);
				str.ReleaseBuffer();
				b = (lRes == ERROR_SUCCESS);
				RegCloseKey(hKeyInProc);
			}
			RegCloseKey(hKeyCLSID);
		}
		RegCloseKey(hKey);
	}
	return b;
}

#endif

BOOL AFXAPI AfxResolveShortcut(CWnd* pWnd, LPCTSTR lpszFileIn,
	LPTSTR lpszFileOut, int cchPath)
{
	USES_CONVERSION;
	AFX_COM com;
	IShellLink* psl;
	*lpszFileOut = 0;   // assume failure

	SHFILEINFO info;
	if ((SHGetFileInfo(lpszFileIn, 0, &info, sizeof(info),
		SHGFI_ATTRIBUTES) == 0) || !(info.dwAttributes & SFGAO_LINK))
	{
		return FALSE;
	}

	if (FAILED(com.CreateInstance(CLSID_ShellLink, NULL, IID_IShellLink,
		(LPVOID*)&psl)))
	{
		return FALSE;
	}

	IPersistFile *ppf;
	if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf)))
	{
		if (SUCCEEDED(ppf->Load(T2COLE(lpszFileIn), STGM_READ)))
		{
			/* Resolve the link, this may post UI to find the link */
			if (SUCCEEDED(psl->Resolve(pWnd->GetSafeHwnd(),
				SLR_ANY_MATCH)))
			{
				psl->GetPath(lpszFileOut, cchPath, NULL, 0);
				ppf->Release();
				psl->Release();
				return TRUE;
			}
		}
		ppf->Release();
	}
	psl->Release();
	return FALSE;
}

BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn)
	// lpszPathOut = buffer of _MAX_PATH
	// lpszFileIn = file, relative path or absolute path
	// (both in ANSI character set)
{
	ASSERT(AfxIsValidAddress(lpszPathOut, _MAX_PATH));

	// first, fully qualify the path name
	LPTSTR lpszFilePart;
	if (!GetFullPathName(lpszFileIn, _MAX_PATH, lpszPathOut, &lpszFilePart))
	{
#ifdef _DEBUG
		if (lpszFileIn[0] != '\0')
			TRACE1("Warning: could not parse the path '%s'.\n", lpszFileIn);
#endif
		lstrcpyn(lpszPathOut, lpszFileIn, _MAX_PATH); // take it literally
		return FALSE;
	}

	CString strRoot;
	// determine the root name of the volume
	AfxGetRoot(lpszPathOut, strRoot);

	// get file system information for the volume
	DWORD dwFlags, dwDummy;
	if (!GetVolumeInformation(strRoot, NULL, 0, NULL, &dwDummy, &dwFlags,
		NULL, 0))
	{
		TRACE1("Warning: could not get volume information '%s'.\n",
			(LPCTSTR)strRoot);
		return FALSE;   // preserving case may not be correct
	}

	// not all characters have complete uppercase/lowercase
	if (!(dwFlags & FS_CASE_IS_PRESERVED))
		CharUpper(lpszPathOut);

	// assume non-UNICODE file systems, use OEM character set
	if (!(dwFlags & FS_UNICODE_STORED_ON_DISK))
	{
		WIN32_FIND_DATA data;
		HANDLE h = FindFirstFile(lpszFileIn, &data);
		if (h != INVALID_HANDLE_VALUE)
		{
			FindClose(h);
			lstrcpy(lpszFilePart, data.cFileName);
		}
	}
	return TRUE;
}

void AFXAPI AfxGetRoot(LPCTSTR lpszPath, CString& strRoot)
{
	ASSERT(lpszPath != NULL);
	// determine the root name of the volume
	LPTSTR lpszRoot = strRoot.GetBuffer(_MAX_PATH);
	memset(lpszRoot, 0, _MAX_PATH);
	lstrcpyn(lpszRoot, lpszPath, _MAX_PATH);
	//MODDD - moving declaration of 'lpsz' outside the loop header. Most likely, old C gave it a scope outside the for-loop (preserving the intent).
	LPTSTR lpsz;
	for (lpsz = lpszRoot; *lpsz != '\0'; lpsz = _tcsinc(lpsz))
	{
		// find first double slash and stop
		if (IsDirSep(lpsz[0]) && IsDirSep(lpsz[1]))
			break;
	}
	if (*lpsz != '\0')
	{
		// it is a UNC name, find second slash past '\\'
		ASSERT(IsDirSep(lpsz[0]));
		ASSERT(IsDirSep(lpsz[1]));
		lpsz += 2;
		while (*lpsz != '\0' && (!IsDirSep(*lpsz)))
			lpsz = _tcsinc(lpsz);
		if (*lpsz != '\0')
			lpsz = _tcsinc(lpsz);
		while (*lpsz != '\0' && (!IsDirSep(*lpsz)))
			lpsz = _tcsinc(lpsz);
		// terminate it just after the UNC root (ie. '\\server\share\')
		if (*lpsz != '\0')
			lpsz[1] = '\0';
	}
	else
	{
		// not a UNC, look for just the first slash
		lpsz = lpszRoot;
		while (*lpsz != '\0' && (!IsDirSep(*lpsz)))
			lpsz = _tcsinc(lpsz);
		// terminate it just after root (ie. 'x:\')
		if (*lpsz != '\0')
			lpsz[1] = '\0';
	}
	strRoot.ReleaseBuffer();
}
