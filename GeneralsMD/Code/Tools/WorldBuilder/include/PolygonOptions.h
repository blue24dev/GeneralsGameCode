//MODDD - new file, somehow - copied from WaterOptions and trimmed down
// If there is a unique file for the water tool's options (WaterOptions.cpp), why isn't there for the polygon tool?
// This should ease some confusion from WaypointOptions having to imply whether a waypoint or polygon (area)
// is selected.
// The main motivation for this new file was in 'WaypointOptions::updateTheUI()':
//   Tool *curTool = ((CWorldBuilderApp*)AfxGetApp())->getCurTool();
// This is seemingly a good idea at first, but more often than not, this is still the pointer tool instead of either the
// waypoint or polygon tools, resulting in some inconsistent behavior on changing the selection between a waypoint / polygon,
// often depending on which existing tool was most recently active to decide how the options dialog is filled.

#pragma once

// PolygonOptions.h : header file
//
#include "WBPopupSlider.h"
#include "OptionsPanel.h"
#include "Common/WellKnownKeys.h"

class MapObject;
class PolygonTrigger;
class MovePolygonUndoable;
/////////////////////////////////////////////////////////////////////////////
// PolygonOptions dialog

class PolygonOptions : public COptionsPanel
{

// Construction
public:
	PolygonOptions(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(PolygonOptions)
	enum { IDD = IDD_POLYGON_OPTIONS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PolygonOptions)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual void OnOK() override {return;};  //!< Modeless dialogs don't OK, so eat this for modeless.
	virtual void OnCancel() override {return;}; //!< Modeless dialogs don't close on ESC, so eat this for modeless.
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PolygonOptions)
	virtual BOOL OnInitDialog() override;
	afx_msg void OnChangePolygonNameEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	static PolygonOptions *m_staticThis;  ///< Reference to the floating panel so SetWidth and SetFeather can be static.
	Bool		m_updating; ///<true if the ui is updating itself.
	MovePolygonUndoable *m_moveUndoable;
protected:
	void updateTheUI();

	void setupUIForPolygon(PolygonTrigger* theTrigger);
	PolygonTrigger *adjustCount(PolygonTrigger *trigger, Int firstPt, Int lastPt, Int desiredPointCount);

public:
	static void update();
	static PolygonTrigger *getSingleSelectedPolygon();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
