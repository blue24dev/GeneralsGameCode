//MODDD - new file, to include dependencies needed in order to start some things from copies of the MFC library source code
// (todo: header stuff)

#include "StdAfx.h"

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
