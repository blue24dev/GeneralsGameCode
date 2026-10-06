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

// playerlistdlg.cpp : implementation file
//

#include "StdAfx.h"
#include "WorldBuilder.h"
#include "playerlistdlg.h"
#include "mapobjectprops.h"
#include "WorldBuilderDoc.h"
#include "CUndoable.h"
#include "addplayerdialog.h"
#include "Common/WellKnownKeys.h"
#include "Common/PlayerTemplate.h"
#include "Common/MultiplayerSettings.h"
#include "GameLogic/SidesList.h"
#include "GameClient/GameText.h"
#include "Common/UnicodeString.h"
//MODDD
#include "GameClient/ChallengeGenerals.h"

static const char* NEUTRAL_NAME_STR = "(neutral)";

static Int thePrevCurPlyr = 0;

static Bool islegalplayernamechar(char c)
{
	// note, spaces are NOT allowed.
	return ::isalnum(c) || c == '_';
}

static void fixDefaultTeamName(SidesList& sides, AsciiString oldpname, AsciiString newpname)
{
	AsciiString tname;
	tname.set("team");
	tname.concat(oldpname);
	TeamsInfo *ti = sides.findTeamInfo(tname);
	if (ti)
	{
		tname.set("team");
		tname.concat(newpname);
		DEBUG_LOG(("rename team %s -> %s",ti->getDict()->getAsciiString(TheKey_teamName).str(),tname.str()));
		ti->getDict()->setAsciiString(TheKey_teamName, tname);
		ti->getDict()->setAsciiString(TheKey_teamOwner, newpname);
	}
	else
	{
		DEBUG_CRASH(("team not found"));
	}
}

static void updateAllTeams(SidesList& sides, AsciiString oldpname, AsciiString newpname)
{
	Int numTeams = sides.getNumTeams();
	for (Int i = 0; i < numTeams; ++i) {
		TeamsInfo *teamInfo = sides.getTeamInfo(i);
		if (teamInfo) {
			Bool exists;
			Dict *dict = teamInfo->getDict();

			AsciiString teamOwner = dict->getAsciiString(TheKey_teamOwner, &exists);

			if (exists && teamOwner.compare(oldpname) == 0) {
				dict->setAsciiString(TheKey_teamOwner, newpname);
			}
		}
	}
}



static void ensureValidPlayerName(Dict *d)
{
	// ensure there are no illegal chars in it. (in particular, no spaces!)
	char buf[1024];
	strlcpy(buf, d->getAsciiString(TheKey_playerName).str(), ARRAY_SIZE(buf));
	for (char* p = buf; *p; ++p)
		if (!islegalplayernamechar(*p))
			*p = '_';
	d->setAsciiString(TheKey_playerName, AsciiString(buf));
}

static AsciiString playerNameForUI(SidesList& sides, int i)
{
	AsciiString b = sides.getSideInfo(i)->getDict()->getAsciiString(TheKey_playerName);
	if (b.isEmpty())
		b = NEUTRAL_NAME_STR;
	return b;
}

static AsciiString UIToInternal(SidesList& sides, const AsciiString& n)
{
	Int i;
	for (i = 0; i < sides.getNumSides(); i++)
	{
		if (playerNameForUI(sides, i) == n)
			return sides.getSideInfo(i)->getDict()->getAsciiString(TheKey_playerName);
	}

	DEBUG_CRASH(("ui name not found"));
	return AsciiString::TheEmptyString;
}

static Bool containsToken(const AsciiString& cur_allies, const AsciiString& tokenIn)
{
	AsciiString name, token;

	name = cur_allies;
	while (name.nextToken(&token))
	{
		if (token == tokenIn)
			return true;
	}
	return false;
}

static AsciiString removeDupsFromEnemies(const AsciiString& cur_allies, const AsciiString& cur_enemies)
{
	AsciiString new_enemies, tmp, token;

	tmp = cur_enemies;
	while (tmp.nextToken(&token))
	{
		if (containsToken(cur_allies, token))
			continue;
		if (!new_enemies.isEmpty())
			new_enemies.concat(" ");
		new_enemies.concat(token);
	}
	return new_enemies;
}

static AsciiString extractFromAlliesList(CListBox *alliesList, SidesList& sides)
{
	char buffer[1024];
	AsciiString allies;
	for (Int i = 0; i < alliesList->GetCount(); i++)
	{
		if (alliesList->GetSel(i) > 0)
		{
			alliesList->GetText(i, buffer);
			AsciiString nm(buffer);
			if (!allies.isEmpty())
				allies.concat(" ");
			allies.concat(UIToInternal(sides, nm));
		}
	}
//DEBUG_LOG(("a/e is (%s)",allies.str()));
	return allies;
}

static void buildAlliesList(CListBox *alliesList, SidesList& sides,	const AsciiString& omitPlayer)
{
	Int i;
	AsciiString name, token, oname;

	alliesList->ResetContent();
	for (i = 0; i < sides.getNumSides(); i++)
	{
		name = sides.getSideInfo(i)->getDict()->getAsciiString(TheKey_playerName);
		if (name == omitPlayer || name.isEmpty())
			continue;
		name = playerNameForUI(sides, i);
		alliesList->AddString(name.str());
	}
}

static void selectAlliesList(CListBox *alliesList, SidesList& sides, const AsciiString& cur_allies)
{
	Int oindex_in_list;
	AsciiString name, token, oname;

	if (extractFromAlliesList(alliesList, sides) == cur_allies)
		return;

	alliesList->SetSel(-1, false);
	name = cur_allies;
	while (name.nextToken(&token))
	{
		Int i;
		SidesInfo *si = sides.findSideInfo(token, &i);
		if (!si)
		{
			DEBUG_CRASH(("player %s not found",token.str()));
			continue;
		}
		// must re-find, since list is sorted
		oindex_in_list = alliesList->FindStringExact(-1, playerNameForUI(sides, i).str());
		if (oindex_in_list == -1)
		{
			DEBUG_CRASH(("hmm, should not happen"));
			continue;
		}
		alliesList->SetSel(oindex_in_list, true);
	}
}

// returns a str describing how t1 considers t2. (implies nothing about how t2 considers t1)
static const char* calcRelationStr(SidesList& sides, int t1, int t2)
{
	const char* allied = "Ally";
	const char* enemies = "Enemy";
	const char* neutral = "Neutral";

	SidesInfo* ti1;
	SidesInfo* ti2;
	AsciiString t2name;


	//	we use the relationship between our player's default teams.
	ti1 = sides.getSideInfo(t1);
	ti2 = sides.getSideInfo(t2);
	t2name = ti2->getDict()->getAsciiString(TheKey_playerName);
	if (containsToken(ti1->getDict()->getAsciiString(TheKey_playerAllies), t2name))
		return allied;
	else if (containsToken(ti1->getDict()->getAsciiString(TheKey_playerEnemies), t2name))
		return enemies;

	// no relation, so assume neutral
	return neutral;
}

/////////////////////////////////////////////////////////////////////////////
// PlayerListDlg dialog


PlayerListDlg::PlayerListDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(PlayerListDlg::IDD, pParent), m_updating(0)
{
	//{{AFX_DATA_INIT(PlayerListDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void PlayerListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//MODDD - alternate way to hook up subclassed UI items for more accuracy, particularly for the now-subclassed combobox
	DDX_Control(pDX, IDC_PlayerColor, m_colorButton);
	DDX_Control(pDX, IDC_PLAYERFACTION, m_factionComboBox);

	//{{AFX_DATA_MAP(PlayerListDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PlayerListDlg, CDialog)
	//{{AFX_MSG_MAP(PlayerListDlg)
	ON_BN_CLICKED(IDC_NEWPLAYER, OnNewplayer)
	ON_BN_CLICKED(IDC_EDITPLAYER, OnEditplayer)
	ON_BN_CLICKED(IDC_REMOVEPLAYER, OnRemoveplayer)
	ON_LBN_SELCHANGE(IDC_PLAYERS, OnSelchangePlayers)
	ON_LBN_DBLCLK(IDC_PLAYERS, OnDblclkPlayers)
	ON_LBN_SELCHANGE(IDC_ALLIESLIST, OnSelchangeAllieslist)
	ON_LBN_SELCHANGE(IDC_ENEMIESLIST, OnSelchangeEnemieslist)
	ON_BN_CLICKED(IDC_PLAYERISCOMPUTER, OnPlayeriscomputer)
	ON_CBN_SELENDOK(IDC_PLAYERFACTION, OnEditchangePlayerfaction)
	ON_BN_CLICKED(IDC_CHANGE_NAME, OnChangePlayername)
	ON_EN_CHANGE(IDC_PLAYERDISPLAYNAME, OnChangePlayerdisplayname)
	ON_BN_CLICKED(IDC_PlayerColor, OnColorPress)
	ON_CBN_SELENDOK(IDC_PlayerColorCombo, OnSelectPlayerColor)
	ON_BN_CLICKED(IDC_ADDSKIRMISHPLAYERS, OnAddskirmishplayers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

	//ON_EN_CHANGE(IDC_PLAYERNAME, OnChangePlayername)

/////////////////////////////////////////////////////////////////////////////
// PlayerListDlg message handlers

void PlayerListDlg::OnNewplayer()
{
	if (m_sides.getNumSides() >= MAX_PLAYER_COUNT - 1) ///Added -1 so we can always have an observer even for Single player games.
		return;

	AddPlayerDialog addPlyr("");
	if (addPlyr.DoModal() != IDOK)
		return;

	AsciiString addedPTName = addPlyr.getAddedSide();

	AsciiString pname;
	UnicodeString pnameu;
	Int num = 1;
	do {
		pname.format("player%04d",num);
		pnameu.format(L"Player %04d's Display Name",num);
		num++;
	} while (m_sides.findSideInfo(pname));

	Dict newPlayerDict;
	newPlayerDict.setAsciiString(TheKey_playerName, pname);
	newPlayerDict.setBool(TheKey_playerIsHuman, true);
	newPlayerDict.setUnicodeString(TheKey_playerDisplayName, pnameu);
	newPlayerDict.setAsciiString(TheKey_playerFaction, addedPTName);
	newPlayerDict.setAsciiString(TheKey_playerEnemies, "");
	newPlayerDict.setAsciiString(TheKey_playerAllies, "");

#ifdef NOT_IN_USE
	// auto-open the advanced prop editor
	MapObjectProps editor(&newPlayerDict, "Create New Player", this);
	if (editor.DoModal() == IDOK)
#endif
	{
		if (newPlayerDict.getAsciiString(TheKey_playerName).isEmpty())
		{
			// sorry, no more than one neutral
		}
		else
		{
			ensureValidPlayerName(&newPlayerDict);
			m_sides.addSide(&newPlayerDict);

			Bool modified = m_sides.validateSides();
			(void)modified;
			DEBUG_ASSERTLOG(!modified,("had to clean up sides in PlayerListDlg::OnNewplayer"));
			m_curPlayerIdx = m_sides.getNumSides()-1;
			updateTheUI();
		}
	}
}

void PlayerListDlg::OnEditplayer()
{
	// TODO: the dialog referenced here has no ok or cancel buttons, so locks the editor
	// re-enable this routine once it is not a guaranteed hang
	AfxMessageBox("Implement me. (Sorry.)");
	return;

#if 0
	Dict *playerDict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();
	AsciiString pnameold = playerDict->getAsciiString(TheKey_playerName);
	Bool isneutral = pnameold.isEmpty();
	if (isneutral)
		return;

	Dict playerDictCopy = *playerDict;
	MapObjectProps editor(&playerDictCopy, "Edit Player", this);
	if (editor.DoModal() == IDOK)
	{
		ensureValidPlayerName(&playerDictCopy);

		if (playerDict->getAsciiString(TheKey_playerName) != playerDictCopy.getAsciiString(TheKey_playerName))
		{
			AsciiString tname;
			tname.set("team");
			tname.concat(playerDict->getAsciiString(TheKey_playerName));
			Int count = MapObject::countMapObjectsWithOwner(tname);
			if (count > 0)
			{
				CString msg;
				msg.Format(IDS_RENAMING_INUSE_TEAM, count);
				if (::AfxMessageBox(msg, MB_YESNO) == IDNO)
					return;
			}
		}

		*m_sides.getSideInfo(m_curPlayerIdx)->getDict() = playerDictCopy;

		AsciiString pnamenew = playerDictCopy.getAsciiString(TheKey_playerName);
		fixDefaultTeamName(m_sides, pnameold, pnamenew);

		Bool modified = m_sides.validateSides();
		DEBUG_ASSERTLOG(!modified,("had to clean up sides in PlayerListDlg::OnEditplayer"));

		updateTheUI();
	}
#endif
}

void PlayerListDlg::OnRemoveplayer()
{
	Dict *playerDict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();
	AsciiString pname = playerDict->getAsciiString(TheKey_playerName);
	Bool isneutral = pname.isEmpty();
	if (isneutral)
		return;

	Int i;
	Int count = 0;
	for (i = 0; i < m_sides.getNumTeams(); i++)
	{
		Dict *tdict = m_sides.getTeamInfo(i)->getDict();
		if (tdict->getAsciiString(TheKey_teamOwner) == pname)
		{
			count += MapObject::countMapObjectsWithOwner(tdict->getAsciiString(TheKey_teamName));
		}
	}

	if (count > 0)
	{
		CString msg;
		msg.Format(IDS_REMOVING_INUSE_TEAM, count);
		if (::AfxMessageBox(msg, MB_YESNO) == IDNO)
			return;
	}

	if (m_sides.getNumSides() <= 1)
		return;

	m_sides.removeSide(m_curPlayerIdx);
try_again:
	for (i = 0; i < m_sides.getNumTeams(); i++)
	{
		Dict *tdict = m_sides.getTeamInfo(i)->getDict();
		if (tdict->getAsciiString(TheKey_teamOwner) == pname)
		{
			m_sides.removeTeam(i);
			goto try_again;
		}
	}

	Bool modified = m_sides.validateSides();
	(void)modified;
	DEBUG_ASSERTLOG(!modified,("had to clean up sides in PlayerListDlg::OnRemoveplayer"));
	updateTheUI();
}

void PlayerListDlg::OnSelchangePlayers()
{
	CListBox *list = (CListBox*)GetDlgItem(IDC_PLAYERS);
	m_curPlayerIdx = list->GetCurSel();
	updateTheUI();
}

//MODDD - VS6-friendly predicate for 'std::sort'
static bool compareStringCaseInsensitive(const CString& a, const CString& b)
{
	return a.CompareNoCase(b) < 0;
}

void PlayerListDlg::updateTheUI()
{
	char buffer[1024];

	if (m_updating)
		return;

	++m_updating;

	// make sure everything is canonical.
	Bool modified = m_sides.validateSides();
	(void)modified;
	DEBUG_ASSERTLOG(!modified,("had to clean up sides in PlayerListDlg::updateTheUI! (caller should do this)"));

	if (m_curPlayerIdx < 0) m_curPlayerIdx = 0;
	if (m_curPlayerIdx >= m_sides.getNumSides())
		m_curPlayerIdx = m_sides.getNumSides()-1;

	// update player list
	CListBox *list = (CListBox*)GetDlgItem(IDC_PLAYERS);
	list->ResetContent();

	Int len = m_sides.getNumSides();
	int i = 0;
	for (; i < len; i++)
	{
		Dict *d = m_sides.getSideInfo(i)->getDict();
		AsciiString name = d->getAsciiString(TheKey_playerName);
		UnicodeString uni = d->getUnicodeString(TheKey_playerDisplayName);
		AsciiString fmt;
		if (name.isEmpty())
			fmt = NEUTRAL_NAME_STR;
		else
			fmt.format("%s=\"%ls\"",name.str(),uni.str());
		list->AddString(fmt.str());
	}

	Dict *pdict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();
	AsciiString cur_pname = pdict->getAsciiString(TheKey_playerName);
	UnicodeString cur_pdname = pdict->getUnicodeString(TheKey_playerDisplayName);
	Bool isNeutral = cur_pname.isEmpty();

	// update player name
	{
		CWnd *playername = GetDlgItem(IDC_PLAYERNAME);
		playername->EnableWindow(!isNeutral);	// neutral names are not editable
		playername->GetWindowText(buffer, sizeof(buffer)-2);
		if (strcmp(cur_pname.str(), buffer) != 0)
			playername->SetWindowText(cur_pname.str());
	}

	// update display name
	{
		CWnd *playerdname = GetDlgItem(IDC_PLAYERDISPLAYNAME);
		playerdname->EnableWindow(!isNeutral);	// neutral names are not editable
		playerdname->GetWindowText(buffer, sizeof(buffer)-2);
		AsciiString cur_pdnamea;
		cur_pdnamea.translate(cur_pdname);
		if (strcmp(cur_pdnamea.str(), buffer) != 0)
			playerdname->SetWindowText(cur_pdnamea.str());
	}

	// update color button
	{
		RGBColor rgb;
		Bool hasColor = false;
		Int color = pdict->getInt(TheKey_playerColor, &hasColor);
		if (hasColor) {
			rgb.setFromInt(color);
		} else {
			AsciiString tmplname = pdict->getAsciiString(TheKey_playerFaction);
			const PlayerTemplate* pt = ThePlayerTemplateStore->findPlayerTemplate(NAMEKEY(tmplname));
			if (pt) {
				rgb = *pt->getPreferredColor();
			}
			else
			{
				//MODDD - 'rgb' remains garbage memory if it's never set (ex: neutral player, no player template 'pt').
				// Use a default of white instead. This is more accurate to what would be seen for a truly neutral-controlled unit.
				rgb.setFromInt(0xFFFFFFFF);
			}
		}
		m_colorButton.setColor(rgb);
		SelectColor(rgb);

	}

	// update control button
	{
		Bool isHuman = pdict->getBool(TheKey_playerIsHuman);
		CButton *controller = (CButton*)GetDlgItem(IDC_PLAYERISCOMPUTER);
		controller->SetCheck(isHuman ? 0 : 1);
		controller->EnableWindow(!isNeutral);
	}

	// update factions popup
	{
		CComboBox *factions = (CComboBox*)GetDlgItem(IDC_PLAYERFACTION);
		factions->ResetContent();
		if (ThePlayerTemplateStore)
		{
			//MODDD - a new first item to be the placeholder for lacking a faction - namely for the neutral player to be able
			// to switch back to in case this is accidentally changed
			factions->AddString("<none>");

			//MODDD - instead of being added directly, the actual factions will be added to a temp(memory) list and sorted
			// here since the styling (CBS_SORT) was removed to do this automatically.
			// This ensures the new "<none>" item isn't part of the re-ordering, just in case factions with some really weird
			// symbols are ever added to go above and change the significance of being "item #0" (could get very confusing).
			std::vector<CString> factionNames;
			for (i = 0; i < ThePlayerTemplateStore->getPlayerTemplateCount(); i++)
			{
				//MODDD - replaced
				// ---
				//AsciiString nm = ThePlayerTemplateStore->getNthPlayerTemplate(i)->getName();
				//factions->AddString(nm.str());
				// ---
				factionNames.push_back(ThePlayerTemplateStore->getNthPlayerTemplate(i)->getName().str());
				// ---
			}

			//MODDD - new per explanation further above
			// ---
			std::sort
			(
				factionNames.begin(),
				factionNames.end(),
				&compareStringCaseInsensitive
			);

			// finally, add the ordered faction names to the combobox
			for (i = 0; i < factionNames.size(); ++i)
			{
				factions->AddString(factionNames[i]);
			}
			// ---
		}
		//MODDD - changing this to handle the special case of the faction string being empty (neutral player by default)
		// and better handling not finding a faction at all: stating as such in display text separate from any menu
		// options.
		// ------------------------
		//i = factions->FindStringExact(-1, pdict->getAsciiString(TheKey_playerFaction).str());
		//factions->SetCurSel(i);
		// ------------------------
		const AsciiString& playerFaction = pdict->getAsciiString(TheKey_playerFaction);
		if (playerFaction.isEmpty())
		{
			// special case: select the new option #0 just for this
			factions->SetCurSel(0);
		}
		else
		{
			// try to find a fitting dropdown option as usual
			// Note that the first param 'nIndexStart' has been changed from -1 to 0 to skip the first item, since it's
			// now the placeholder for the empty faction string
			i = factions->FindStringExact(0, playerFaction.str());
			if (i != CB_ERR)
			{
				// success
				factions->SetCurSel(i);
			}
			else
			{
				// error - could not find the faction in the list - let the user know
				factions->SetCurSel(-1);
				char errorTextBuf[256];
				snprintf(errorTextBuf, ARRAY_SIZE(errorTextBuf), "MISSING: %s",playerFaction.str());
				factions->SetWindowText(errorTextBuf);
			}
		}
	}
	// ------------------------
	
	// update allies & enemies
	CListBox *allieslist = (CListBox*)GetDlgItem(IDC_ALLIESLIST);
	CListBox *enemieslist = (CListBox*)GetDlgItem(IDC_ENEMIESLIST);
	CListBox *regardOthers = (CListBox*)GetDlgItem(IDC_PLAYER_ATTITUDE_OUT);
	CListBox *regardMe = (CListBox*)GetDlgItem(IDC_PLAYER_ATTITUDE_IN);

	AsciiString cur_allies = m_sides.getSideInfo(m_curPlayerIdx)->getDict()->getAsciiString(TheKey_playerAllies);
	buildAlliesList(allieslist, m_sides, cur_pname);
	selectAlliesList(allieslist, m_sides, cur_allies);
	allieslist->EnableWindow(!isNeutral);

	AsciiString cur_enemies = m_sides.getSideInfo(m_curPlayerIdx)->getDict()->getAsciiString(TheKey_playerEnemies);
	buildAlliesList(enemieslist, m_sides, cur_pname);
	selectAlliesList(enemieslist, m_sides, cur_enemies);
	enemieslist->EnableWindow(!isNeutral);

	regardOthers->ResetContent();
	regardMe->ResetContent();
	const char* rstr;
	AsciiString pname;
	for (i = 0; i < m_sides.getNumSides(); i++)
	{
		pname = m_sides.getSideInfo(i)->getDict()->getAsciiString(TheKey_playerName);
		if (pname.isEmpty() || pname == cur_pname)
			continue;	// skip neutral and self
		pname = playerNameForUI(m_sides, i);

		rstr = calcRelationStr(m_sides, m_curPlayerIdx, i);
		snprintf(buffer, ARRAY_SIZE(buffer), "%s: %s",pname.str(),rstr);
		regardOthers->AddString(buffer);

		rstr = calcRelationStr(m_sides, i, m_curPlayerIdx);
		snprintf(buffer, ARRAY_SIZE(buffer), "%s: %s",pname.str(),rstr);
		regardMe->AddString(buffer);
	}

	list->SetCurSel(m_curPlayerIdx);

	CWnd *newbtn = GetDlgItem(IDC_NEWPLAYER);
	CWnd *editbtn = GetDlgItem(IDC_EDITPLAYER);
	CWnd *rmvbtn = GetDlgItem(IDC_REMOVEPLAYER);
	Dict *playerDict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();
	Bool isneutral = playerDict->getAsciiString(TheKey_playerName).isEmpty();
	if( newbtn )
		newbtn->EnableWindow(m_sides.getNumSides() < MAX_PLAYER_COUNT);
	if( editbtn )
		editbtn->EnableWindow(!isneutral);
	if( rmvbtn )
		rmvbtn->EnableWindow(m_sides.getNumSides() > 1 && !isneutral);

	Invalidate();
	UpdateWindow();

	--m_updating;
}


BOOL PlayerListDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_updating = 0;
	m_sides = *TheSidesList;
	m_curPlayerIdx = thePrevCurPlyr;

	//MODDD - why did the original devs do this switch-around thing to copy some info from the original, init the new
	// subclass UI item with it, and then delete the original?
	// Seems some things might be lost in translation on doing this, particularly for the now-subclassed IDC_PLAYERFACTION
	// combobox (rather broken if the retail approach below is used for that).
	// See a 'DDX_Control' line per item in this dialog's 'DoDataExchange' method for the replacement for this.
	/*
	CRect rect;
	CWnd *item = GetDlgItem(IDC_PlayerColor);
	if (item) {
		item->GetWindowRect(&rect);
		ScreenToClient(&rect);
		DWORD style = item->GetStyle();
		m_colorButton.Create("", style, rect, this, IDC_PlayerColor);
		item->DestroyWindow();
	}
	*/

	//MODDD - new location
	PopulateColorComboBox();

	updateTheUI();
	//MODDD - moved above, fixes slight issue of the selected color choice not being 1-1 with the UI on opening the window (combobox options weren't in yet)
	//PopulateColorComboBox();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PlayerListDlg::OnDblclkPlayers()
{
	OnEditplayer();
}

void PlayerListDlg::OnColorPress()
{
	Dict *playerDict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();

	//MODDD - start with the color dialog set to the current color
	//CColorDialog dlg;
	CColorDialog dlg(CButtonShowColor::RGBtoBGR(m_colorButton.getColor().getAsInt()), CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT|CC_SOLIDCOLOR);
	if (dlg.DoModal() == IDOK) {
		m_colorButton.setColor(CButtonShowColor::BGRtoRGB(dlg.GetColor()));
		RGBColor color = m_colorButton.getColor();
		playerDict->setInt(TheKey_playerColor, color.getAsInt());
	}
	updateTheUI();
}

void PlayerListDlg::PopulateColorComboBox()
{
	Int numColors = TheMultiplayerSettings->getNumColors();
	AsciiString colorName;

	CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_PlayerColorCombo);
	if (pCombo) {
		for (Int c=0; c<numColors; ++c)
		{
			MultiplayerColorDefinition *def = TheMultiplayerSettings->getColor(c);
			if (!def)
				continue;
			UnicodeString colorName = TheGameText->fetch(def->getTooltipName().str());
			AsciiString str;
			str.translate(colorName);
			pCombo->AddString(str.str());
		}
	}
}

void PlayerListDlg::SelectColor(RGBColor rgb)
{
	Int numColors = TheMultiplayerSettings->getNumColors();
	AsciiString colorName;
	Bool selected = false;

	CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_PlayerColorCombo);
	if (pCombo) {
		for (Int c=0; c<numColors; ++c)
		{
			MultiplayerColorDefinition *def = TheMultiplayerSettings->getColor(c);
			if (!def)
				continue;
			if (rgb.getAsInt() == def->getRGBValue().getAsInt()) {
				pCombo->SetCurSel(c);
				selected = true;
				break;
			}
		}
		if (!selected) {
			pCombo->SetCurSel(-1);
		}
	}
}

void PlayerListDlg::OnSelectPlayerColor()
{
	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_PlayerColorCombo);
	Dict *playerDict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();
	if (pCombo && playerDict) {
		CString str;
		pCombo->GetWindowText(str);
		Int index = -1;
		Int numColors = TheMultiplayerSettings->getNumColors();
		Int c=0;
		for (; c<numColors; ++c)
		{
			MultiplayerColorDefinition *def = TheMultiplayerSettings->getColor(c);
			if (!def)
				continue;
			UnicodeString colorName = TheGameText->fetch(def->getTooltipName().str());
			AsciiString asciiColor;
			asciiColor.translate(colorName);

			if (str == asciiColor.str()) {
				index = c;
				break;
			}
		}
		if (index >= 0) {
			Int color = TheMultiplayerSettings->getColor(c)->getColor();
			playerDict->setInt(TheKey_playerColor, color);
		}
	}
	updateTheUI();
}

//MODDD - turned 'OnSelchangeAllieslist' into a utility for both 'OnSelchangeAllieslist' and 'OnSelchangeEnemieslist' to call.
// This allows clicking a side/player in the 'enemy' list to de-select that side in the 'ally' list if it was selected there.
// As-is, that click would appear to be ignored.
void PlayerListDlg::OnSelChangeRelationshipList(RelationshipListType relationshipListUpdated)
{
	Dict *playerDict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();
	AsciiString pname = playerDict->getAsciiString(TheKey_playerName);
	Bool isneutral = pname.isEmpty();
	if (isneutral)
		return;

	CListBox *allieslist = (CListBox*)GetDlgItem(IDC_ALLIESLIST);
	AsciiString allies = extractFromAlliesList(allieslist, m_sides);

	CListBox *enemieslist = (CListBox*)GetDlgItem(IDC_ENEMIESLIST);
	AsciiString enemies = extractFromAlliesList(enemieslist, m_sides);

	//MODDD - check for the new param
	if (relationshipListUpdated == ALLIES_RELATIONSHIP_LIST)
	{
		// remove enemies that are now in the allies list
		// or, making a player an ally that's in the enemies list removes it from the enemies list
		enemies = removeDupsFromEnemies(allies, enemies);
	}
	else
	{
		// vice versa
		allies = removeDupsFromEnemies(enemies, allies);
	}

	m_sides.getSideInfo(m_curPlayerIdx)->getDict()->setAsciiString(TheKey_playerAllies, allies);
	m_sides.getSideInfo(m_curPlayerIdx)->getDict()->setAsciiString(TheKey_playerEnemies, enemies);

	updateTheUI();
}

void PlayerListDlg::OnSelchangeAllieslist()
{
	//MODDD
	// <contents used as the basis for 'OnSelChangeRelationshipList'>
	OnSelChangeRelationshipList(ALLIES_RELATIONSHIP_LIST);
}

void PlayerListDlg::OnSelchangeEnemieslist()
{
	//MODDD
	//OnSelchangeAllieslist();
	OnSelChangeRelationshipList(ENEMIES_RELATIONSHIP_LIST);
}

void PlayerListDlg::OnOK()
{
	Bool modified = m_sides.validateSides();
	(void)modified;
	DEBUG_ASSERTLOG(!modified,("had to clean up sides in CTeamsDialog::OnOK"));

	CWorldBuilderDoc* pDoc = CWorldBuilderDoc::GetActiveDoc();
	SidesListUndoable *pUndo = new SidesListUndoable(m_sides, pDoc);
	pDoc->AddAndDoUndoable(pUndo);
	REF_PTR_RELEASE(pUndo); // belongs to pDoc now.

	thePrevCurPlyr = m_curPlayerIdx;

	CDialog::OnOK();
}

void PlayerListDlg::OnCancel()
{
	CDialog::OnCancel();
}

void PlayerListDlg::OnPlayeriscomputer()
{
	CButton *b = (CButton*)GetDlgItem(IDC_PLAYERISCOMPUTER);
	m_sides.getSideInfo(m_curPlayerIdx)->getDict()->setBool(TheKey_playerIsHuman, b->GetCheck() == 0);

	updateTheUI();
}

void PlayerListDlg::OnEditchangePlayerfaction()
{
	CComboBox *faction = (CComboBox*)GetDlgItem(IDC_PLAYERFACTION);

	if (faction) {
		// get the text out of the combo. If it is user-typed, sel will be -1, otherwise it will be >=0
		CString theText;
		Int sel = faction->GetCurSel();

		//MODDD - changing how this works since index 0 is now a special item for "no faction"
		/*
		if (sel >= 0) {
			faction->GetLBText(sel, theText);
		} else {
			faction->GetWindowText(theText);
		}
		*/
		if (sel == 0)
		{
			theText = "";
		}
		else if (sel != -1)
		{
			faction->GetLBText(sel, theText);
		}
		else
		{
			// if -1, the only possibility is error text (this field is not editable by the user).
			// Don't try to handle this - stop
			return;
		}

		AsciiString name((LPCTSTR)theText);

		Dict *pdict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();
		pdict->setAsciiString(TheKey_playerFaction, name);

		updateTheUI();
	}
}

void PlayerListDlg::OnChangePlayername()
{
	CWnd *playername = GetDlgItem(IDC_PLAYERNAME);
	char buf[1024];
	playername->GetWindowText(buf, sizeof(buf)-2);

	Dict *pdict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();
	AsciiString pnamenew(buf);
	AsciiString pnameold = pdict->getAsciiString(TheKey_playerName);

	if (pnameold == pnamenew)
		return;	// hmm, no change, so just punt.

	if (m_sides.findSideInfo(pnamenew))
	{
		::AfxMessageBox(IDS_NAME_IN_USE);
	}
	else
	{
		pdict->setAsciiString(TheKey_playerName, pnamenew);
		ensureValidPlayerName(pdict);

		updateAllTeams(m_sides, pnameold, pnamenew);
		fixDefaultTeamName(m_sides, pnameold, pnamenew);
	}

	updateTheUI();
}

void PlayerListDlg::OnChangePlayerdisplayname()
{
	CWnd *playername = GetDlgItem(IDC_PLAYERDISPLAYNAME);
	char buf[1024];
	playername->GetWindowText(buf, sizeof(buf)-2);

	Dict *pdict = m_sides.getSideInfo(m_curPlayerIdx)->getDict();

	AsciiString tmp(buf);
	UnicodeString pnamenew;
	pnamenew.translate(tmp);
	UnicodeString pnameold = pdict->getUnicodeString(TheKey_playerDisplayName);

	if (pnameold == pnamenew)
		return;	// hmm, no change, so just punt.

	pdict->setUnicodeString(TheKey_playerDisplayName, pnamenew);

	updateTheUI();
}

//MODDD - removing the third param. Display name is always the same as the internal player name so why not
// handle that here?
//static void addSide(SidesList *sides, AsciiString faction,
//										AsciiString playerName, const wchar_t *playerUName)
static void addSide(SidesList *sides, AsciiString faction, AsciiString playerName)
{
	if (!sides->findSideInfo(playerName)) {

		Dict newPlayerDict;
		UnicodeString playerUStr;

		//MODDD - deciding here now
		//playerUStr = playerUName;
		playerUStr.translate(playerName);

		newPlayerDict.setAsciiString(TheKey_playerName, playerName);
		newPlayerDict.setBool(TheKey_playerIsHuman, false);
		newPlayerDict.setUnicodeString(TheKey_playerDisplayName, playerUStr);
		newPlayerDict.setAsciiString(TheKey_playerFaction, faction);
		newPlayerDict.setAsciiString(TheKey_playerEnemies, "");
		newPlayerDict.setAsciiString(TheKey_playerAllies, "");

		ensureValidPlayerName(&newPlayerDict);
		sides->addSide(&newPlayerDict);

		Bool modified = sides->validateSides();
		(void)modified;
		DEBUG_ASSERTLOG(!modified,("had to clean up sides in PlayerListDlg::OnNewplayer"));
	}
}

void PlayerListDlg::OnAddskirmishplayers()
{
	//MODDD - replacing the hardcoded 'addSide' calls with something that's more flexible to the PlayerTemplate's currently
	// loaded in case of mods, though most tend to use existing (retail) 'Faction...' names internally anyway, possibly
	// just for compatibility with the auto-populated sides by this very script block.
	// ------------------------------------------------
	// Needed for a lookup to see if a faction(PlayerTemplate) has info in 'ChallengeMode.ini' indicating whether it is
	// locked or not ("StartsLocked"). This is how retail knows to hide the boss general from the skirmish
	// who-to-play-as dropdown. Lacking a starting building also hides the faction (how civilian & observer factions are
	// hidden). The Contra mod also uses this: boss factions deliberately lack this -> hidden.
	// Note that the worldbuilder doesn't call 'initSubsystem(TheGameClient...' which would've handled initializing
	// challenge generals info.
	TheChallengeGenerals = createChallengeGenerals();
 	TheChallengeGenerals->init();

	// First, add the civilian player.
	// Note that 'TheSidesList->addPlayerByTemplate' includes creating a team for the player - don't want that here.
	// I'll stick to preserving retail behavior as much as possible for the internals of mutating the sides list.
	// Also, 'CWorldBuilderDoc::OnNewDocument()' has since been edited to include creating the civilian player when a map
	// is created anyway - this is fine since already having a side with the expected name blocks redundant creation.
	// This will still be done in case the player manually deleted the civ side.
	// Lastly, this uses the normal player name ("Plyr") instead of "Skirmish" below, also to match retail behavior.
	const PlayerTemplate* ptCiv = ThePlayerTemplateStore->findPlayerTemplateWithSideFieldValue("Civilian");
	addSide(&m_sides, ptCiv->getName(), TheSidesList->getPlayerNameForTemplate(ptCiv));

	// For the rest of the factions, each follows a pattern of "PlayerTemplate.name:FactionX" -> "Side.name: SkirmishX".
	// This won't create "SkirmishCivilian" because lacking a starting building blocks the civilian template from
	// adding a side here. Same case for avoiding "SkirmishObserver".
	int i;
	for (i = 0; i < ThePlayerTemplateStore->getPlayerTemplateCount(); ++i)
	{
		const PlayerTemplate* pt = ThePlayerTemplateStore->getNthPlayerTemplate(i);
		if (!pt)
			continue;
		
		if (pt->getStartingBuilding().isEmpty())
			continue;

		Bool disallowLockedGenerals = TRUE;
		const GeneralPersona *general = TheChallengeGenerals->getGeneralByTemplateName(pt->getName());
		Bool startsLocked = general ? !general->isStartingEnabled() : FALSE;
		if (disallowLockedGenerals && startsLocked)
			continue;

		// Finally, add the side a player is expected to be able to play as in skirmish
		addSide(&m_sides, pt->getName(), TheSidesList->getSkirmishPlayerNameForTemplate(pt));
	}

	// Delete the loaded challenge generals info as TheGameClient's deconstructor would have.
	delete TheChallengeGenerals;
	// ------------------------------------------------

	updateTheUI();
}
