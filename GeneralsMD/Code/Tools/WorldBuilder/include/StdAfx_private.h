//MODDD - new file, to include dependencies needed in order to start some things from copies of the MFC library source code
// (todo: header stuff)

#include "StdAfx.h"

// some other things originally included by '<mfc>/src/stdafx.h' (included by most .cpp files in <mfc>/src):
// ---
// '<mfc>/include/afx.h', includes '<standard>/stdlib.h' for '_countof'
// (this did not fix the issue I was having, but the include doesn't hurt regardless)
#include <afx.h>
// '<mfc>/include/afxpriv.h', includes afxconv.h -> atlconv.h
#include <afxpriv.h>
// ---

// I don't know why the 'vc6-debug+t+e' job in github always says '_countof' is undefined.
// '_countof' was found in stdlib.h, '__crt_countof' was found in 'vcruntime.h' (probably too specific) on my machine and
// appears to fix the issue.
//#include <stdlib.h>
//#include <cstdlib>
#ifndef _countof
	#define _countof __crt_countof
	#define __crt_countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

#pragma once

// Think of this file as a stand-in for the MFC library's not-publically-available 'STDAFX.H' file.
// This included the 'AFXIMPL.h' file, which has the prototypes below.
// No sure what '_AFX_NO_OLE_SUPPORT' is for, as there was a reference to 'AFX_COM' regardless of the state of that macro setting.

// MFC dependency: 'AFXIMPL.h'

// ---------------

// misc helpers
BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
void AFXAPI AfxGetRoot(LPCTSTR lpszPath, CString& strRoot);
BOOL AFXAPI AfxResolveShortcut(CWnd* pWnd, LPCTSTR pszShortcutFile, LPTSTR pszPath, int cchPath);

#ifndef _AFX_NO_OLE_SUPPORT
class AFX_COM
{
public:
	HRESULT CreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
		REFIID riid, LPVOID* ppv);
	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
};
#endif

#ifndef _AFX_NO_OLE_SUPPORT
CString AFXAPI AfxStringFromCLSID(REFCLSID rclsid);
BOOL AFXAPI AfxGetInProcServer(LPCTSTR lpszCLSID, CString& str);
#endif
