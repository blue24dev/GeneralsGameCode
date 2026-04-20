//MODDD - new file. Subclass the MFC library's 'CDocManager' for some finer behavior
// (todo: header stuff)

#include "StdAfx.h"
#include "StdAfx_private.h"
//#include "resource.h"
#include "WorldBuilderDocManager.h"

//TEST
//#include <stdlib.h>
#include <cstdlib>

// why is the 'vc6-debug+t+e' build refusing to cooperate here
#define _countof __crt_countof

#define __crt_countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))

// Copied from 'DOCMGR.CPP' in the MFC library (1998).
// Overriding 'CDocTemplate::MatchDocType' to never return 'yesAlreadyOpen' might also work.
CDocument* CWorldBuilderDocManager::OpenDocumentFile(LPCTSTR lpszFileName)
{
	// find the highest confidence
	POSITION pos = m_templateList.GetHeadPosition();
	CDocTemplate::Confidence bestMatch = CDocTemplate::noAttempt;
	CDocTemplate* pBestTemplate = NULL;
	CDocument* pOpenDocument = NULL;

	TCHAR szPath[_MAX_PATH];
	ASSERT(lstrlen(lpszFileName) < _countof(szPath));
	TCHAR szTemp[_MAX_PATH];
	if (lpszFileName[0] == '\"')
		++lpszFileName;
	lstrcpyn(szTemp, lpszFileName, _MAX_PATH);
	LPTSTR lpszLast = _tcsrchr(szTemp, '\"');
	if (lpszLast != NULL)
		*lpszLast = 0;
	AfxFullPath(szPath, szTemp);
	TCHAR szLinkName[_MAX_PATH];
	if (AfxResolveShortcut(AfxGetMainWnd(), szPath, szLinkName, _MAX_PATH))
		lstrcpy(szPath, szLinkName);

	while (pos != NULL)
	{
		CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetNext(pos);
		ASSERT_KINDOF(CDocTemplate, pTemplate);

		CDocTemplate::Confidence match;
		ASSERT(pOpenDocument == NULL);
		match = pTemplate->MatchDocType(szPath, pOpenDocument);

		//MODDD - if they match, open it anyway
		if (match == CDocTemplate::yesAlreadyOpen)
		{
			match = CDocTemplate::yesAttemptNative;
		}

		if (match > bestMatch)
		{
			bestMatch = match;
			pBestTemplate = pTemplate;
		}

		//MODDD - don't
		/*
		if (match == CDocTemplate::yesAlreadyOpen)
			break;      // stop here
		*/
	}

	//MODDD - this must be disabled too, since this checks that 'pOpenDocument' is set by 'pTemplate->MatchDocType' finding
	// the same document (non-null).
	// Looks like the purpose is to try to show the user the window that has the already opened document, such as somewhere in
	// a multi-window application. It would be neat to be able to keep that behavior if the same doc is already open in a
	// different window (assuming 'View' is 1-1 with a window), but allow opening the same document over itself (ex: same
	// window it's already opened in opening the same doc -> loading itself to deliberately revert unsaved changes), but it
	// looks like there isn't a way to tell the source of this 'OpenDocumentFile' call (which window/view is trying to open
	// this already-opened document).
	// This is more food-for-thought for modding the MFC library for other cases, not really the world builder, seeing as it's
	// not multi-window at all (multiple worldbuilder instancces are separate apps -> can't tell if you're opening the same
	// map even as-is).
	/*
	if (pOpenDocument != NULL)
	{
		POSITION pos = pOpenDocument->GetFirstViewPosition();
		if (pos != NULL)
		{
			CView* pView = pOpenDocument->GetNextView(pos); // get first one
			ASSERT_VALID(pView);
			CFrameWnd* pFrame = pView->GetParentFrame();
			if (pFrame != NULL)
				pFrame->ActivateFrame();
			else
				TRACE0("Error: Can not find a frame for document to activate.\n");
			CFrameWnd* pAppFrame;
			if (pFrame != (pAppFrame = (CFrameWnd*)AfxGetApp()->m_pMainWnd))
			{
				ASSERT_KINDOF(CFrameWnd, pAppFrame);
				pAppFrame->ActivateFrame();
			}
		}
		else
		{
			TRACE0("Error: Can not find a view for document to activate.\n");
		}
		return pOpenDocument;
	}
	*/

	if (pBestTemplate == NULL)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_OPEN_DOC);
		return NULL;
	}

	return pBestTemplate->OpenDocumentFile(szPath);
}
