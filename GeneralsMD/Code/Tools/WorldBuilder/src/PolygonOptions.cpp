//MODDD - new file, somehow.
// See the explanation in the '.h' file.

// PolygonOptions.cpp : implementation file
//

#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "CUndoable.h"
#include "PolygonOptions.h"
#include "WorldBuilder.h"
#include "WorldBuilderDoc.h"
#include "wbview3d.h"
#include "PolygonTool.h"
#include "GameLogic/PolygonTrigger.h"
#include "GameLogic/Scripts.h"
#include "Common/WellKnownKeys.h"
#include "LayersList.h"

PolygonOptions *PolygonOptions::m_staticThis = nullptr;
/////////////////////////////////////////////////////////////////////////////
/// PolygonOptions dialog trivial constructor - Create does the real work.


PolygonOptions::PolygonOptions(CWnd* pParent /*=nullptr*/):
m_moveUndoable(nullptr)
{
	//{{AFX_DATA_INIT(PolygonOptions)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

/// Windows default stuff.
void PolygonOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PolygonOptions)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

//MODDD - previously in 'WaypointOptions'
PolygonTrigger *PolygonOptions::getSingleSelectedPolygon()
{
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (pDoc==nullptr) return nullptr;
	WbView3d *p3View = pDoc->GetActive3DView();
	Bool showPoly = false;
	if (p3View) {
		showPoly = p3View->isPolygonTriggerVisible();
	}
	if (showPoly || PolygonTool::isActive()) {
		for (PolygonTrigger *pTrig=PolygonTrigger::getFirstPolygonTrigger(); pTrig; pTrig = pTrig->getNext()) {
			if (PolygonTool::isSelected(pTrig)) {
				return pTrig;
			}
		}
	}
	return(nullptr);
}

//MODDD - NOTE - all of this method's contents began from leftovers from 'WaypointOptions::updateTheUI'
void PolygonOptions::updateTheUI()
{
	PolygonTrigger *theTrigger = PolygonOptions::getSingleSelectedPolygon();
	
	CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_POLYGONNAME_EDIT);
	// how would this even be possible?
	if (pCombo == nullptr)
	{
		return;
	}

	if (theTrigger) {
		pCombo->ResetContent();
		AsciiString trigger;
		trigger = INNER_PERIMETER;
		trigger.concat("1");
		pCombo->AddString(trigger.str());
		trigger = OUTER_PERIMETER;
		trigger.concat("1");
		pCombo->AddString(trigger.str());
		trigger = INNER_PERIMETER;
		trigger.concat("2");
		pCombo->AddString(trigger.str());
		trigger = OUTER_PERIMETER;
		trigger.concat("2");
		pCombo->AddString(trigger.str());
		trigger = INNER_PERIMETER;
		trigger.concat("3");
		pCombo->AddString(trigger.str());
		trigger = OUTER_PERIMETER;
		trigger.concat("3");
		pCombo->AddString(trigger.str());
		trigger = INNER_PERIMETER;
		trigger.concat("4");
		pCombo->AddString(trigger.str());
		trigger = OUTER_PERIMETER;
		trigger.concat("4");
		pCombo->AddString(trigger.str());
		pCombo->ShowWindow(SW_SHOW);
	}

	if (theTrigger)
	{
		setupUIForPolygon(theTrigger);
	}
	else
	{
		pCombo->EnableWindow(false);
		pCombo->SetWindowText("");
	}
}

void PolygonOptions::setupUIForPolygon(PolygonTrigger* theTrigger)
{
	CWnd *pWnd = this->GetDlgItem(IDC_POLYGONNAME_EDIT);
	//CWnd *pCaption1 = this->GetDlgItem(IDC_POLYGON_CAPTION);

	// Removed references to UI elements that no longer exist from back when this was part of 'WaypointOptions'

	//MODDD - uppercase: "Area name" -> "Area Name"
	// nevermind - no longer need to do this
	//pCaption1->SetWindowText("Area Name:");
	//MODDD - same
	//SetWindowText("Area Trigger Options");
	pWnd->SetWindowText(theTrigger->getTriggerName().str());
	pWnd->EnableWindow();
}

void PolygonOptions::update()
{
	if (m_staticThis) {
		m_staticThis->updateTheUI();
	}
}

/////////////////////////////////////////////////////////////////////////////
// PolygonOptions message handlers

/// Dialog UI initialization.
/** Creates the slider controls, and sets the initial values for
width and feather in the ui controls. */
BOOL PolygonOptions::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_updating = true;

	m_staticThis = this;
	m_updating = false;
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//MODDD - started from some relevant portions of 'WaypointOptions::OnChangeWaypointnameEdit()' 
void PolygonOptions::OnChangePolygonNameEdit()
{
	PolygonTrigger *theTrigger = PolygonOptions::getSingleSelectedPolygon();

	if (theTrigger == nullptr)
	{
		return;
	}

	// get the combo box
	CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_POLYGONNAME_EDIT);
	if (pCombo == nullptr)
	{
		return;
	}

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

	// check trigger area objects
	PolygonTrigger *pTrig;
	for (pTrig=PolygonTrigger::getFirstPolygonTrigger(); pTrig != nullptr; pTrig = pTrig->getNext()) {
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

	// if there's a match, throw up a messagebox, otherwise set the name
	if (didMatch) {
		::AfxMessageBox("Name already in use");
	} else {
		theTrigger->setTriggerName(name);
	}
}

// empty map - necessary?
BEGIN_MESSAGE_MAP(PolygonOptions, COptionsPanel)
	//{{AFX_MSG_MAP(PolygonOptions)
	ON_CBN_KILLFOCUS(IDC_POLYGONNAME_EDIT, OnChangePolygonNameEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
