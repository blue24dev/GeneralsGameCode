//MODDD - new file. Subclass the MFC library's 'CDocManager' for some finer behavior
// (todo: header stuff)

#pragma once

#include "Common/STLTypedefs.h"

class CWorldBuilderDocManager : public CDocManager
{
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName) override; // open named file

};
