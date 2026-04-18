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

#include "StdAfx.h"

#include "GameClient/ParticleSys.h"
#include "CButtonShowColor.h"


//MODDD - new, override of a MFC event to conveniently add the 'BS_OWNERDRAW' style
BOOL CButtonShowColor::Create(LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	//MODDD - style added to avoid the pressed-in button appearance
	dwStyle |= BS_OWNERDRAW;
	return CButton::Create(lpszCaption, dwStyle, rect, pParentWnd, nID);
}

//MODDD - new
// Without overriding this method, clicking the button with this style causes a crash.
// The as-is 'OnPaint' still works as usual.
void CButtonShowColor::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// empty
}

// CButtonShowColor ///////////////////////////////////////////////////////////////////////////////
void CButtonShowColor::OnPaint()
{
	try {
		CPaintDC paintDC(this);

		CPen pen(PS_SOLID, 1, 0xFFFFFF00);
		CBrush brush(RGBtoBGR(m_color.getAsInt()));
		// Select my stuff
		CPen *oldPen = paintDC.SelectObject(&pen);

		CRect rect;
		GetWindowRect(&rect);
		ScreenToClient(&rect);

		paintDC.FillRect(&rect, &brush);
		paintDC.MoveTo(rect.left, rect.top);
		paintDC.LineTo(rect.right, rect.top);
		paintDC.LineTo(rect.right, rect.bottom);
		paintDC.LineTo(rect.left, rect.bottom);
		paintDC.LineTo(rect.left, rect.top);

		// Restore the states.
		paintDC.SelectObject(oldPen);

	} catch (...) {
		// Unlikely, but possible.
		return;
	}
}

CButtonShowColor::~CButtonShowColor()
{
	DestroyWindow();
}

//MODDD - implementations moved from .h (getColor and all setColor overloads)
const RGBColor& CButtonShowColor::getColor() const
{
	return m_color;
}

void CButtonShowColor::setColor(Int color)
{
	m_color.setFromInt(color);
	//MODDD - apply the edit immediately (more notes below)
	this->Invalidate();
}

void CButtonShowColor::setColor(const RGBColor& color)
{
	m_color = color;

	//MODDD - apply the edit immediately, needed in some cases where the button color is set without changes to
	// other UI to trigger a repaint naturally
	this->Invalidate();
	// 'this->UpdateWindow()' doesn't appear to be needed (nor did it work without 'Invalidate()' in several cases)
}

// Convert from 0x00RRGGBB to 0x00BBGGRR
COLORREF CButtonShowColor::RGBtoBGR(Int color)
{
	return ((color & 0x00FF0000) >> 16 |
					(color & 0x0000FF00) <<  0 |
					(color & 0x000000FF) << 16);
}

// Convert from 0x00BBGGRR to 0x00RRGGBB
Int CButtonShowColor::BGRtoRGB(COLORREF color)
{
	return RGBtoBGR(color);
}

BEGIN_MESSAGE_MAP(CButtonShowColor, CButton)
	ON_WM_PAINT()
END_MESSAGE_MAP()
