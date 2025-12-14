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
#include "resource.h"

#include "BorderTool.h"
#include "DrawObject.h"
#include "MainFrm.h"
#include "wbview3d.h"
#include "WorldBuilderDoc.h"

const long BOUNDARY_PICK_DISTANCE = 5.0f;

BorderTool::BorderTool() : Tool(ID_BORDERTOOL, IDC_POINTER),
													 m_mouseDown(false),
													 m_addingNewBorder(false),
													 m_modifyBorderNdx(-1),
													 //MODDD - may as well
													 m_modificationType(BORDERMOD_TYPE_INVALID)

{ }

BorderTool::~BorderTool()
{

}

void BorderTool::setCursor(void)
{

}

void BorderTool::activate()
{
	CMainFrame::GetMainFrame()->showOptionsDialog(IDD_NO_OPTIONS);
	DrawObject::setDoBoundaryFeedback(TRUE);
}

void BorderTool::deactivate()
{
	WbView3d *p3View = CWorldBuilderDoc::GetActive3DView();
	DrawObject::setDoBoundaryFeedback(p3View->getShowMapBoundaryFeedback());
}

void BorderTool::mouseMoved(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc)
{
	if (m != TRACK_L) {
		return;
	}

	//MODDD - disabling this block. Turns out the one below is nearly the same and can handle dragging for new
	// borders too, just treat it as FREE dragging
	/*
	if (m_addingNewBorder) {
		Int count = pDoc->getNumBoundaries();
		ICoord2D current;
		pDoc->getBoundary(count - 1, &current);
		Coord3D new3DPoint;
		pView->viewToDocCoords(viewPt, &new3DPoint, false);

		current.x = REAL_TO_INT((new3DPoint.x / MAP_XY_FACTOR) + 0.5f);
		current.y = REAL_TO_INT((new3DPoint.y / MAP_XY_FACTOR) + 0.5f);

		//MODDD - moved from above to here so it's after the 'current.x/.y' assignment as likely intended
		if (current.x < 0) {
			current.x = 0;
		}

		if (current.y < 0) {
			current.y = 0;
		}

		pDoc->changeBoundary(count - 1, &current);
		return;
	}
	*/

	if (m_modifyBorderNdx >= 0) {
		ICoord2D currentBorder;
		pDoc->getBoundary(m_modifyBorderNdx, &currentBorder);

		Coord3D new3DPoint;
		pView->viewToDocCoords(viewPt, &new3DPoint, false);

		switch (m_modificationType)
		{
			//MODDD - Removing this case. Setting the border NDX here is redundant anyway.
			// In fact, an INVALID type already forces m_modifyBorderNdx to -1 so this shouldn't be reached then.
			//case MOD_TYPE_INVALID: m_modifyBorderNdx = -1; return;
			case BORDERMOD_TYPE_UP:
				currentBorder.y = REAL_TO_INT((new3DPoint.y / MAP_XY_FACTOR) + 0.5f);
				break;
			case BORDERMOD_TYPE_RIGHT:
				currentBorder.x = REAL_TO_INT((new3DPoint.x / MAP_XY_FACTOR) + 0.5f);
				break;
			case BORDERMOD_TYPE_FREE:
				currentBorder.x = REAL_TO_INT((new3DPoint.x / MAP_XY_FACTOR) + 0.5f);
				currentBorder.y = REAL_TO_INT((new3DPoint.y / MAP_XY_FACTOR) + 0.5f);
				break;
			//MODDD - give up on anything else if ever possible?
			default:
				return;
		}

		if (currentBorder.x < 0) {
			currentBorder.x = 0;
		}

		if (currentBorder.y < 0) {
			currentBorder.y = 0;
		}

		pDoc->changeBoundary(m_modifyBorderNdx, &currentBorder);
	}
}

void BorderTool::mouseDown(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc)
{
	if (m != TRACK_L) {
		return;
	}

	//static Coord3D zero = {0.0f, 0.0f, 0.0f};

	Coord3D groundPt;
	pView->viewToDocCoords(viewPt, &groundPt);

	//MODDD - I disagree with this block, disabling.
	// The same tolerance range against the lower-left corner of the map being hit means a tiny border that
	// happens to be at the bottom-left corner becomes a lot harder to select.  Clicking anywhere else to start
	// a new border sized there is easy enough to do instead anyway.
	/*
	if (groundPt.length() < BOUNDARY_PICK_DISTANCE) {
		m_addingNewBorder = true;

		ICoord2D initialBoundary = { 1, 1 };
		pDoc->addBoundary(&initialBoundary);
		return;
	}
	*/

	//MODDD - reset this, seems like good practice
	m_addingNewBorder = false;

	//MODDD - change tempvar type/name
	// Actually remove it, just save to the member version like 'm_modifyBorderNdx'.
	//Int motion;
	//pDoc->findBoundaryNear(&groundPt, BOUNDARY_PICK_DISTANCE, &m_modifyBorderNdx, &motion);
	pDoc->findBoundaryNear(&groundPt, BOUNDARY_PICK_DISTANCE, &m_modifyBorderNdx, &m_modificationType);

	//MODDD - comparisons to numbers replaced with enum values
	// if bottom left boundary grabbed
	if (m_modificationType == BORDERMOD_TYPE_INVALID)
	{
		// modifying the bottom left is not allowed.
		m_modifyBorderNdx = -1;
	}
	// else if no boundary is near
	else if (m_modificationType == BORDERMOD_TYPE_NONE)
	{
		// add a boundary
		m_addingNewBorder = true;

		//MODDD - replacing this. Set the initial boundary to the mouse-pos like free dragging at the current mouse
		// position. That way clicking without moving the mouse at all doesn't leave the border really tiny (a
		// high-friction mouse will make this issue apparent).
		//ICoord2D initialBoundary = { 1, 1 };
		// ---
		ICoord2D initialBoundary;
		Coord3D new3DPoint;
		pView->viewToDocCoords(viewPt, &new3DPoint, false);
		initialBoundary.x = REAL_TO_INT((new3DPoint.x / MAP_XY_FACTOR) + 0.5f);
		initialBoundary.y = REAL_TO_INT((new3DPoint.y / MAP_XY_FACTOR) + 0.5f);
		if (initialBoundary.x < 0) {
			initialBoundary.x = 0;
		}
		if (initialBoundary.y < 0) {
			initialBoundary.y = 0;
		}
		// ---

		pDoc->addBoundary(&initialBoundary);
		//MODDD - set 'm_modifyBorderNdx' so it's possible to tell what to delete if a created border is sized to
		// (0, 0) without releasing the mouse.
		m_modifyBorderNdx = pDoc->getNumBoundaries() - 1;

		//MODDD - use FREE type to size towards the top or right sides, same as changing an
		// existing border from the top-right corner.
		m_modificationType = BORDERMOD_TYPE_FREE;
	}
}

void BorderTool::mouseUp(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc)
{
	if (m != TRACK_L) {
		return;
	}

	if (m_addingNewBorder) {
		m_addingNewBorder = false;
		// Do the undoable on the last border
	}

	//MODDD - if the resulting border coords are (0,0), or either coord is 0 (border isn't even rendered),
	// delete the border. A border with a width/height of 0 isn't very useful and there isn't any other way to
	// signal for deleting a border anyway.
	if (m_modifyBorderNdx >= 0) {
		ICoord2D currentBorder;
		pDoc->getBoundary(m_modifyBorderNdx, &currentBorder);
		if (currentBorder.x == 0 || currentBorder.y == 0) {
			// Delete it.
			// Note that this expects the clear-logic below to set 'm_modifyBorderNdx' back to -1.
			// Can do it here & return if things below get much more complicated.
			// MODDD - TODO - On deleting a border, go through all script actions involving the border
			// to refer to the border at one index less if they refered to an index above the index deleted.
			// Ones that referred to the deleted index exactly should complain that they need to be set to something else
			// and not just accept the next boundary that then occupies that color.
			pDoc->removeBoundary(m_modifyBorderNdx);
		}
	}

	//MODDD - for safety, may as well reset these on releasing the mouse.
	m_modificationType = BORDERMOD_TYPE_INVALID;
	m_modifyBorderNdx = -1;
}
