/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// ImpassableOptions.cpp
// Author: John McDonald, April 2001

#include "StdAfx.h"
#include "resource.h"

#include "ImpassableOptions.h"
#include "W3DDevice/GameClient/HeightMap.h"
#include "wbview3d.h"
#include "WorldBuilderDoc.h"

//MODDD
#include "WHeightMapEdit.h"

//-------------------------------------------------------------------------------------------------
//MODDD - added param 'view'
ImpassableOptions::ImpassableOptions(CWnd* pParent, WbView3d* view, Real defaultSlope) :
	CDialog(ImpassableOptions::IDD, pParent),
	//MODDD
	m_view(view),
	m_slopeToShow(defaultSlope),
	m_defaultSlopeToShow(defaultSlope)
{
	// nada to do
	m_showImpassableAreas = TheTerrainRenderObject->getShowImpassableAreas();
	TheTerrainRenderObject->setShowImpassableAreas(TRUE);
}

//-------------------------------------------------------------------------------------------------
ImpassableOptions::~ImpassableOptions()
{
	TheTerrainRenderObject->setShowImpassableAreas(m_showImpassableAreas);
}

//MODDD - outcome is checked and reacted to here, moved from 'wbview3d.cpp'
int ImpassableOptions::DoModal()
{
	int res = CDialog::DoModal();
	if (res == IDOK) {
		// use what's in the text field at the moment
		TheTerrainRenderObject->setViewImpassableAreaSlope(m_slopeToShow);
	} else {
		// set this back to what was in the text field at first in case the preview feature changed it to something else
		TheTerrainRenderObject->setViewImpassableAreaSlope(m_defaultSlopeToShow);
	}

	// Run to apply the most recent 'setViewImpassableAreaSlope' call
	IRegion2D range = {0,0,0,0};
	m_view->updateHeightMapInView(m_view->WbDoc()->GetHeightMap(), false, range);

	return res;
}

//-------------------------------------------------------------------------------------------------
Bool ImpassableOptions::ValidateSlope()
{
	if (m_slopeToShow < 0.0f) {
		m_slopeToShow = 0.0f;
		return FALSE;
	}

	if (m_slopeToShow >= 90.0f) {
		m_slopeToShow = 89.9f;
		return FALSE;
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
BOOL ImpassableOptions::OnInitDialog()
{
	CWnd* pWnd = GetDlgItem(IDC_ANGLE);
	if (!pWnd) {
		return FALSE;
	}

	AsciiString astr;
	astr.format("%.2f", m_slopeToShow);
	pWnd->SetWindowText(astr.str());

	return CDialog::OnInitDialog();
}

//-------------------------------------------------------------------------------------------------
void ImpassableOptions::OnAngleChange()
{
	CWnd* pWnd = GetDlgItem(IDC_ANGLE);
	if (!pWnd) {
		return;
	}

	CString cstr;
	pWnd->GetWindowText(cstr);
	char* buff = cstr.GetBuffer(0);

	m_slopeToShow = (Real)atof(buff);

	if (!ValidateSlope()) {
		AsciiString astr;
		astr.format("%.2f", m_slopeToShow);
		pWnd->SetWindowText(astr.str());
	}
	//MODDD - why not wait for clicking the preview button to do this?
	//TheTerrainRenderObject->setViewImpassableAreaSlope(m_slopeToShow);
}

void ImpassableOptions::OnPreview()
{
	// update it.
	//WbView3d *pView = CWorldBuilderDoc::GetActive3DView();

	//MODDD - moved to here
	TheTerrainRenderObject->setViewImpassableAreaSlope(m_slopeToShow);

	IRegion2D range = {0,0,0,0};
	//MODDD - better resemble the final call, though I doubt there's any functional difference in this
	//pView->updateHeightMapInView(TheTerrainRenderObject->getMap(), false, range);
	m_view->updateHeightMapInView(m_view->WbDoc()->GetHeightMap(), false, range);
}

BEGIN_MESSAGE_MAP(ImpassableOptions, CDialog)
	ON_EN_UPDATE(IDC_ANGLE, OnAngleChange)
	ON_BN_CLICKED(IDC_PREVIEW, OnPreview)
END_MESSAGE_MAP()
