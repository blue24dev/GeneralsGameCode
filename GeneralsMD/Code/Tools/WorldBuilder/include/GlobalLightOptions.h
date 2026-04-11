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

#pragma once

// GLOBALLIGHTOPTIONS.h : header file
//

#include "WBPopupSlider.h"
#include "resource.h"
#include "CButtonShowColor.h"
//MODDD
#include "Common/GlobalData.h"

//MODDD
class Vector3;

/////////////////////////////////////////////////////////////////////////////
/// GlobalLightOptions modeless (floating) dialog - allows entry and display of brush width and feather.

#define  GLOBALLIGHT_OPTIONS_PANEL_SECTION "LightOptionsWindow"

class GlobalLightOptions : public CDialog	, public PopupSliderOwner
{
// Construction
public:
	enum {K_TERRAIN=1, K_OBJECTS=2, K_BOTH=3};
	enum {K_SUN=0, K_ACCENT1=1, K_ACCENT2=2};
	int kUIRedIDs[3];// = {IDC_RD_EDIT, IDC_RD_EDIT1, IDC_RD_EDIT2};
	int kUIGreenIDs[3];// = {IDC_GD_EDIT, IDC_GD_EDIT1, IDC_GD_EDIT2};
	int kUIBlueIDs[3];// = {IDC_BD_EDIT, IDC_BD_EDIT1, IDC_BD_EDIT2};
	CButtonShowColor m_colorButton;

	GlobalLightOptions(CWnd* pParent = nullptr);   // standard constructor

	//MODDD - several new methods
	// ---
	void onMapChangeStart();
	void onMapChangeEnd();
	void onTimeOfDayChanged();
	// ---

// Dialog Data
	//{{AFX_DATA(GlobalLightOptions)
	enum { IDD = IDD_GLOBAL_LIGHT_OPTIONS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(GlobalLightOptions)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(GlobalLightOptions)
	virtual BOOL OnInitDialog() override;
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnChangeSunFrontBackEdit();
	afx_msg void OnChangeAccent1FrontBackEdit();
	afx_msg void OnChangeAccent2FrontBackEdit();
	afx_msg void OnChangeSunLeftRightEdit();
	afx_msg void OnChangeAccent1LeftRightEdit();
	afx_msg void OnChangeAccent2LeftRightEdit();
	//MODDD
	// ---
	//afx_msg void OnChangeColorEdit();
	// ---
	afx_msg void OnChangeAmbientColorEdit();
	afx_msg void OnChangeSunColorEdit();
	afx_msg void OnChangeAccent1ColorEdit();
	afx_msg void OnChangeAccent2ColorEdit();
	// ---
	afx_msg void OnRadioEverything();
	afx_msg void OnRadioObjects();
	afx_msg void OnRadioTerrain();
	afx_msg void OnColorPress();
	afx_msg void OnResetLights();
	afx_msg void OnClose();
	virtual void OnOK() override {return;};  //!< Modeless dialogs don't OK, so eat this for modeless.
	virtual void OnCancel() override {return;}; //!< Modeless dialogs don't close on ESC, so eat this for modeless.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	//MODDD - implementations moved to .cpp
	Real	ComponentToPercent(Int component);
	Int		PercentToComponent(Real percent);
	BOOL	GetInt(Int ctrlID, Int *rVal);
	void	PutInt(Int ctrlID, Int val);

protected:

	Bool		m_updating; ///<true if the ui is updating itself.
	WBPopupSliderButton m_frontBackPopup;
	WBPopupSliderButton m_leftRightPopup;
	WBPopupSliderButton m_frontBackPopupAccent1;
	WBPopupSliderButton m_leftRightPopupAccent1;
	WBPopupSliderButton m_frontBackPopupAccent2;
	WBPopupSliderButton m_leftRightPopupAccent2;
	Int			m_angleAzimuth[3];
	Int			m_angleElevation[3];
	Int			m_lighting;

protected:
	void applyAngle(Int lightIndex=0);
	void showLightFeedback(Int lightIndex=0);
	void applyColor(Int lightIndex=0);
	void stuffValuesIntoFields(Int lightIndex = 0);

	//MODDD - several new methods (except 'updateEditFields', renamed)
	// ---
	void updateFields();
	const GlobalData::TerrainLighting* getTerrainLighting(Int lightIndex);
	GlobalData::TerrainLighting getTerrainLightingCopy(Int lightIndex);
	void horizontalCoordsToPosition(Vector3* out_lightPos, Int lightIndex);
	void updateColorButton();
	void updateColorFields(Int lightIndex);
	// renamed from 'updateEditFields' to 'updateHorizontalCoordFields' for clarity
	// Also, param 'lightIndex'
	void updateHorizontalCoordFields(Int lightIndex);
	void determineHorizontalCoords(Int lightIndex);
	void updateTimeOfDayDisplay();
	void updateLightPositionDisplay(Vector3* lightPos);
	void _OnResetLights();
	// ---

public:

	virtual void GetPopSliderInfo(const long sliderID, long *pMin, long *pMax, long *pLineSize, long *pInitial) override;
	virtual void PopSliderChanged(const long sliderID, long theVal) override;
	virtual void PopSliderFinished(const long sliderID, long theVal) override;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
