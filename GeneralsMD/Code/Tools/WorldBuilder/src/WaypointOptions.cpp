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

// WaypointOptions.cpp : implementation file
//

#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "CUndoable.h"
#include "WaypointOptions.h"
#include "WorldBuilder.h"
#include "WorldBuilderDoc.h"
#include "wbview3d.h"
//MODDD - disabled polygon includes
//#include "PolygonTool.h"
#include "WaypointTool.h"
//#include "GameLogic/PolygonTrigger.h"
#include "GameLogic/Scripts.h"
#include "Common/WellKnownKeys.h"
#include "LayersList.h"

WaypointOptions *WaypointOptions::m_staticThis = nullptr;
/////////////////////////////////////////////////////////////////////////////
/// WaypointOptions dialog trivial constructor - Create does the real work.


//MODDD - disabled initializer list, only member 'm_moveUndoable' no longer part of this class
WaypointOptions::WaypointOptions(CWnd* pParent /*=nullptr*/)/*:
m_moveUndoable(nullptr)
*/
{
	//{{AFX_DATA_INIT(WaypointOptions)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

/// Windows default stuff.
void WaypointOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(WaypointOptions)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

MapObject *WaypointOptions::getSingleSelectedWaypoint()
{
	MapObject *theMapObj = nullptr;
//	Bool found = false;
	Int selCount=0;
	MapObject *pMapObj;
	for (pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext()) {
		if (pMapObj->isSelected()) {
			if (pMapObj->isWaypoint()) {
				theMapObj = pMapObj;
			}
			selCount++;
		}
	}
	if (selCount==1 && theMapObj) {
		return theMapObj;
	}

	return(nullptr);
}

//MODDD - moved to the new 'PolygonOptions'
//PolygonTrigger *WaypointOptions::getSingleSelectedPolygon()
// ...

void WaypointOptions::updateTheUI()
{
	//MODDD NOTE - since 'PolygonTool' has been added, this class no longer needs to handle its functionality
	// Making several edits for this purpose
	// ---
	//MODDD - disabling
	/*
	Tool *curTool = ((CWorldBuilderApp*)AfxGetApp())->getCurTool();

	Bool isWaypointTool = (curTool && (curTool->getToolID() == ID_WAYPOINT_TOOL));
	*/

	MapObject *theMapObj = getSingleSelectedWaypoint();
	//MODDD - disabling
	//PolygonTrigger *theTrigger = WaypointOptions::getSingleSelectedPolygon();

	//MODDD - this separate reference to the same UI component is just confusing - replacing references to this with 'pCombo'
	//CWnd *pWnd = this->GetDlgItem(IDC_WAYPOINTNAME_EDIT);
	//MODDD - disabled, see point further down
	//CWnd *pCaption1 = this->GetDlgItem(IDC_WAYPOINT_CAPTION1);
	CWnd *pCaption2 = this->GetDlgItem(IDC_WAYPOINT_PATHLABELS);

	CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_WAYPOINTNAME_EDIT);
	CComboBox *pListWayptNames = (CComboBox*)GetDlgItem(IDC_LIST_OF_WAYPOINT_NAMES);
	
	//MODDD - represents the 'pWnd' seen in all the conditions in the chunk at the very bottom of this method as-is
	// (as in, the commented out "if ((theMapObj || isWaypointTool) ) {" now).
	// Just skip the rest if an important part of the UI couldn't be located.
	if (!pCombo || !pListWayptNames)
	{
		return;
	}

	//MODDD - disabled - since this class is only for waypoints, the UI will remain shown at all times, no need
	// to handle toggling these basic parts on/off
	/*
	if (theTrigger) {
		//MODDD - disabled visibility toggling for 'pCaption1'.
		// It's erroneously hidden when the waypoint dialog is shown, even though there's something for it to say in that case too.
		//pCaption1->ShowWindow(SW_SHOW);
		pCombo->ShowWindow(SW_SHOW);
	} else {
		//MODDD - per the point above
		//pCaption1->ShowWindow(SW_HIDE);
		pCaption2->ShowWindow(SW_HIDE);
		pCombo->ShowWindow(SW_HIDE);
	}
	*/

	//MODDD - removed '!theTrigger' part of condition: no longer applicable since this class is now only for waypoints.
	// The 'else' of this block was moved to the new 'PolygonTool.cpp'.
	//if (pCombo)
	{
		pCombo->ResetContent();
		pCombo->AddString((TheNameKeyGenerator->keyToName(TheKey_InitialCameraPosition)).str());
		pCombo->AddString((TheNameKeyGenerator->keyToName(TheKey_Player_1_Start)).str());
		pCombo->AddString((TheNameKeyGenerator->keyToName(TheKey_Player_2_Start)).str());
		pCombo->AddString((TheNameKeyGenerator->keyToName(TheKey_Player_3_Start)).str());
		pCombo->AddString((TheNameKeyGenerator->keyToName(TheKey_Player_4_Start)).str());
		pCombo->AddString((TheNameKeyGenerator->keyToName(TheKey_Player_5_Start)).str());
		pCombo->AddString((TheNameKeyGenerator->keyToName(TheKey_Player_6_Start)).str());
		pCombo->AddString((TheNameKeyGenerator->keyToName(TheKey_Player_7_Start)).str());
		pCombo->AddString((TheNameKeyGenerator->keyToName(TheKey_Player_8_Start)).str());
		pCombo->ShowWindow(SW_SHOW);
	}

	// display the list of waypoint names drop down menu
	//MODDD - removed the '!theTrigger' part of condition - 'theTrigger' is no longer applicable since this is only for waypoints.
	//if (pListWayptNames)
	{
		// reset everything and start fresh again
		pListWayptNames->ResetContent();

		// get the first map object, and then cycle through the rest
		MapObject *tempObj = MapObject::getFirstMapObject();
		while (true) {

			if (!tempObj)
				break;

			// if it is a waypoint, add its name to the combo box
			if (tempObj->isWaypoint())
				pListWayptNames->AddString(tempObj->getWaypointName().str());

			tempObj = tempObj->getNext();
		}

		// make sure the window is displayed
		pCombo->ShowWindow(SW_SHOW);
	}

	//MODDD - outright replacing the rest of this block since this can be greatly simplified by only handling the
	// waypoint case now
#if 0
	//MODDD - why the 'pWnd' (now 'pCombo') null checks, it was already referred to without null checks at all well before this point
	// Also, starting the condition with just 'isWaypointTool' to be easier to understand.
	//if ((theMapObj || isWaypointTool) ) {
	if (isWaypointTool) {
		if (theMapObj) {
			//MODDD - condensed
			setupUIForWaypoint(theMapObj);
			//MODDD - pasting the 'else' block from the polygon tool section further down.
			// If there's nothing to show, hide the UI all the same
		}	else {
			pCaption2->ShowWindow(SW_HIDE);
			pCombo->EnableWindow(false);
			pCombo->SetWindowText("");
		}
	}
	//MODDD - new separate block for if 'theMapObj' is present
	// ...actually, no. Going to let only the waypoint tool handle 'theMapObj' (it is the waypoint - why do anything
	// here for an incompatible tool?)
	/*
	else if (theMapObj)
	{
		setupUIForWaypoint(theMapObj);
	}
	*/
	//MODDD - wrapping the rest to make the intent this is for the "other tool" (polygon tool) more clear
	else
	{
		if (theTrigger) {
			//MODDD - condensed
			setupUIForPolygon(theTrigger);
		}	else {
			pCaption2->ShowWindow(SW_HIDE);
			pCombo->EnableWindow(false);
			pCombo->SetWindowText("");
		}
	}
#endif
	//MODDD - replacement
	// ------------
	if (theMapObj) {
		setupUIForWaypoint(theMapObj);
	} else {
		pCaption2->ShowWindow(SW_HIDE);
		pCombo->EnableWindow(false);
		pCombo->SetWindowText("");
	}
	// ------------
}

//MODDD
void WaypointOptions::setupUIForWaypoint(MapObject *theMapObj)
{
	CWnd *pWnd = this->GetDlgItem(IDC_WAYPOINTNAME_EDIT);
	//MODDD - disabled
	//CWnd *pCaption1 = this->GetDlgItem(IDC_WAYPOINT_CAPTION1);
	CWnd *pCaption2 = this->GetDlgItem(IDC_WAYPOINT_PATHLABELS);
	CWnd *pCaption5 = this->GetDlgItem(IDC_LIST_WAYPOINTS);

	CComboBox *pListWayptNames = (CComboBox*)GetDlgItem(IDC_LIST_OF_WAYPOINT_NAMES);

	CWnd *pWaypointLabel1 = GetDlgItem(IDC_WAYPOINTLABEL1_EDIT);
	CWnd *pWaypointLabel2 = GetDlgItem(IDC_WAYPOINTLABEL2_EDIT);
	CWnd *pWaypointLabel3 = GetDlgItem(IDC_WAYPOINTLABEL3_EDIT);

	CWnd *pWaypointLocation = this->GetDlgItem(IDC_WAYPOINT_LOCATION);
	CWnd *pWaypointX = GetDlgItem(IDC_WAYPOINT_LOCATIONX);
	CWnd *pWaypointY = GetDlgItem(IDC_WAYPOINT_LOCATIONY);
	CButton *pBiDirCheck = (CButton *)GetDlgItem(IDC_WAYPOINT_BIDIRECTIONAL);

	Bool exists;
	pWnd->EnableWindow();
	pWnd->SetWindowText(theMapObj->getProperties()->getAsciiString(TheKey_waypointName).str());
	//MODDD - uppercase text: "Waypoint name" -> "Waypoint Name"
	// (nevermind, disabled - never changed from initial state now)
	//pCaption1->SetWindowText("Waypoint Name:");
	//MODDD - same
	//SetWindowText("Waypoint Options");
	CWorldBuilderDoc* pDoc = CWorldBuilderDoc::GetActiveDoc();

	/* display location of waypoint */

	pWaypointLocation->ShowWindow(SW_SHOW);
	pWaypointY->ShowWindow(SW_SHOW);
	pWaypointX->ShowWindow(SW_SHOW);
	pListWayptNames->ShowWindow(SW_SHOW);
	pCaption5->ShowWindow(SW_SHOW);
	const Coord3D *waypointLocation = getSingleSelectedWaypoint()->getLocation();
	AsciiString locX, locY;

	// convert the location coordinates to strings
	locX.format("%f", waypointLocation->x);
	locY.format("%f", waypointLocation->y);

	// set the window text to reflect the current position of the waypoint
	pWaypointX->SetWindowText(locX.str());
	pWaypointY->SetWindowText(locY.str());

	if (pDoc->isWaypointLinked(theMapObj)) {
		pCaption2->ShowWindow(SW_SHOW);
		AsciiString name;
		name = theMapObj->getProperties()->getAsciiString(TheKey_waypointPathLabel1, &exists);
		pWaypointLabel1->ShowWindow(SW_SHOW);
		pWaypointLabel1->SetWindowText(name.str());
		name = theMapObj->getProperties()->getAsciiString(TheKey_waypointPathLabel2, &exists);
		pWaypointLabel2->ShowWindow(SW_SHOW);
		pWaypointLabel2->SetWindowText(name.str());
		name = theMapObj->getProperties()->getAsciiString(TheKey_waypointPathLabel3, &exists);
		pWaypointLabel3->ShowWindow(SW_SHOW);
		pWaypointLabel3->SetWindowText(name.str());
	} else {
		pCaption2->ShowWindow(SW_HIDE);
		pWaypointLabel1->ShowWindow(SW_HIDE);
		pWaypointLabel2->ShowWindow(SW_HIDE);
		pWaypointLabel3->ShowWindow(SW_HIDE);
	}
	if (pBiDirCheck) {
		pBiDirCheck->ShowWindow(SW_SHOW);
		Bool checked = theMapObj->getProperties()->getBool(TheKey_waypointPathBiDirectional, &exists);
		pBiDirCheck->SetCheck(checked);
	}
}

//MODDD - 'WaypointOptions::setupUIForPolygon' has been moved to the since-added 'PolygonOptions.cpp'

void WaypointOptions::update()
{
	if (m_staticThis) {
		m_staticThis->updateTheUI();
	}
}

/////////////////////////////////////////////////////////////////////////////
// WaypointOptions message handlers

/// Dialog UI initialization.
/** Creates the slider controls, and sets the initial values for
width and feather in the ui controls. */
BOOL WaypointOptions::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_updating = true;

	m_staticThis = this;
	m_updating = false;
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BEGIN_MESSAGE_MAP(WaypointOptions, COptionsPanel)
	//{{AFX_MSG_MAP(WaypointOptions)
	ON_CBN_KILLFOCUS(IDC_WAYPOINTNAME_EDIT, OnChangeWaypointnameEdit)
	ON_CBN_KILLFOCUS(IDC_LIST_OF_WAYPOINT_NAMES, OnChangeSelectedWaypoint)
	ON_EN_CHANGE(IDC_WAYPOINT_LOCATIONX, OnEditWaypointLocationX)
	ON_EN_CHANGE(IDC_WAYPOINT_LOCATIONY, OnEditWaypointLocationY)
	ON_EN_CHANGE(IDC_WAYPOINTLABEL1_EDIT, OnEditchangeWaypointlabel1Edit)
	ON_EN_CHANGE(IDC_WAYPOINTLABEL2_EDIT, OnEditchangeWaypointlabel2Edit)
	ON_EN_CHANGE(IDC_WAYPOINTLABEL3_EDIT, OnEditchangeWaypointlabel3Edit)
	ON_CBN_SELENDOK(IDC_LIST_OF_WAYPOINT_NAMES, OnChangeSelectedWaypoint)
	//MODDD - this doesn't seem necessary, killing the focus after making the selection will tell you if there's a problem
	//ON_CBN_SELENDOK(IDC_WAYPOINTNAME_EDIT, OnChangeWaypointnameEdit)
	ON_BN_CLICKED(IDC_WAYPOINT_BIDIRECTIONAL, OnWaypointBidirectional)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void WaypointOptions::OnChangeSelectedWaypoint()
{
	// deselect whatever is currently selected
	MapObject *currentlySelected = getSingleSelectedWaypoint(), *pMapObj, *waypt;
	if (!currentlySelected)
		return;
	currentlySelected->setSelected(false);

	// retrieve information from dialog box, if user-typed -- sel will be -1, otherwise it will be >=0
	CString theText;
	CComboBox *pListWayptNames = (CComboBox*)GetDlgItem(IDC_LIST_OF_WAYPOINT_NAMES);
	Int sel = pListWayptNames->GetCurSel();
	if (sel >= 0) {
		pListWayptNames->GetLBText(sel, theText);
	} else {
		pListWayptNames->GetWindowText(theText);
	}
	AsciiString name((LPCTSTR)theText);

	// find and store the waypoint that corresponds to the information in the dialog box
	Bool foundWaypoint = false;
	for (pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext()) {
		if (pMapObj->isWaypoint()) {
			if (pMapObj->getWaypointName() != AsciiString::TheEmptyString) {
				if (pMapObj->getWaypointName() == name) {
					foundWaypoint = true;
					waypt = pMapObj;
					break;
				}
			}
		}
	}

	// if no waypoint corresponds to whatever is in the dialog box, then don't do anything
	if (!foundWaypoint)
		return;

	// select the waypoint and set camera position to position of waypoint
	waypt->setSelected(true);
	Coord3D pos = *waypt->getLocation();
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (pDoc) {
		WbView3d *p3View = pDoc->GetActive3DView();
		if (p3View) {
			p3View->setCenterInView(pos.x/MAP_XY_FACTOR, pos.y/MAP_XY_FACTOR);
		}
	}

	update();
}

void WaypointOptions::OnEditWaypointLocationX()
{
	CString tempName;
	AsciiString name;

	//make sure that only one waypoint is being selected currently
	MapObject *waypt = getSingleSelectedWaypoint();
	if (!waypt)
		return;

	// get old waypoint information
	const Coord3D *waypointLocation = waypt->getLocation();
	Coord3D newWaypointLocation;

	// retrieve information from dialog box
	CWnd *pWnd = GetDlgItem(IDC_WAYPOINT_LOCATIONX);
	pWnd->GetWindowText(tempName);
	name = ((LPCTSTR)tempName);

	// make sure that there is a value in the field, otherwise funky stuff would happen
	if (name.isEmpty())
		return;

	// determine new values of waypoint location
	newWaypointLocation.x = atof(name.str());
	newWaypointLocation.y = waypointLocation->y;
	newWaypointLocation.z = 0;

	// set the new information into both the waypointa and the window
	waypt->setLocation(&newWaypointLocation);
}

void WaypointOptions::OnEditWaypointLocationY()
{
	CString tempName;
	AsciiString name;

	//make sure that only one waypoint is being selected currently
	MapObject *waypt = getSingleSelectedWaypoint();
	if (!waypt)
		return;

	// get old waypoint information
	const Coord3D *waypointLocation = waypt->getLocation();
	Coord3D newWaypointLocation;

	// retrieve information from dialog box
	CWnd *pWnd = GetDlgItem(IDC_WAYPOINT_LOCATIONY);
	pWnd->GetWindowText(tempName);
	name = ((LPCTSTR)tempName);

	// make sure that there is a value in the field, otherwise funky stuff would happen
	if (name.isEmpty())
		return;

	// determine new values of waypoint location
	newWaypointLocation.y = atof(name.str());
	newWaypointLocation.x = waypointLocation->x;
	newWaypointLocation.z = 0;

	// set the new information into both the waypointa and the window
	waypt->setLocation(&newWaypointLocation);
}

Bool WaypointOptions::isUnique(AsciiString name, MapObject* theMapObj)
{
	MapObject *pMapObj;
	Bool didMatch = false;
	for (pMapObj = MapObject::getFirstMapObject(); !didMatch && pMapObj; pMapObj = pMapObj->getNext()) {
		if (pMapObj->isWaypoint()) {
			if (pMapObj == theMapObj) continue; // don't check against self.
			AsciiString wayName = pMapObj->getWaypointName();
			if (name == wayName) {
				didMatch = true;
				break;
			}
		}
	}
	return (didMatch == false);
}

AsciiString WaypointOptions::GenerateUniqueName(Int id)
{
	AsciiString name;
	name.format("Waypoint %d", id);
	while (!isUnique(name)) {
		id++;
		name.format("Waypoint %d", id);
	}

	return name;
}

void WaypointOptions::OnChangeWaypointnameEdit()
{
	MapObject *theMapObj = getSingleSelectedWaypoint();
	//MODDD - no longer handled here, see the new 'PolygonOptions'
	//PolygonTrigger *theTrigger = WaypointOptions::getSingleSelectedPolygon();

	//MODDD - added small block - why bother proceeding if there isn't a selected waypoint?
	if (theMapObj == nullptr)
	{
		return;
	}

	// get the combo box
	CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_WAYPOINTNAME_EDIT);
	if (pCombo) {
		// get the text out of the combo. If it is user-typed, sel will be -1, otherwise it will be >=0
		CString theText;
		Int sel = pCombo->GetCurSel();
		if (sel >= 0) {
			pCombo->GetLBText(sel, theText);
		} else {
			pCombo->GetWindowText(theText);
		}
		AsciiString name((LPCTSTR)theText);

		// check to see if the user-entered name is already in use.
		Bool didMatch = false;

		// check waypoint objects.
		didMatch = !isUnique(name, theMapObj);

		//MODDD - moved to the new 'PolygonOptions'
		/*
		// check trigger area objects
		PolygonTrigger *pTrig;
		for (pTrig=PolygonTrigger::getFirstPolygonTrigger(); !didMatch && pTrig; pTrig = pTrig->getNext()) {
			if (pTrig==theTrigger) continue; // don't check against yourself.
			const AsciiString& trigName = pTrig->getTriggerName();
			if (name == trigName) {
				if (pTrig->isValid()) {
					didMatch = true;
				} else {
					PolygonTrigger::removePolygonTrigger(pTrig);
				}
				break;
			}
		}
		*/

		// if there's a match, throw up a messagebox, otherwise set the name
		if (didMatch) {
			::AfxMessageBox("Name already in use");
		} else {
			//MODDD - disabled - condition redundant, 'else' nonapplicable
			//if (theMapObj) {
				AsciiString layerName = TheLayersList->removeMapObjectFromLayersList(theMapObj);
				theMapObj->setWaypointName(name);
				theMapObj->validate();
				TheLayersList->addMapObjectToLayersList(theMapObj, layerName);
			//MODDD - disabled per above
			/*
			}	else if (theTrigger) {
				theTrigger->setTriggerName(name);
			}
			*/
		}
	}
}


void WaypointOptions::OnEditchangeWaypointlabel1Edit()
{
	changeWaypointLabel(IDC_WAYPOINTLABEL1_EDIT, TheKey_waypointPathLabel1);
}

void WaypointOptions::changeWaypointLabel(Int editControlID, NameKeyType key)
{
	MapObject *theMapObj = getSingleSelectedWaypoint();
	if (theMapObj==nullptr) return;
	CWorldBuilderDoc* pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (!pDoc->isWaypointLinked(theMapObj)) {
		return;
	}
	// get the edit box
	CWnd *pWnd = GetDlgItem(editControlID);
	if (pWnd) {
		// get the text
		CString theText;
		pWnd->GetWindowText(theText);
		AsciiString name((LPCTSTR)theText);
		theMapObj->getProperties()->setAsciiString(key, name);
		pDoc->updateLinkedWaypointLabels(theMapObj);
	}
}

void WaypointOptions::OnEditchangeWaypointlabel2Edit()
{
	changeWaypointLabel(IDC_WAYPOINTLABEL2_EDIT, TheKey_waypointPathLabel2);
}

void WaypointOptions::OnEditchangeWaypointlabel3Edit()
{
	changeWaypointLabel(IDC_WAYPOINTLABEL3_EDIT, TheKey_waypointPathLabel3);
}

void WaypointOptions::OnWaypointBidirectional()
{
	MapObject *theMapObj = getSingleSelectedWaypoint();
	if (theMapObj==nullptr) return;
	CWorldBuilderDoc* pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (!pDoc->isWaypointLinked(theMapObj)) {
		return;
	}
	// get the edit box
	CButton *pWnd = (CButton *)GetDlgItem(IDC_WAYPOINT_BIDIRECTIONAL);
	if (pWnd) {
		Bool checked = pWnd->GetCheck()==1;
		// get the text
		CString theText;
		pWnd->GetWindowText(theText);
		AsciiString name((LPCTSTR)theText);
		theMapObj->getProperties()->setBool(TheKey_waypointPathBiDirectional, checked);
		pDoc->updateLinkedWaypointLabels(theMapObj);
	}
}
