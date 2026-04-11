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

// GlobalLightOptions.cpp : implementation file
//

#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "GlobalLightOptions.h"
#include "WorldBuilderDoc.h"
//MODDD - 'include' moved to .h
//#include "Common/GlobalData.h"
#include "wbview3d.h"

//MODDD
#include "Common/SubsystemInterface.h"

//MODDD
// ---------------
extern SubsystemInterfaceList TheSubsystemListRecord;

//MODDD - copy of GlobalData.cpp's 'GlobalData::s_GlobalDataFieldParseTable' with only relevant fields for this WorldBuilder panel
const FieldParse s_GlobalData_Lighting_FieldParseTable[] =
{
	{ "TerrainLightingMorningAmbient",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_MORNING ][0].ambient ) },
	{ "TerrainLightingMorningDiffuse",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_MORNING ][0].diffuse ) },
	{ "TerrainLightingMorningLightPos",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_MORNING ][0].lightPos ) },
	{ "TerrainLightingAfternoonAmbient",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_AFTERNOON ][0].ambient ) },
	{ "TerrainLightingAfternoonDiffuse",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_AFTERNOON ][0].diffuse ) },
	{ "TerrainLightingAfternoonLightPos",	INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_AFTERNOON ][0].lightPos ) },
	{ "TerrainLightingEveningAmbient",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_EVENING ][0].ambient ) },
	{ "TerrainLightingEveningDiffuse",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_EVENING ][0].diffuse ) },
	{ "TerrainLightingEveningLightPos",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_EVENING ][0].lightPos ) },
	{ "TerrainLightingNightAmbient",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_NIGHT ][0].ambient ) },
	{ "TerrainLightingNightDiffuse",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_NIGHT ][0].diffuse ) },
	{ "TerrainLightingNightLightPos",			INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_NIGHT ][0].lightPos ) },

	{ "TerrainObjectsLightingMorningAmbient",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_MORNING ][0].ambient ) },
	{ "TerrainObjectsLightingMorningDiffuse",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_MORNING ][0].diffuse ) },
	{ "TerrainObjectsLightingMorningLightPos",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_MORNING ][0].lightPos ) },
	{ "TerrainObjectsLightingAfternoonAmbient",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_AFTERNOON ][0].ambient ) },
	{ "TerrainObjectsLightingAfternoonDiffuse",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_AFTERNOON ][0].diffuse ) },
	{ "TerrainObjectsLightingAfternoonLightPos",	INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_AFTERNOON ][0].lightPos ) },
	{ "TerrainObjectsLightingEveningAmbient",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_EVENING ][0].ambient ) },
	{ "TerrainObjectsLightingEveningDiffuse",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_EVENING ][0].diffuse ) },
	{ "TerrainObjectsLightingEveningLightPos",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_EVENING ][0].lightPos ) },
	{ "TerrainObjectsLightingNightAmbient",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_NIGHT ][0].ambient ) },
	{ "TerrainObjectsLightingNightDiffuse",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_NIGHT ][0].diffuse ) },
	{ "TerrainObjectsLightingNightLightPos",			INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_NIGHT ][0].lightPos ) },

	//Secondary global light
	{ "TerrainLightingMorningAmbient2",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_MORNING ][1].ambient ) },
	{ "TerrainLightingMorningDiffuse2",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_MORNING ][1].diffuse ) },
	{ "TerrainLightingMorningLightPos2",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_MORNING ][1].lightPos ) },
	{ "TerrainLightingAfternoonAmbient2",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_AFTERNOON ][1].ambient ) },
	{ "TerrainLightingAfternoonDiffuse2",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_AFTERNOON ][1].diffuse ) },
	{ "TerrainLightingAfternoonLightPos2",	INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_AFTERNOON ][1].lightPos ) },
	{ "TerrainLightingEveningAmbient2",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_EVENING ][1].ambient ) },
	{ "TerrainLightingEveningDiffuse2",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_EVENING ][1].diffuse ) },
	{ "TerrainLightingEveningLightPos2",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_EVENING ][1].lightPos ) },
	{ "TerrainLightingNightAmbient2",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_NIGHT ][1].ambient ) },
	{ "TerrainLightingNightDiffuse2",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_NIGHT ][1].diffuse ) },
	{ "TerrainLightingNightLightPos2",			INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_NIGHT ][1].lightPos ) },

	{ "TerrainObjectsLightingMorningAmbient2",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_MORNING ][1].ambient ) },
	{ "TerrainObjectsLightingMorningDiffuse2",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_MORNING ][1].diffuse ) },
	{ "TerrainObjectsLightingMorningLightPos2",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_MORNING ][1].lightPos ) },
	{ "TerrainObjectsLightingAfternoonAmbient2",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_AFTERNOON ][1].ambient ) },
	{ "TerrainObjectsLightingAfternoonDiffuse2",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_AFTERNOON ][1].diffuse ) },
	{ "TerrainObjectsLightingAfternoonLightPos2",	INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_AFTERNOON ][1].lightPos ) },
	{ "TerrainObjectsLightingEveningAmbient2",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_EVENING ][1].ambient ) },
	{ "TerrainObjectsLightingEveningDiffuse2",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_EVENING ][1].diffuse ) },
	{ "TerrainObjectsLightingEveningLightPos2",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_EVENING ][1].lightPos ) },
	{ "TerrainObjectsLightingNightAmbient2",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_NIGHT ][1].ambient ) },
	{ "TerrainObjectsLightingNightDiffuse2",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_NIGHT ][1].diffuse ) },
	{ "TerrainObjectsLightingNightLightPos2",			INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_NIGHT ][1].lightPos ) },

	//Third global light
	{ "TerrainLightingMorningAmbient3",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_MORNING ][2].ambient ) },
	{ "TerrainLightingMorningDiffuse3",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_MORNING ][2].diffuse ) },
	{ "TerrainLightingMorningLightPos3",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_MORNING ][2].lightPos ) },
	{ "TerrainLightingAfternoonAmbient3",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_AFTERNOON ][2].ambient ) },
	{ "TerrainLightingAfternoonDiffuse3",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_AFTERNOON ][2].diffuse ) },
	{ "TerrainLightingAfternoonLightPos3",	INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_AFTERNOON ][2].lightPos ) },
	{ "TerrainLightingEveningAmbient3",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_EVENING ][2].ambient ) },
	{ "TerrainLightingEveningDiffuse3",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_EVENING ][2].diffuse ) },
	{ "TerrainLightingEveningLightPos3",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_EVENING ][2].lightPos ) },
	{ "TerrainLightingNightAmbient3",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_NIGHT ][2].ambient ) },
	{ "TerrainLightingNightDiffuse3",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_NIGHT ][2].diffuse ) },
	{ "TerrainLightingNightLightPos3",			INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainLighting[ TIME_OF_DAY_NIGHT ][2].lightPos ) },

	{ "TerrainObjectsLightingMorningAmbient3",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_MORNING ][2].ambient ) },
	{ "TerrainObjectsLightingMorningDiffuse3",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_MORNING ][2].diffuse ) },
	{ "TerrainObjectsLightingMorningLightPos3",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_MORNING ][2].lightPos ) },
	{ "TerrainObjectsLightingAfternoonAmbient3",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_AFTERNOON ][2].ambient ) },
	{ "TerrainObjectsLightingAfternoonDiffuse3",		INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_AFTERNOON ][2].diffuse ) },
	{ "TerrainObjectsLightingAfternoonLightPos3",	INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_AFTERNOON ][2].lightPos ) },
	{ "TerrainObjectsLightingEveningAmbient3",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_EVENING ][2].ambient ) },
	{ "TerrainObjectsLightingEveningDiffuse3",			INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_EVENING ][2].diffuse ) },
	{ "TerrainObjectsLightingEveningLightPos3",		INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_EVENING ][2].lightPos ) },
	{ "TerrainObjectsLightingNightAmbient3",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_NIGHT ][2].ambient ) },
	{ "TerrainObjectsLightingNightDiffuse3",				INI::parseRGBColor,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_NIGHT ][2].diffuse ) },
	{ "TerrainObjectsLightingNightLightPos3",			INI::parseCoord3D,			nullptr,			offsetof( GlobalData, m_terrainObjectsLighting[ TIME_OF_DAY_NIGHT ][2].lightPos ) },
/* These are internal use only, they do not need file definitions
	{ "TerrainAmbientRGB",				INI::parseRGBColor,		nullptr,			offsetof( GlobalData, m_terrainAmbient ) },
	{ "TerrainDiffuseRGB",				INI::parseRGBColor,		nullptr,			offsetof( GlobalData, m_terrainDiffuse ) },
	{ "TerrainLightPos",					INI::parseCoord3D,		nullptr,			offsetof( GlobalData, m_terrainLightPos ) },
*/
	{ nullptr,					nullptr,						nullptr,						0 }
};

static void parseGameData_Lighting_Definition( INI* ini )
{
	// parse the ini weapon definition
	ini->initFromINI_allowUnknown( TheWritableGlobalData, s_GlobalData_Lighting_FieldParseTable );
}

static const BlockParse GlobalData_Lighting_TypeTable[] =
{
	{ "GameData",												parseGameData_Lighting_Definition },
	{ nullptr,													nullptr },
};
// ---------------


/////////////////////////////////////////////////////////////////////////////
/// GlobalLightOptions dialog trivial constructor - Create does the real work.


GlobalLightOptions::GlobalLightOptions(CWnd* pParent /*=nullptr*/)
	: CDialog(GlobalLightOptions::IDD, pParent)
{
	//{{AFX_DATA_INIT(GlobalLightOptions)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void GlobalLightOptions::onMapChangeStart()
{
	// Reset any light settings specific to the previously open map.
	// Could argue this is only needed for 'onNewMapStart', as opposed to loading an existing map, since an existing
	// map will overwrite this with its saved lighting values anyway.
	_OnResetLights();
}

void GlobalLightOptions::onMapChangeEnd()
{
	if (this->IsWindowVisible())
	{
		// Keep the existing dialog in-sync with the new/loaded map
		updateFields();
	}
}

void GlobalLightOptions::onTimeOfDayChanged()
{
	if (this->IsWindowVisible())
	{
		updateFields();
	}
}

/// Windows default stuff.
void GlobalLightOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(GlobalLightOptions)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


//MODDD - never used from as-is
/*
static void calcNewLight(Int lr, Int fb, Vector3 *newLight)
{
	newLight->Set(0,0,-1);
	Real yAngle = PI*(lr-90)/180;
	Real xAngle = PI*(fb-90)/180;
	Real zAngle = xAngle * WWMath::Sin(yAngle);
	xAngle *= WWMath::Cos(yAngle);
	newLight->Rotate_Y(yAngle);
	newLight->Rotate_X(xAngle);
	newLight->Rotate_Z(zAngle);
}
*/

//MODDD - helper to keep all fields in-sync with some important external change such as changing the time of day while
// the dialog is open. Also, any occurrences of the first 3 calls will use this helper instead.
void GlobalLightOptions::updateFields()
{
	determineHorizontalCoords(K_SUN);
	determineHorizontalCoords(K_ACCENT1);
	determineHorizontalCoords(K_ACCENT2);
	
	m_updating = true;

	stuffValuesIntoFields(K_SUN);
	stuffValuesIntoFields(K_ACCENT1);
	stuffValuesIntoFields(K_ACCENT2);

	m_updating = false;

	updateTimeOfDayDisplay();

	showLightFeedback(K_SUN);
	showLightFeedback(K_ACCENT1);
	showLightFeedback(K_ACCENT2);
}

//MODDD - commonly used script
const GlobalData::TerrainLighting* GlobalLightOptions::getTerrainLighting(Int lightIndex)
{
	if (m_lighting == K_OBJECTS || m_lighting == K_BOTH)
	{
		return &TheGlobalData->m_terrainObjectsLighting[TheGlobalData->m_timeOfDay][lightIndex];
	}
	else
	{
		return &TheGlobalData->m_terrainLighting[TheGlobalData->m_timeOfDay][lightIndex];
	}
}

//MODDD - same logic as above but returns a copy by value of the 'TerrainLighting' instead of a pointer to it.
// That is, changes to what's returned do not affect the original.
// However, anywhere that referred to this as-is happened to call 'pView->setLighting(&tl...' which then applies the copy
// back to the current lighting info in 'TheWritableGlobalData->m_terrain...'.
GlobalData::TerrainLighting GlobalLightOptions::getTerrainLightingCopy(Int lightIndex)
{
	if (m_lighting == K_OBJECTS || m_lighting == K_BOTH)
	{
		return TheGlobalData->m_terrainObjectsLighting[TheGlobalData->m_timeOfDay][lightIndex];
	}
	else
	{
		return TheGlobalData->m_terrainLighting[TheGlobalData->m_timeOfDay][lightIndex];
	}
}

//MODDD
void GlobalLightOptions::horizontalCoordsToPosition(Vector3* out_lightPos, Int lightIndex)
{
	out_lightPos->X = sin(PI/2.0f+m_angleElevation[lightIndex]/180.0f*PI)*cos(m_angleAzimuth[lightIndex]/180.0f*PI);// -WWMath::Sin(PI*(m_angleLR[lightIndex]-90)/180);
	out_lightPos->Y = sin(PI/2.0f+m_angleElevation[lightIndex]/180.0f*PI)*sin(m_angleAzimuth[lightIndex]/180.0f*PI);//-WWMath::Sin(PI*(m_angleFB[lightIndex]-90)/180);
	out_lightPos->Z = cos (PI/2.0f+m_angleElevation[lightIndex]/180.0f*PI);
}

//MODDD - helper to keep the ambient color button up-to-date with smaller changes such as editing any of the ambient color's R/G/B fields
void GlobalLightOptions::updateColorButton()
{
	Int lightIndex = K_SUN;
	const GlobalData::TerrainLighting *tl = getTerrainLighting(lightIndex);
	m_colorButton.setColor(tl->ambient);
}

//MODDD
void GlobalLightOptions::updateColorFields(Int lightIndex)
{
	const GlobalData::TerrainLighting *tl = getTerrainLighting(lightIndex);

	switch (lightIndex)
	{
		case K_SUN:
		default:
			PutInt(IDC_RA_EDIT, PercentToComponent(tl->ambient.red));
			PutInt(IDC_GA_EDIT, PercentToComponent(tl->ambient.green));
			PutInt(IDC_BA_EDIT, PercentToComponent(tl->ambient.blue));

			PutInt(IDC_RD_EDIT, PercentToComponent(tl->diffuse.red));
			PutInt(IDC_GD_EDIT, PercentToComponent(tl->diffuse.green));
			PutInt(IDC_BD_EDIT, PercentToComponent(tl->diffuse.blue));
			m_colorButton.setColor(tl->ambient);
			break;

		case K_ACCENT1:
			PutInt(IDC_RD_EDIT1, PercentToComponent(tl->diffuse.red));
			PutInt(IDC_GD_EDIT1, PercentToComponent(tl->diffuse.green));
			PutInt(IDC_BD_EDIT1, PercentToComponent(tl->diffuse.blue));
			break;

		case K_ACCENT2:
			PutInt(IDC_RD_EDIT2, PercentToComponent(tl->diffuse.red));
			PutInt(IDC_GD_EDIT2, PercentToComponent(tl->diffuse.green));
			PutInt(IDC_BD_EDIT2, PercentToComponent(tl->diffuse.blue));
			break;
	}
}

//MODDD - renamed from 'updateEditFields' to 'updateHorizontalCoordFields' for clarity
// Also, param 'lightIndex'
void GlobalLightOptions::updateHorizontalCoordFields(Int lightIndex)
{
	//m_updating = true;
	CString str;
	CWnd *pEdit;

	if (lightIndex == K_SUN)
	{
		str.Format("%d",m_angleAzimuth[K_SUN]);
		pEdit = GetDlgItem(IDC_FB_EDIT);
		if (pEdit) pEdit->SetWindowText(str);
		str.Format("%d",m_angleElevation[K_SUN]);
		pEdit = GetDlgItem(IDC_LR_EDIT);
		if (pEdit) pEdit->SetWindowText(str);
	}

	if (lightIndex == K_ACCENT1)
	{
		str.Format("%d",m_angleAzimuth[K_ACCENT1]);
		pEdit = GetDlgItem(IDC_FB_EDIT1);
		if (pEdit) pEdit->SetWindowText(str);
		str.Format("%d",m_angleElevation[K_ACCENT1]);
		pEdit = GetDlgItem(IDC_LR_EDIT1);
		if (pEdit) pEdit->SetWindowText(str);
	}

	if (lightIndex == K_ACCENT2)
	{
		str.Format("%d",m_angleAzimuth[K_ACCENT2]);
		pEdit = GetDlgItem(IDC_FB_EDIT2);
		if (pEdit) pEdit->SetWindowText(str);
		str.Format("%d",m_angleElevation[K_ACCENT2]);
		pEdit = GetDlgItem(IDC_LR_EDIT2);
		if (pEdit) pEdit->SetWindowText(str);
	}

	//m_updating = false;
}


//MODDD
void GlobalLightOptions::determineHorizontalCoords(Int lightIndex)
{
	const GlobalData::TerrainLighting *tl = getTerrainLighting(lightIndex);

	Vector3 light(tl->lightPos.x, tl->lightPos.y, tl->lightPos.z);
	light.Normalize();

	Real angleAzimuth = atan2(light.Y,light.X);//WWMath::Asin(light.Y);
	Real azimuth = angleAzimuth*180.0f/PI;//90-(angleFB/PI)*180;
	if (azimuth < 0) {
		azimuth += 360;
	}
	Real angleElevation = acos(light.Z);//WWMath::Asin(light.X);
	Real elevation = (angleElevation-PI/2.0f)*180.0f/PI;//90-(angleLR/PI)*180;

	m_angleElevation[lightIndex] = elevation;
	m_angleAzimuth[lightIndex] = azimuth;
}

//MODDD
void GlobalLightOptions::updateTimeOfDayDisplay()
{
	CWnd *pWnd;
	pWnd = GetDlgItem(IDC_TIME_OF_DAY_CAPTION);
	if (pWnd) {
		switch (TheGlobalData->m_timeOfDay) {
			default:
			case TIME_OF_DAY_MORNING: pWnd->SetWindowText("Time of day: Morning."); break;
			case TIME_OF_DAY_AFTERNOON: pWnd->SetWindowText("Time of day: Afternoon."); break;
			case TIME_OF_DAY_EVENING: pWnd->SetWindowText("Time of day: Evening."); break;
			case TIME_OF_DAY_NIGHT: pWnd->SetWindowText("Time of day: Night."); break;
		}
	}
}

//MODDD
void GlobalLightOptions::updateLightPositionDisplay(Vector3* lightPos)
{
	CWnd *pWnd = this->GetDlgItem(IDC_XYZ_STATIC);
	if (pWnd)
	{
		CString str;
		str.Format("XYZ: %.2f, %.2f, %.2f", lightPos->X, lightPos->Y, lightPos->Z);
		pWnd->SetWindowText(str);
	}
}

void GlobalLightOptions::showLightFeedback(Int lightIndex)
{
	//MODDD - condensed script into a helper
	Vector3 light;
	horizontalCoordsToPosition(&light, lightIndex);

	WbView3d * pView = CWorldBuilderDoc::GetActive3DView();
	if (pView) {
		Coord3D lightRay;
		lightRay.x=light.X;lightRay.y=light.Y;lightRay.z=light.Z;
		pView->doLightFeedback(true,lightRay,lightIndex);
	}
}

void GlobalLightOptions::applyAngle(Int lightIndex)
{
	//MODDD - condensed script into a helper
	Vector3 light;
	horizontalCoordsToPosition(&light, lightIndex);

	//MODDD - script condensed
	updateLightPositionDisplay(&light);

	GlobalData::TerrainLighting tl = getTerrainLightingCopy(lightIndex);
	tl.lightPos.x = light.X;
	tl.lightPos.y = light.Y;
	tl.lightPos.z = light.Z;

	WbView3d * pView = CWorldBuilderDoc::GetActive3DView();
	if (pView) {
		pView->setLighting(&tl, m_lighting, lightIndex);
		Coord3D lightRay;
		lightRay.x=light.X;lightRay.y=light.Y;lightRay.z=light.Z;
		pView->doLightFeedback(true,lightRay,lightIndex);
	}
}

static void SpitLights()
{
#ifdef DEBUG_LOGGING
	CString lightstrings[100];
	DEBUG_LOG(("GlobalLighting\n"));
	Int redA, greenA, blueA;
	Int redD, greenD, blueD;
	Real x, y, z;
	CString times[4];
	CString lights[3];
	times[0] = "Morning";
	times[1] = "Afternoon";
	times[2] = "Evening";
	times[3] = "Night";

	lights[0] = "";
	lights[1] = "2";
	lights[2] = "3";

	Int time=0;
	for (; time<4; time++) {
		for (Int light=0; light<3; light++) {
			redA = TheGlobalData->m_terrainLighting[time+TIME_OF_DAY_FIRST][light].ambient.red*255;
			greenA = TheGlobalData->m_terrainLighting[time+TIME_OF_DAY_FIRST][light].ambient.green*255;
			blueA = TheGlobalData->m_terrainLighting[time+TIME_OF_DAY_FIRST][light].ambient.blue*255;

			redD = TheGlobalData->m_terrainLighting[time+TIME_OF_DAY_FIRST][light].diffuse.red*255;
			greenD = TheGlobalData->m_terrainLighting[time+TIME_OF_DAY_FIRST][light].diffuse.green*255;
			blueD = TheGlobalData->m_terrainLighting[time+TIME_OF_DAY_FIRST][light].diffuse.blue*255;

			x = TheGlobalData->m_terrainLighting[time+TIME_OF_DAY_FIRST][light].lightPos.x;
			y = TheGlobalData->m_terrainLighting[time+TIME_OF_DAY_FIRST][light].lightPos.y;
			z = TheGlobalData->m_terrainLighting[time+TIME_OF_DAY_FIRST][light].lightPos.z;

			DEBUG_LOG(("TerrainLighting%sAmbient%s = R:%d G:%d B:%d", times[time], lights[light], redA, greenA, blueA));
			DEBUG_LOG(("TerrainLighting%sDiffuse%s = R:%d G:%d B:%d", times[time], lights[light], redD, greenD, blueD));
			DEBUG_LOG(("TerrainLighting%sLightPos%s = X:%0.2f Y:%0.2f Z:%0.2f", times[time], lights[light], x, y, z));

			redA = TheGlobalData->m_terrainObjectsLighting[time+TIME_OF_DAY_FIRST][light].ambient.red*255;
			greenA = TheGlobalData->m_terrainObjectsLighting[time+TIME_OF_DAY_FIRST][light].ambient.green*255;
			blueA = TheGlobalData->m_terrainObjectsLighting[time+TIME_OF_DAY_FIRST][light].ambient.blue*255;

			redD = TheGlobalData->m_terrainObjectsLighting[time+TIME_OF_DAY_FIRST][light].diffuse.red*255;
			greenD = TheGlobalData->m_terrainObjectsLighting[time+TIME_OF_DAY_FIRST][light].diffuse.green*255;
			blueD = TheGlobalData->m_terrainObjectsLighting[time+TIME_OF_DAY_FIRST][light].diffuse.blue*255;

			x = TheGlobalData->m_terrainObjectsLighting[time+TIME_OF_DAY_FIRST][light].lightPos.x;
			y = TheGlobalData->m_terrainObjectsLighting[time+TIME_OF_DAY_FIRST][light].lightPos.y;
			z = TheGlobalData->m_terrainObjectsLighting[time+TIME_OF_DAY_FIRST][light].lightPos.z;

			DEBUG_LOG(("TerrainObjectsLighting%sAmbient%s = R:%d G:%d B:%d", times[time], lights[light], redA, greenA, blueA));
			DEBUG_LOG(("TerrainObjectsLighting%sDiffuse%s = R:%d G:%d B:%d", times[time], lights[light], redD, greenD, blueD));
			DEBUG_LOG(("TerrainObjectsLighting%sLightPos%s = X:%0.2f Y:%0.2f Z:%0.2f", times[time], lights[light], x, y, z));

			DEBUG_LOG_RAW(("\n"));
		}
		DEBUG_LOG_RAW(("\n"));
	}

	DEBUG_LOG(("GlobalLighting Code\n"));
	for (time=0; time<4; time++) {
		for (Int light=0; light<3; light++) {
			Int theTime = time+TIME_OF_DAY_FIRST;
			GlobalData::TerrainLighting tl = TheGlobalData->m_terrainLighting[theTime][light];

			DEBUG_LOG(("TheGlobalData->m_terrainLighting[%d][%d].ambient.red = %0.4ff;", theTime, light, tl.ambient.red));
			DEBUG_LOG(("TheGlobalData->m_terrainLighting[%d][%d].ambient.green = %0.4ff;", theTime, light, tl.ambient.green));
			DEBUG_LOG(("TheGlobalData->m_terrainLighting[%d][%d].ambient.blue = %0.4ff;", theTime, light, tl.ambient.blue));

			DEBUG_LOG(("TheGlobalData->m_terrainLighting[%d][%d].diffuse.red = %0.4ff;", theTime, light, tl.diffuse.red));
			DEBUG_LOG(("TheGlobalData->m_terrainLighting[%d][%d].diffuse.green = %0.4ff;", theTime, light, tl.diffuse.green));
			DEBUG_LOG(("TheGlobalData->m_terrainLighting[%d][%d].diffuse.blue = %0.4ff;", theTime, light, tl.diffuse.blue));

			DEBUG_LOG(("TheGlobalData->m_terrainLighting[%d][%d].lightPos.x = %0.4ff;", theTime, light, tl.lightPos.x));
			DEBUG_LOG(("TheGlobalData->m_terrainLighting[%d][%d].lightPos.y = %0.4ff;", theTime, light, tl.lightPos.y));
			DEBUG_LOG(("TheGlobalData->m_terrainLighting[%d][%d].lightPos.z = %0.4ff;", theTime, light, tl.lightPos.z));

			tl = TheGlobalData->m_terrainObjectsLighting[theTime][light];

			DEBUG_LOG(("TheGlobalData->m_terrainObjectsLighting[%d][%d].ambient.red = %0.4ff;", theTime, light, tl.ambient.red));
			DEBUG_LOG(("TheGlobalData->m_terrainObjectsLighting[%d][%d].ambient.green = %0.4ff;", theTime, light, tl.ambient.green));
			DEBUG_LOG(("TheGlobalData->m_terrainObjectsLighting[%d][%d].ambient.blue = %0.4ff;", theTime, light, tl.ambient.blue));

			DEBUG_LOG(("TheGlobalData->m_terrainObjectsLighting[%d][%d].diffuse.red = %0.4ff;", theTime, light, tl.diffuse.red));
			DEBUG_LOG(("TheGlobalData->m_terrainObjectsLighting[%d][%d].diffuse.green = %0.4ff;", theTime, light, tl.diffuse.green));
			DEBUG_LOG(("TheGlobalData->m_terrainObjectsLighting[%d][%d].diffuse.blue = %0.4ff;", theTime, light, tl.diffuse.blue));

			DEBUG_LOG(("TheGlobalData->m_terrainObjectsLighting[%d][%d].lightPos.x = %0.4ff;", theTime, light, tl.lightPos.x));
			DEBUG_LOG(("TheGlobalData->m_terrainObjectsLighting[%d][%d].lightPos.y = %0.4ff;", theTime, light, tl.lightPos.y));
			DEBUG_LOG(("TheGlobalData->m_terrainObjectsLighting[%d][%d].lightPos.z = %0.4ff;", theTime, light, tl.lightPos.z));

			DEBUG_LOG_RAW(("\n"));
		}
		DEBUG_LOG_RAW(("\n"));
	}
#endif
}

//MODDD - separate version to hold everything from the original 'OnResetLights' except the UI update.
void GlobalLightOptions::_OnResetLights()
{
	//MODDD
	// Re-load 'GameData.ini' for these instead of setting things back from these hardcoded values.
	// This is based off the 'initSubsystem' call in WorldBuilder.cpp (app startup).
	//initSubsystem(TheWritableGlobalData, new GlobalData(), "Data\\INI\\Default\\GameData", "Data\\INI\\GameData");
	// First, call this in case of any fields not specified by GameData.ini (this is the case as of retail)
	TheWritableGlobalData->resetLightingFields();
	TheSubsystemListRecord.loadSubsystemAgain(TheWritableGlobalData, "Data\\INI\\Default\\GameData", "Data\\INI\\GameData", nullptr, GlobalData_Lighting_TypeTable);

	WbView3d * pView = CWorldBuilderDoc::GetActive3DView();
	if (pView) {
		pView->setLighting(&TheGlobalData->m_terrainLighting[TheGlobalData->m_timeOfDay][K_SUN], K_TERRAIN, K_SUN);
		pView->setLighting(&TheGlobalData->m_terrainLighting[TheGlobalData->m_timeOfDay][K_ACCENT1], K_TERRAIN, K_ACCENT1);
		pView->setLighting(&TheGlobalData->m_terrainLighting[TheGlobalData->m_timeOfDay][K_ACCENT2], K_TERRAIN, K_ACCENT2);

		pView->setLighting(&TheGlobalData->m_terrainObjectsLighting[TheGlobalData->m_timeOfDay][K_SUN], K_OBJECTS, K_SUN);
		pView->setLighting(&TheGlobalData->m_terrainObjectsLighting[TheGlobalData->m_timeOfDay][K_ACCENT1], K_OBJECTS, K_ACCENT1);
		pView->setLighting(&TheGlobalData->m_terrainObjectsLighting[TheGlobalData->m_timeOfDay][K_ACCENT2], K_OBJECTS, K_ACCENT2);
	}
}

void GlobalLightOptions::OnResetLights()
{
	//MODDD - most of the existing content moved to the underscore version
	_OnResetLights();

	updateFields();
}

/////////////////////////////////////////////////////////////////////////////
// GlobalLightOptions message handlers

/// Dialog UI initialization.
/** Creates the slider controls, and sets the initial values for
width and feather in the ui controls. */
BOOL GlobalLightOptions::OnInitDialog()
{
	CDialog::OnInitDialog();

	//MODDD
	m_updating = false;

	kUIRedIDs[0] = IDC_RD_EDIT;
	kUIRedIDs[1] = IDC_RD_EDIT1;
	kUIRedIDs[2] = IDC_RD_EDIT2;

	kUIGreenIDs[0] = IDC_GD_EDIT;
	kUIGreenIDs[1] = IDC_GD_EDIT1;
	kUIGreenIDs[2] = IDC_GD_EDIT2;

	kUIBlueIDs[0] = IDC_BD_EDIT;
	kUIBlueIDs[1] = IDC_BD_EDIT1;
	kUIBlueIDs[2] = IDC_BD_EDIT2;

	m_frontBackPopup.SetupPopSliderButton(this, IDC_FB_POPUP, this);
	m_leftRightPopup.SetupPopSliderButton(this, IDC_LR_POPUP, this);

	m_frontBackPopupAccent1.SetupPopSliderButton(this, IDC_FB_POPUP1, this);
	m_leftRightPopupAccent1.SetupPopSliderButton(this, IDC_LR_POPUP1, this);

	m_frontBackPopupAccent2.SetupPopSliderButton(this, IDC_FB_POPUP2, this);
	m_leftRightPopupAccent2.SetupPopSliderButton(this, IDC_LR_POPUP2, this);

	CButton *pButton = (CButton *)GetDlgItem(IDC_RADIO_EVERYTHING);
	pButton->SetCheck(1);
	m_lighting = K_BOTH;

	//MODDD - not needed this early. Showing the dialog will invoke this anyway.
	/*
	updateFields();
	*/

	CRect rect;
	CWnd *item = GetDlgItem(IDC_PSEd_Color1);
	if (item) {
		item->GetWindowRect(&rect);
		ScreenToClient(&rect);
		DWORD style = item->GetStyle();
		m_colorButton.Create("", style, rect, this, IDC_PSEd_Color1);
		item->DestroyWindow();
	}

	return TRUE;	// return TRUE unless you set the focus to a control
								// EXCEPTION: OCX Property Pages should return FALSE
}

/** Displays the current values in the fields. */
void GlobalLightOptions::stuffValuesIntoFields(Int lightIndex)
{
	const GlobalData::TerrainLighting *tl = getTerrainLighting(lightIndex);

	//MODDD - script moved to a new method 'determineHorizontalCoords', called elsewhere

	//MODDD - adding arg 'lightIndex'
	updateHorizontalCoordFields(lightIndex);
	//MODDD - existing script moved to here
	updateColorFields(lightIndex);

	//m_updating = true;

	//MODDD - script condensed
	if (lightIndex==K_SUN)
	{
		Vector3 light(tl->lightPos.x, tl->lightPos.y, tl->lightPos.z);
		light.Normalize();
		updateLightPositionDisplay(&light);
	}

	//MODDD - script moved to new method 'updateColorFields', called above

	//m_updating = false;
	
	//MODDD - script moved to a new method 'updateTimeOfDayDisplay'

	//MODDD - call moved elsewhere
	//showLightFeedback(lightIndex);
}


//MODDD - implementation from .h
Real GlobalLightOptions::ComponentToPercent(Int component)
{
	Real percent;
	if (component >= 255) {
		return 1.0;
	}
	if (component <= 0) {
		return 0.0;
	}
	percent = (Real)component/255.0;
	return percent;
}

//MODDD - implementation from .h
Int GlobalLightOptions::PercentToComponent(Real percent)
{
	Int component;
	if (percent >= 1.0) {
		return 255;
	}
	if (percent <= 0.0) {
		return 0;
	}
	component = (percent * 255.0);
	return component;
}

/** Gets the new edit control text, converts it to an int, then updates
		the slider and brush tool. */

BOOL GlobalLightOptions::GetInt(Int ctrlID, Int *rVal)
{
	CWnd *pEdit = GetDlgItem(ctrlID);
	char buffer[_MAX_PATH];
	if (pEdit) {
		pEdit->GetWindowText(buffer, sizeof(buffer));
		Int val;
		if (1==sscanf(buffer, "%d", &val)) {
			*rVal = val;
			return true;
		}
	}
	return false;
}

void GlobalLightOptions::PutInt(Int ctrlID, Int val)
{
	CWnd *pEdit = GetDlgItem(ctrlID);
	if (pEdit) {
		CString str;
		str.Format("%d", val);
		pEdit->SetWindowText(str);
	}
}

//MODDD - split
// ---
/*
void GlobalLightOptions::OnChangeFrontBackEdit()
{
	if (m_updating) return;
	GetInt(IDC_FB_EDIT, &m_angleAzimuth[K_SUN]);
	GetInt(IDC_FB_EDIT1, &m_angleAzimuth[K_ACCENT1]);
	GetInt(IDC_FB_EDIT2, &m_angleAzimuth[K_ACCENT2]);
	m_updating = true;
	applyAngle(K_SUN);
	applyAngle(K_ACCENT1);
	applyAngle(K_ACCENT2);
	m_updating = false;
}
*/
void GlobalLightOptions::OnChangeSunFrontBackEdit()
{
	if (m_updating) return;
	GetInt(IDC_FB_EDIT, &m_angleAzimuth[K_SUN]);
	//MODDD - these 'applyAngle' calls don't need 'm_updating' guards - removed
	applyAngle(K_SUN);
}
void GlobalLightOptions::OnChangeAccent1FrontBackEdit()
{
	if (m_updating) return;
	GetInt(IDC_FB_EDIT1, &m_angleAzimuth[K_ACCENT1]);
	applyAngle(K_ACCENT1);
}
void GlobalLightOptions::OnChangeAccent2FrontBackEdit()
{
	if (m_updating) return;
	GetInt(IDC_FB_EDIT2, &m_angleAzimuth[K_ACCENT2]);
	applyAngle(K_ACCENT2);
}
// ---

/// Handles width edit ui messages.
/** Gets the new edit control text, converts it to an int, then updates
		the angles. */
//MODDD - split
// ---
/*
void GlobalLightOptions::OnChangeLeftRightEdit()
{
	if (m_updating){
		return;
	}
	GetInt(IDC_LR_EDIT, &m_angleElevation[K_SUN]);
	GetInt(IDC_LR_EDIT1, &m_angleElevation[K_ACCENT1]);
	GetInt(IDC_LR_EDIT2, &m_angleElevation[K_ACCENT2]);
	m_updating = true;
	applyAngle(K_SUN);
	applyAngle(K_ACCENT1);
	applyAngle(K_ACCENT2);
	m_updating = false;
}
*/
// ---
void GlobalLightOptions::OnChangeSunLeftRightEdit()
{
	if (m_updating){
		return;
	}
	GetInt(IDC_LR_EDIT, &m_angleElevation[K_SUN]);
	//MODDD - these 'applyAngle' calls don't need 'm_updating' guards - removed
	applyAngle(K_SUN);
}
void GlobalLightOptions::OnChangeAccent1LeftRightEdit()
{
	if (m_updating){
		return;
	}
	GetInt(IDC_LR_EDIT1, &m_angleElevation[K_ACCENT1]);
	applyAngle(K_ACCENT1);
}
void GlobalLightOptions::OnChangeAccent2LeftRightEdit()
{
	if (m_updating){
		return;
	}
	GetInt(IDC_LR_EDIT2, &m_angleElevation[K_ACCENT2]);
	applyAngle(K_ACCENT2);
}
// ---

/// Handles width edit ui messages.
/** Gets the new edit control text, converts it to an int, then updates
		the slider and brush tool. */
void GlobalLightOptions::applyColor(Int lightIndex)
{
	Int clr;
	GlobalData::TerrainLighting tl = getTerrainLightingCopy(lightIndex);
	if (lightIndex == K_SUN) {
		tl.ambient.red		= TheGlobalData->m_terrainAmbient[K_SUN].red;
		tl.ambient.green	= TheGlobalData->m_terrainAmbient[K_SUN].green;
		tl.ambient.blue		= TheGlobalData->m_terrainAmbient[K_SUN].blue;

		if (GetInt(IDC_RA_EDIT, &clr))
			tl.ambient.red = ComponentToPercent(clr);
		if (GetInt(IDC_GA_EDIT, &clr))
			tl.ambient.green = ComponentToPercent(clr);
		if (GetInt(IDC_BA_EDIT, &clr))
			tl.ambient.blue = ComponentToPercent(clr);
	} else {
		tl.ambient.red		= 0;
		tl.ambient.green	= 0;
		tl.ambient.blue		= 0;
	}

	tl.diffuse.red		= TheGlobalData->m_terrainDiffuse[lightIndex].red;
	tl.diffuse.green	= TheGlobalData->m_terrainDiffuse[lightIndex].green;
	tl.diffuse.blue		= TheGlobalData->m_terrainDiffuse[lightIndex].blue;

	if (GetInt(kUIRedIDs[lightIndex], &clr))
		tl.diffuse.red = ComponentToPercent(clr);
	if (GetInt(kUIGreenIDs[lightIndex], &clr))
		tl.diffuse.green = ComponentToPercent(clr);
	if (GetInt(kUIBlueIDs[lightIndex], &clr))
		tl.diffuse.blue = ComponentToPercent(clr);
	WbView3d * pView = CWorldBuilderDoc::GetActive3DView();
	if (pView) {
		pView->setLighting(&tl, m_lighting, lightIndex);
	}
}

//MODDD - split
// ---
/*
void GlobalLightOptions::OnChangeColorEdit()
{
	if (m_updating) return;
	applyColor(K_SUN);
	applyColor(K_ACCENT1);
	applyColor(K_ACCENT2);
}
*/
// ---
void GlobalLightOptions::OnChangeAmbientColorEdit()
{
	if (m_updating) return;
	applyColor(K_SUN);
	updateColorButton();
}

void GlobalLightOptions::OnChangeSunColorEdit()
{
	if (m_updating) return;
	applyColor(K_SUN);
}

void GlobalLightOptions::OnChangeAccent1ColorEdit()
{
	if (m_updating) return;
	applyColor(K_ACCENT1);
}

void GlobalLightOptions::OnChangeAccent2ColorEdit()
{
	if (m_updating) return;
	applyColor(K_ACCENT2);
}
// ---

void GlobalLightOptions::GetPopSliderInfo(const long sliderID, long *pMin, long *pMax, long *pLineSize, long *pInitial)
{
	switch (sliderID) {

		case IDC_FB_POPUP:
			*pMin = 0;
			*pMax = 359;
			*pInitial = m_angleAzimuth[K_SUN];
			*pLineSize = 1;
			break;
		case IDC_FB_POPUP1:
			*pMin = 0;
			*pMax = 359;
			*pInitial = m_angleAzimuth[K_ACCENT1];
			*pLineSize = 1;
			break;
		case IDC_FB_POPUP2:
			*pMin = 0;
			*pMax = 359;
			*pInitial = m_angleAzimuth[K_ACCENT2];
			*pLineSize = 1;
			break;

		case IDC_LR_POPUP:
			*pMin = 0;
			*pMax = 90;
			*pInitial = m_angleElevation[K_SUN];
			*pLineSize = 1;
			break;
		case IDC_LR_POPUP1:
			*pMin = 0;
			*pMax = 90;
			*pInitial = m_angleElevation[K_ACCENT1];
			*pLineSize = 1;
			break;
		case IDC_LR_POPUP2:
			*pMin = 0;
			*pMax = 90;
			*pInitial = m_angleElevation[K_ACCENT2];
			*pLineSize = 1;
			break;

		default:
			// uh-oh!
			DEBUG_CRASH(("Slider message from unknown control"));
			break;
	}
}

void GlobalLightOptions::PopSliderChanged(const long sliderID, long theVal)
{
	//MODDD - replacement
	// Changing the editable text field values already triggers the field edit event (ex: 'OnChangeSunFrontBackEdit'),
	// so no need to do anything else here.
	// Removing the 'm_angleAzimuth' / 'm_angleElevation' assignments and 'applyAngle' calls
	switch (sliderID) {

		case IDC_FB_POPUP:
			PutInt(IDC_FB_EDIT, theVal);
			break;

		case IDC_FB_POPUP1:
			PutInt(IDC_FB_EDIT1, theVal);
			break;

		case IDC_FB_POPUP2:
			PutInt(IDC_FB_EDIT2, theVal);
			break;

		case IDC_LR_POPUP:
			PutInt(IDC_LR_EDIT, theVal);
			break;

		case IDC_LR_POPUP1:
			PutInt(IDC_LR_EDIT1, theVal);
			break;

		case IDC_LR_POPUP2:
			PutInt(IDC_LR_EDIT2, theVal);
			break;

		default:
			// uh-oh!
			DEBUG_CRASH(("Slider message from unknown control"));
			break;
	}
}

void GlobalLightOptions::PopSliderFinished(const long sliderID, long theVal)
{
	switch (sliderID) {
		case IDC_FB_POPUP:
			break;
		case IDC_LR_POPUP:
			break;
		case IDC_FB_POPUP1:
			break;
		case IDC_LR_POPUP1:
			break;
		case IDC_FB_POPUP2:
			break;
		case IDC_LR_POPUP2:
			break;

		default:
			// uh-oh!
			DEBUG_CRASH(("Slider message from unknown control"));
			break;
	}

}


BEGIN_MESSAGE_MAP(GlobalLightOptions, CDialog)
	//{{AFX_MSG_MAP(GlobalLightOptions)
	ON_WM_MOVE()
	ON_WM_SHOWWINDOW()
	ON_WM_CLOSE()
	//MODDD - split the 'OnChangeColorEdit' event into more specific ones
	ON_EN_CHANGE(IDC_RA_EDIT, OnChangeAmbientColorEdit)
	ON_EN_CHANGE(IDC_BA_EDIT, OnChangeAmbientColorEdit)
	ON_EN_CHANGE(IDC_GA_EDIT, OnChangeAmbientColorEdit)
	ON_EN_CHANGE(IDC_FB_EDIT, OnChangeSunFrontBackEdit)
	ON_EN_CHANGE(IDC_FB_EDIT1, OnChangeAccent1FrontBackEdit)
	ON_EN_CHANGE(IDC_FB_EDIT2, OnChangeAccent2FrontBackEdit)
	ON_EN_CHANGE(IDC_LR_EDIT, OnChangeSunLeftRightEdit)
	ON_EN_CHANGE(IDC_LR_EDIT1, OnChangeAccent1LeftRightEdit)
	ON_EN_CHANGE(IDC_LR_EDIT2, OnChangeAccent2LeftRightEdit)
	ON_EN_CHANGE(IDC_RD_EDIT, OnChangeSunColorEdit)
	ON_EN_CHANGE(IDC_GD_EDIT, OnChangeSunColorEdit)
	ON_EN_CHANGE(IDC_BD_EDIT, OnChangeSunColorEdit)
	ON_EN_CHANGE(IDC_RD_EDIT1, OnChangeAccent1ColorEdit)
	ON_EN_CHANGE(IDC_GD_EDIT1, OnChangeAccent1ColorEdit)
	ON_EN_CHANGE(IDC_BD_EDIT1, OnChangeAccent1ColorEdit)
	ON_EN_CHANGE(IDC_RD_EDIT2, OnChangeAccent2ColorEdit)
	ON_EN_CHANGE(IDC_GD_EDIT2, OnChangeAccent2ColorEdit)
	ON_EN_CHANGE(IDC_BD_EDIT2, OnChangeAccent2ColorEdit)
	ON_BN_CLICKED(IDC_RADIO_EVERYTHING, OnRadioEverything)
	ON_BN_CLICKED(IDC_RADIO_OBJECTS, OnRadioObjects)
	ON_BN_CLICKED(IDC_RADIO_TERRAIN, OnRadioTerrain)
	ON_BN_CLICKED(IDC_PSEd_Color1, OnColorPress)
	ON_BN_CLICKED(IDC_GlobalLightingReset, OnResetLights)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



void GlobalLightOptions::OnRadioEverything()
{
	m_lighting = K_BOTH;
	updateFields();
}

void GlobalLightOptions::OnRadioObjects()
{
	m_lighting = K_OBJECTS;
	updateFields();
}

void GlobalLightOptions::OnRadioTerrain()
{
	m_lighting = K_TERRAIN;
	updateFields();
}

void GlobalLightOptions::OnColorPress()
{
	//MODDD - start with the color dialog set to the current color
	//CColorDialog dlg;
	CColorDialog dlg(CButtonShowColor::RGBtoBGR(m_colorButton.getColor().getAsInt()), CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT|CC_SOLIDCOLOR);
	if (dlg.DoModal() == IDOK) {
		m_colorButton.setColor(CButtonShowColor::BGRtoRGB(dlg.GetColor()));
		RGBColor color = m_colorButton.getColor();
		//MODDD - guard with 'm_updating' so each R-G-B field edit doesn't trigger an update.
		// May as well wait until all 3 values are entered
		m_updating = true;
		PutInt(IDC_RA_EDIT, PercentToComponent(color.red));
		PutInt(IDC_GA_EDIT, PercentToComponent(color.green));
		PutInt(IDC_BA_EDIT, PercentToComponent(color.blue));
		m_updating = false;
		applyColor(K_SUN);
	}
}

void GlobalLightOptions::OnClose()
{
	ShowWindow(SW_HIDE);

	WbView3d * pView = CWorldBuilderDoc::GetActive3DView();
	if (pView) {
		Coord3D lightRay;
		lightRay.x=0.0f;lightRay.y=0.0f;lightRay.z=-1.0f;	//default light above terrain.
		pView->doLightFeedback(false,lightRay,0);	//turn off the light direction indicator
	}
}

void GlobalLightOptions::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);

	//MODDD - surrounding this with a 'bShow' condition. Why keep the fields up to date on being closed?
	// I think doing it on both open & close is a mistake.
	if (bShow)
	{
		updateFields();
	}

	if (!bShow) {
		WbView3d * pView = CWorldBuilderDoc::GetActive3DView();
		if (pView) {
			Coord3D lightRay;
			lightRay.x=0.0f;lightRay.y=0.0f;lightRay.z=-1.0f;	//default light above terrain.
			pView->doLightFeedback(false,lightRay,0);	//turn off the light direction indicator
			pView->doLightFeedback(false,lightRay,1);	//turn off the light direction indicator
			pView->doLightFeedback(false,lightRay,2);	//turn off the light direction indicator
		}
	}
#if 0
	SpitLights();
#endif
}

void GlobalLightOptions::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);

	if (this->IsWindowVisible() && !this->IsIconic()) {
		CRect frameRect;
		GetWindowRect(&frameRect);
		::AfxGetApp()->WriteProfileInt(GLOBALLIGHT_OPTIONS_PANEL_SECTION, "Top", frameRect.top);
		::AfxGetApp()->WriteProfileInt(GLOBALLIGHT_OPTIONS_PANEL_SECTION, "Left", frameRect.left);
	}

}
