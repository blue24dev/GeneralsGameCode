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

// terrainmaterial.h : header file
//

#include "WBPopupSlider.h"
#include "TerrainSwatches.h"
#include "OptionsPanel.h"
class WorldHeightMapEdit;
/////////////////////////////////////////////////////////////////////////////
// TerrainMaterial dialog

//MODDD - to control which of 3 choices the user has selected for painting pathing info since adding a 3rd one.
// In fact, may as well include painting terrain normally as a choice & eliminate the need for 'isPaintingPathingInfo'.
enum TerrainToolPaintType CPP_11(: Int)
{
	TERRAIN_TOOL_PAINT_TEXTURE,
	TERRAIN_TOOL_PAINT_PASSABLE,
	TERRAIN_TOOL_PAINT_IMPASSABLE,
	TERRAIN_TOOL_PAINT_UPDATE_FROM_HEIGHT
};

class TerrainMaterial : public COptionsPanel, public PopupSliderOwner
{
// Construction
public:
	TerrainMaterial(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(TerrainMaterial)
	enum { IDD = IDD_TERRAIN_MATERIAL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TerrainMaterial)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual void OnOK() override {return;};  ///< Modeless dialogs don't OK, so eat this for modeless.
	virtual void OnCancel() override {return;}; ///< Modeless dialogs don't close on ESC, so eat this for modeless.
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
	//}}AFX_VIRTUAL

// Implementation
protected:
	enum {MIN_TILE_SIZE=2, MAX_TILE_SIZE = 100};
	// Generated message map functions
	//{{AFX_MSG(TerrainMaterial)
	virtual BOOL OnInitDialog() override;
	afx_msg void OnSwapTextures();
	afx_msg void OnChangeSizeEdit();
	afx_msg void OnImpassable();
	afx_msg void OnPassableCheck();
	afx_msg void OnPassable();
	//MODDD
	afx_msg void OnUpdateFromHeight();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


protected:
	static TerrainMaterial	*m_staticThis;
	Bool										m_updating;
	static Int							m_currentFgTexture;
	static Int							m_currentBgTexture;
	CTreeCtrl								m_terrainTreeView;
	TerrainSwatches					m_terrainSwatches;
	WBPopupSliderButton			m_widthPopup;
	Int											m_currentWidth;

	//MODDD - replaced
	/*
	static Bool m_paintingPathingInfo;	 // If true, we are painting passable/impassable.  If false, normal texture painting.
	static Bool m_paintingPassable;
	*/
	static TerrainToolPaintType m_paintType;

protected:
	void addTerrain(char *pPath, Int terrainNdx, HTREEITEM parent);
	HTREEITEM findOrAdd(HTREEITEM parent, const char *pLabel);
	void updateLabel();

public:
	static Int getFgTexClass() {return m_currentFgTexture;}
	static Int getBgTexClass() {return m_currentBgTexture;}

	static void setFgTexClass(Int texClass);
	static void setBgTexClass(Int texClass);
	static void updateTextures(WorldHeightMapEdit *pMap);
	static void updateTextureSelection();
	static void setToolOptions(Bool singleCell);
	static void setWidth(Int width);

	//MODDD - replaced
	/*
	static Bool isPaintingPathingInfo() {return m_paintingPathingInfo;}
	static Bool isPaintingPassable() {return m_paintingPassable;}
	*/
	static TerrainToolPaintType getPaintType() {return m_paintType;}

public:
	Bool setTerrainTreeViewSelection(HTREEITEM parent, Int selection);

	// Popup slider interface.
	virtual void GetPopSliderInfo(const long sliderID, long *pMin, long *pMax, long *pLineSize, long *pInitial) override;
	virtual void PopSliderChanged(const long sliderID, long theVal) override;
	virtual void PopSliderFinished(const long sliderID, long theVal) override;

	//MODDD
	void setPaintTypeRadioButtonGroupEnabled(Bool enabled);
	TerrainToolPaintType getPaintTypeFromUI();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
