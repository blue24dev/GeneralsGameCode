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

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: PlayerList.cpp /////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Westwood Studios Pacific.
//
//                       Confidential Information
//                Copyright (C) 2001 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
// Project:   RTS3
//
// File name: PlayerList.cpp
//
// Created:   Steven Johnson, October 2001
//
// Desc:      @todo
//
//-----------------------------------------------------------------------------

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Errors.h"
#include "Common/DataChunk.h"
#include "Common/GameState.h"
#include "Common/GlobalData.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/PlayerTemplate.h"
#include "Common/Team.h"
#include "Common/WellKnownKeys.h"
#include "Common/Xfer.h"
#ifdef RTS_DEBUG
#include "GameLogic/Object.h"
#endif
#include "GameLogic/SidesList.h"
#include "GameNetwork/GameInfo.h"
#include "GameNetwork/NetworkDefs.h"

//MODDD
#include "GameLogic/GameLogic.h"

//-----------------------------------------------------------------------------
/*extern*/ PlayerList *ThePlayerList = nullptr;

//-----------------------------------------------------------------------------
PlayerList::PlayerList() :
	m_local(nullptr),
	m_playerCount(0)
{
	// we only allocate a few of these, so don't bother pooling 'em
	for (Int i = 0; i < MAX_PLAYER_COUNT; i++)
		m_players[ i ] = NEW Player( i );
	init();
}

//-----------------------------------------------------------------------------
PlayerList::~PlayerList()
{
	// the world is happier if we reinit things before destroying them,
	// to avoid debug warnings
	init();

	for( Int i = 0; i < MAX_PLAYER_COUNT; ++i )
		delete m_players[ i ];
}

//-----------------------------------------------------------------------------
Player *PlayerList::getNthPlayer(Int i)
{
	if( i < 0 || i >= MAX_PLAYER_COUNT )
	{
//		DEBUG_CRASH( ("Illegal player index") );
		return nullptr;
	}
	return m_players[i];
}

//MODDD - tell whether this is a player that typically owns non-playable-user-owned-things, such as unmanned vehicles,
// ungarrisoned buildings, tech structures, etc.
// In many cases, this is often a standard-named 'civilian' player, so this check is better than only a '... == neutral player' check.
Bool PlayerList::isPlayerUnaffiliated(const Player* player) const
{
	// Using the cached var instead of 'getNeutralPlayer()' in case this check is frequent (oh who am I kidding, this has to be micro optimization)
	if (player == m_neutralPlayer)
	{
		return TRUE;
	}

	// even if 'm_civilianPlayer' were null, this would just never pass - 'player' should not be null
	if (player == m_civilianPlayer)
	{
		return TRUE;
	}

	// No side at all? Observer?
	if (player->getPlayerTemplate() == nullptr || player->isPlayerObserver())
	{
		return TRUE;
	}

	// Is this player using the "FactionCivilian" side?
	// Going to be careful about this check - as likely said elsewhere, consider some pre-source-code-release Co-Op maps
	// that do weird things to get around the limitation of killing non-civilian-placed-things in a skirmish/multiplayer
	// game such as making a player use the civilian faction & make it mock a typical computer player.
	/*
	if (m_civilianPlayerTemplate != nullptr && player->getPlayerTemplate() == m_civilianPlayerTemplate)
	{
		return TRUE;
	}
	*/

	return FALSE;
}

//MODDD - a variant of above that allows being an observer - basically something believably 'playing' or watching the game,
// not just a bare-minimum background computer player that doesn't do much.
// Ex: cheats to reveal fog-of-war for all players
Bool PlayerList::isPlayerViewer(const Player* player) const
{
	if (player == m_neutralPlayer)
	{
		return FALSE;
	}

	if (player == m_civilianPlayer)
	{
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
Player *PlayerList::findPlayerWithNameKey(NameKeyType key)
{
	for (Int i = 0; i < m_playerCount; i++)
	{
		if (m_players[i]->getPlayerNameKey() == key)
		{
			return m_players[i];
		}
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
void PlayerList::reset()
{
	// TheSuperHackers @bugfix xezon 20/03/2025 Delete the Player AI before purging the dependent teams in the factory.
	for (int i = 0; i < MAX_PLAYER_COUNT; i++)
		m_players[i]->deletePlayerAI();

	TheTeamFactory->clear(); // cleans up energy, among other things
	
	//MODDD - un-set the existing 'm_local' if needed.
	// Typical call trace to reach here:
	//   GameEngine::reset (ending a game)
	//   GameEngine::resetSubsytems
	//   SubsystemInterfaceList::resetAll()
	//   PlayerList::reset (here)
	if (m_local)
		m_local->becomingLocalPlayer(false);

	//MODDD - NOTE - This will set 'm_local' to null
	init();
}

//MODDD - new
// Once all players have been built from the map, find the neutral player, likely to be cached for easier references later.
// (shouldn't need to call this over and over after finding & saving it once for a game)
// The as-is 'PlayerList' had the assumption that player #0 was always the neutral player, but 'SidesList::validateSides()'
// appears to suggest it could be any other player, depending on the first one encountered with an empty name.
Player* PlayerList::findNeutralPlayer()
{
	AsciiString targetPlayerName;
	Player* playerRef;

	targetPlayerName.set("");
	playerRef = this->findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(targetPlayerName));
	if (playerRef != nullptr)
	{
		return playerRef;
	}

	// shouldn't be possible to have no empty-named player - force the retail default of player #0 then?
	return m_players[0];
}

//MODDD - new
// Once all players have been built from the map, find the civilian player, similar to above.
Player* PlayerList::findCivilianPlayer()
{
	// First, try the strict check: "PlyrCivilian"
	AsciiString targetPlayerName;
	Player* playerRef;

	targetPlayerName.set("PlyrCivilian");
	playerRef = this->findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(targetPlayerName));
	if (playerRef != nullptr)
	{
		// Not part of retail, but could require it to have the 'Civilian' side name. I doubt this would ever not be the case.
		//if (playerRef->getSide().compare("Civilian") == 0)
			return playerRef;
	}

	// just "Civilian"?
	targetPlayerName.set("Civilian");
	playerRef = this->findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(targetPlayerName));
	if (playerRef != nullptr)
	{
		//if (playerRef->getSide().compare("Civilian") == 0)
			return playerRef;
	}

	// Try any player who's side is 'Civilian'.
	// ...actually, deciding against this. I've seen co-op intended maps for the retail game use "Civilian"-sided players
	// for everything so they're not automatically removed by the skirmish system's checks (only skirmish is possible for
	// network games in retail).
	/*
	if (m_civilianPlayerTemplate != nullptr)
	{
		Int i;
		for (i = 0; i < m_playerCount; i++)
		{
			//if (m_players[i]->getSide().compare("Civilian") == 0)
			if (m_players[i]->getPlayerTemplate() == m_civilianPlayerTemplate)
				return m_players[i];
		}
	}
	*/

	// Nothing worked - give up, not worth digging deeper when retail never expected this, or was at least ok dealing with
	// a lack of civilian player. There is still the neutral one guaranteed as player #0 in the list.
	// (using the first computer player for this isn't advisable, total crapshoot on being a normal active AI-controlled player)
	return nullptr;
}

//MODDD - new
// Helper to find the first human-intended player, since it could come from an expected name or have to come
// from the first non-computer-marked player.
Player* PlayerList::findFirstSlotPlayer()
{
	AsciiString targetPlayerName;
	Player* playerRef;
	// The name of the intended side/player to play as in the campaign was inconsistent but was usually one of these.
	const char* commonPlayerNames[] =
	{
		"ThePlayer",
		"Player",
		"PlyrPLAYER",
		"player0",
		// (test - did any retail maps use this one? Seems reasonable to occur)
		"PlyrPlayer"
	};
	int i;

	for (i = 0; i < sizeof(commonPlayerNames) / sizeof(commonPlayerNames[0]); ++i)
	{
		targetPlayerName.set(commonPlayerNames[i]);
		playerRef = this->findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(targetPlayerName));
		if (playerRef != nullptr)
		{
			// done searching
			return playerRef;
		}
	}

	// Fallback: find the first non-computer-marked side.
	for (i = 0; i < MAX_PLAYER_COUNT; i++)
	{
		playerRef = getNthPlayer(i);
		if (playerRef->getPlayerType() == PLAYER_HUMAN && !playerRef->isPlayerObserver())
		{
			return playerRef;
		}
	}

	// ???
	return nullptr;
}

//MODDD - new
// After creating the players from the sides in 'PlayerList::newGame()', find the human players for 'm_slotPlayerRefs'
// (this now uses 'setSlotIndex' added by 'TheSuperHackers' since).
// This is less error prone than assuming any non-computer-marked player is meant to be played as, because several maps
// had multiple non-computer-marked players, and most were clearly not intended to be played as.
// Only the player usually played as per single player logic (by an expected name, or the first non-computer-marked player),
// and subsequent ones by a "player1", "player2", etc. naming scheme, should be in the slot-to-player indices for
// skirmish/multiplayer logic to easily reach.
// This will allow it to mirror the choices of players selected per machine in GameLogic.cpp: getMultiplayerLocalSide.
// This lets any one machine be aware of who the other slot players are.
void PlayerList::populateSlotPlayerRefs()
{
	// Note that 'm_slotSoftMax' used to be incremented alongside 'setSlotIndex' under the assumption that
	// players would always be found to assign to slots in increasing order without any gaps. Deciding this at the end
	// instead for absolute safety.
	if (TheGameLogic->getGameMode() == GAME_SHELL)
	{
		// Shell map: find the first human-marked player, add to the list of slot player refs. That's it.
		int i;
		for (i = 0; i < this->getPlayerCount(); ++i)
		{
			Player* playerRef = getNthPlayer(i);
			if (playerRef->getPlayerType() == PLAYER_HUMAN)
			{
				setSlotIndex(playerRef->getPlayerIndex(), 0);
				break;
			}
		}
		return;
	}
	
	// For anything else, decide if this is a campaign map, a generals challenge (GC) map, or a skirmish map (basically 'other').
	// Note that both GC and skirmish maps use the skirmish system to populate players "player<0-7>" (done by the time this method
	// is reached), so string-lookups for those will work in either case.
	// GC maps are supposed to have a dummy "ThePlayer" played that isn't actually meant to be played as. So ignoring it works out
	// here (other players shouldn't be able to treat it as a slot player, i.e. something to target with support powers).
	// However, campaign maps are trickier - standard is to use the first non-computer-marked player to be played as.
	// For co-op support, all I can figure is to name players in a standard way (ex: "player<1-7>" like skirmish generation would),
	// because a map may have multiple non-computer-marked players as-is (can't determine which is meant to be played as by other
	// connected users in a network game).
	Bool isCampaignMap;

	// Check the setting for an easy assumption
#if FORCE_GAME_CONTEXT == FGC_GENERALS_CHALLENGE
	isCampaignMap = FALSE;
#elif FORCE_GAME_CONTEXT == FGC_CAMPAIGN
	isCampaignMap = TRUE;
#else
	// No constant to tell us. Check the game mode and the usual generals challenge check.
	// (remember that playing a campaign or GC map in the skirmish/network from the menus isn't possible here)
	// Actually just checking for 'TheGameInfo' being null or not (assume single player & not generals challenge) will work out.
	if (TheGameInfo == nullptr)
	{
		isCampaignMap = TRUE;
	}
	else
	{
		isCampaignMap = FALSE;
	}
#endif

	AsciiString targetPlayerName;
	Player* playerRef;
	int standardSlotPlayerSearch_firstSlotIndex;

	if (isCampaignMap)
	{
		// Campaign map - special handling to find the first player, the rest are expected to be named "player#" if
		// provided by the map (named that way) for co-op support

		// This can't possibly be null or else a similar earlier check (GameLogic.cpp: getMultiplayerLocalSide)
		// would have already crashed.
		playerRef = findFirstSlotPlayer();
		setSlotIndex(playerRef->getPlayerIndex(), 0);
		
		// For the remaining possible users, search for the expected name: "player<1-7>".
		standardSlotPlayerSearch_firstSlotIndex = 1;
	}
	else
	{
		// GC or Skirmish map - simple: "player#" for all players.
		standardSlotPlayerSearch_firstSlotIndex = 0;
	}

	// for the remaining slots to figure out (in a new scope for old VC safety, may be unnecessary)
	{
		int i;
		for (i = standardSlotPlayerSearch_firstSlotIndex; i < MAX_SLOTS; ++i)
		{
			targetPlayerName.format("player%d", i);
			playerRef = this->findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(targetPlayerName));
			if (playerRef != nullptr)
			{
				setSlotIndex(playerRef->getPlayerIndex(), i);
			}
			else
			{
				// Not found? Stop searching
				// Actually no - keep going in case a slot was skipped but there are still players for later set slots
				// Ex: player1 through 4 is found, 5 isn't but 6 exists - don't give up at 5!
				//break;
			}
		}
	}

	m_slotSoftMax = determineSlotSoftMax();
}

//MODDD - only used for 'populateSlotPlayerRefs' to decide what the highest slot number in use is after setting slot-to-player indices
Int PlayerList::determineSlotSoftMax() const
{
	int i;
	for (i = MAX_SLOTS - 1; i >= 0; --i)
	{
		if (m_slotToPlayerIndices[i] != -1)
		{
			return i + 1;
		}
	}

	// every single slot-to-player indice was -1(unset)? shouldn't be possible, but 0 would be accurate to that case
	return 0;
}

//MODDD - new event for handling some cheat/extra-difficulty-related settings to apply to players.
void PlayerList::postPlayersInit()
{
	int i;
	for (i=0; i < getPlayerCount(); ++i)
	{
		Player* player = ThePlayerList->getNthPlayer(i);

		// If this is a slot player and there is game info to check, see if this slot is unoccupied (skip if so)
		int slotIndex = getSlotIndex(player->getPlayerIndex());
		if (TheGameInfo && slotIndex != -1)
		{
			GameSlot *slot = TheGameInfo->getSlot(slotIndex);
			// NOTE - barring observer players with 'isParticipant' vs. 'isOccupied'
			if (!slot->isParticipant())
			{
				continue;
			}
		}
		
		//MODDD - this used to be handled in Player.cpp: initFromDict, but the 'slotIndex' wasn't seen since this
		// is assigned in 'populateSlotPlayerRefs' instead of earlier in SidesInfo (this has the benefit of being
		// re-determined on loading a saved game).
		// Could do something like assign 'slotIndex' before 'initFromDict' so the old way works but doesn't seem
		// worth it to decide what-player-belongs-to-what-slot so far in advance for only this cheap hack.
		// So, decide who gets the n00b bonus here instead.
#if NOOB_MODE
		if (slotIndex == 1 && player->getPlayerType() == PLAYER_HUMAN)
		{
			// get the current amount of money, remove it from the player, add it back with a scalar applied
			Money* moneyRef = player->getMoney();
			UnsignedInt currentMoney = moneyRef->countMoney();
			moneyRef->withdraw(currentMoney);
			moneyRef->deposit((UnsignedInt)((Real)currentMoney * NOOB_START_MONEY_SCALAR), FALSE, FALSE);
			continue;
		}
#endif
	}
}

//-----------------------------------------------------------------------------
void PlayerList::newGame()
{
	Int i;

	DEBUG_ASSERTCRASH(this != nullptr, ("null this"));

	reset();

	// ok, now create the rest of players we need.
	//MODDD - no need for this now
	//Bool setLocal = false;
	for( i = 0; i < TheSidesList->getNumSides(); i++)
	{
		Dict *d = TheSidesList->getSideInfo(i)->getDict();
		AsciiString pname = d->getAsciiString(TheKey_playerName);

		/// @todo The Player class should have a reset() method, instead of directly calling initFromDict() (MSB)
		Player* p = m_players[m_playerCount++];
		
		//MODDD - moved the empty-name check below getting the current player & incrementing 'playerCount',
		// and calling a special 'initNeutral' here.
		// There should still never be more than one blank-named / 'neutral' player, however.
		if (pname.isEmpty())
		{
			p->initNeutral();
			continue;	// it's neutral, which we've already done, so skip it.
		}

		//MODDD - quick hack. Carry the 'slotIndex' from the side over to this player.
		// (do this early since some stuff in 'initFromDict' might need to know this in time)
		// UPDATE - handled elsewhere now
		//p->slotIndex = TheSidesList->getSideInfo(i)->slotIndex;

		p->initFromDict(d);

		// Multiplayer override
		Bool exists;	// throwaway, since we don't care if it exists
		if (d->getBool(TheKey_multiplayerIsLocal, &exists))
		{
			DEBUG_LOG(("Player %s is multiplayer local", pname.str()));
			setLocalPlayer(p);
			//setLocal = true;
		}
		
		//MODDD - NOTE - this block should never be needed to tell who the local player is for FGC_CAMPAIGN.
		// Who the local player is, is determined by setting 'TheKey_multiplayerIsLocal' for a side & it's reached
		// parsing that side's dict here (see above).
		// This works in case a player other than the first human player is the local player for this machine
		// (co-op mode under FGC_CAMPAIGN).
		// You could argue that all non-network game modes should also use the 'TheKey_multiplayerIsLocal' way for
		// consistency and remove this block entirely (probably only campaign & shell game modes relied on this block as of
		// retail, even generals challenge is just a reskin of skirmish).
#if FORCE_GAME_CONTEXT != FGC_CAMPAIGN
		//MODDD
		//if (!setLocal && !TheNetwork && d->getBool(TheKey_playerIsHuman))
		if (!isLocalPlayerSet() && !TheNetwork && d->getBool(TheKey_playerIsHuman))
		{
			setLocalPlayer(p);
			//setLocal = true;
		}
#else
		// Do a check for having the 'IsHuman' bool anyway.
		// UPDATE - no longer a good assumption in case of non-computer-marked players that aren't intended for someone to play as.
		// See the 'populateSlotPlayerRefs' after this loop through all sides.
		/*
		if (d->getBool(TheKey_playerIsHuman))
		{
			setSlotIndex(p->getPlayerIndex(), 0);
		}
		*/
#endif

		// Set the build list.
		p->setBuildList(TheSidesList->getSideInfo(i)->getBuildList());
		// Build list is attached to player now, so release it from the side info.
		TheSidesList->getSideInfo(i)->releaseBuildList();
	}

	//MODDD
	m_civilianPlayerTemplate = ThePlayerTemplateStore->findPlayerTemplate( NAMEKEY("FactionCivilian") );
	m_neutralPlayer = findNeutralPlayer();
	m_civilianPlayer = findCivilianPlayer();
	
	//MODDD
	//if (!setLocal)
	if (!isLocalPlayerSet())
	{
		DEBUG_ASSERTCRASH(TheNetwork, ("*** Map has no human player... picking first nonneutral player for control"));
		//MODDD - NOTE - this for-loop is going through 'getNumSides()' yet getting the 'i-th' player with 'i' instead of
		// the 'i-th' side. Is that intentional? Then again, this fallback shouldn't be needed in a well-formed map.
		for( i = 0; i < TheSidesList->getNumSides(); i++)
		{
			Player* p = getNthPlayer(i);
			if (p != getNeutralPlayer())
			{
				p->setPlayerType(PLAYER_HUMAN, false);
				setLocalPlayer(p);
				//setLocal = true;
				break;
			}
		}
	}

	// must reset teams *after* creating players.
	TheTeamFactory->initFromSides(TheSidesList);

	for( i = 0; i < TheSidesList->getNumSides(); i++)
	{
		Dict *d = TheSidesList->getSideInfo(i)->getDict();
		Player* p = findPlayerWithNameKey(NAMEKEY(d->getAsciiString(TheKey_playerName)));

		AsciiString tok;

		AsciiString enemies = d->getAsciiString(TheKey_playerEnemies);
		while (enemies.nextToken(&tok))
		{
			Player *p2 = findPlayerWithNameKey(NAMEKEY(tok));
			if (p2)
			{
				p->setPlayerRelationship(p2, ENEMIES);
			}
			else
			{
				DEBUG_LOG(("unknown enemy %s",tok.str()));
			}
		}

		AsciiString allies = d->getAsciiString(TheKey_playerAllies);
		while (allies.nextToken(&tok))
		{
			Player *p2 = findPlayerWithNameKey(NAMEKEY(tok));
			if (p2)
			{
				p->setPlayerRelationship(p2, ALLIES);
			}
			else
			{
				DEBUG_LOG(("unknown ally %s",tok.str()));
			}
		}

		// finally, make sure self & neutral are correct.
		p->setPlayerRelationship(p, ALLIES);
		if (p != getNeutralPlayer())
			p->setPlayerRelationship(getNeutralPlayer(), NEUTRAL);

		p->setDefaultTeam();
	}

	//MODDD - I'm going to replace this completey with the 'populateSlotPlayerRefs' I've already created before the change,
	// but changing assignments to my 'm_slotPlayerRefs' to use TheSuperHackers addition 'm_slotToPlayerIndices' instead.
	// Also, this was below the first loop through 'getNumSides()' at the start of this method (right below where
	// 'm_civilianPlayer = findCivilianPlayer();' is), but here should be fine too.
	// Also note that this happens regardless of 'TheGameInfo' being present in case some of my changes still expect the
	// slot-to-player indices to be set up as has been the case for a while
	// (check around 'TheGameInfo != nullptr' checks in ScriptEngine.cpp for examples).
	/*
	if (TheGameInfo)
	{
		assignSlotIndices(*TheGameInfo);
	}
	*/
	// ---
	populateSlotPlayerRefs();
	postPlayersInit();
	// ---

}

//-----------------------------------------------------------------------------
void PlayerList::init()
{
	//MODDD - changing the start 'playerCount' to 0, finding the neutral player increases the count like anything else.
	//m_playerCount = 1;
	m_playerCount = 0;

	//MODDD
	m_neutralPlayer = nullptr;
	m_civilianPlayer = nullptr;
	m_civilianPlayerTemplate = nullptr;
	m_slotSoftMax = 0;

	//MODDD - why separately?
	//m_players[0]->init(nullptr);

	//MODDD - all within this loop is fine (start val of 'i' changed: 1 -> 0)
	for (int i = 0; i < MAX_PLAYER_COUNT; i++)
		m_players[i]->init(nullptr);

	std::fill(m_slotIndices, m_slotIndices + ARRAY_SIZE(m_slotIndices), -1);
	std::fill(m_slotToPlayerIndices, m_slotToPlayerIndices + ARRAY_SIZE(m_slotToPlayerIndices), -1);

	// call setLocalPlayer so that becomingLocalPlayer() gets called appropriately
	//MODDD - just initting 'm_local' to null is good enough now
	//setLocalPlayer(m_players[0]);
	m_local = nullptr;
}

//-----------------------------------------------------------------------------
void PlayerList::update()
{
	// update all players
	for( Int i = 0; i < MAX_PLAYER_COUNT; i++ )
	{
		m_players[i]->update();
	}

}

//-----------------------------------------------------------------------------
void PlayerList::newMap()
{
	// update all players
	for( Int i = 0; i < MAX_PLAYER_COUNT; i++ )
	{
		m_players[i]->newMap();
	}

}

// ------------------------------------------------------------------------
void PlayerList::teamAboutToBeDeleted(Team* team)
{
	for( Int i = 0; i < MAX_PLAYER_COUNT; i++ )
	{
		m_players[i]->removeTeamRelationship(team);
	}
}

//=============================================================================
void PlayerList::updateTeamStates()
{
	// Clear team flags for all players.
	for( Int i = 0; i < MAX_PLAYER_COUNT; i++ )
	{
		m_players[i]->updateTeamStates();
	}
}

//-----------------------------------------------------------------------------
Team *PlayerList::validateTeam( AsciiString owner )
{
	// owner could be a player or team. first, check team names.
	Team *t = TheTeamFactory->findTeam(owner);
	if (!t)
	{
		DEBUG_CRASH(("no team or player named %s could be found!", owner.str()));
		t = getNeutralPlayer()->getDefaultTeam();
	}
	return t;
}

//MODDD - implementation moved from .h, changed a bit
Player* PlayerList::getLocalPlayer()
{
	return m_local ? m_local : m_players[0];
}

//MODDD - tell if the local player has been set to a non-default choice
Bool PlayerList::isLocalPlayerSet() const
{
	return m_local != nullptr;
}

//-----------------------------------------------------------------------------
void PlayerList::setLocalPlayer(Player *player)
{
	// can't set local player to null -- if you try, you get neutral.
	if (player == nullptr)
	{
		DEBUG_CRASH(("local player may not be null"));
		player = getNeutralPlayer();
	}

	if (player != m_local)
	{
		// m_local can be null the very first time we call this.
		if (m_local)
			m_local->becomingLocalPlayer(false);
		m_local = player;
		player->becomingLocalPlayer(true);
	}

#ifdef INTENSE_DEBUG
	if (player)
	{
		// did you know? you can use "%ls" to print a doublebyte string, even in a single-byte printf...
		DEBUG_LOG(("Switching local players. The new player is named '%ls' (%s) and owns the following objects:",
			player->getPlayerDisplayName().str(),
			TheNameKeyGenerator->keyToName(player->getPlayerNameKey()).str()
		));
		for (Object *obj = player->getFirstOwnedObject(); obj; obj = obj->getNextOwnedObject())
		{
			DEBUG_LOG_RAW(("Obj %08lx is of type %s",obj,obj->getTemplate()->getName().str()));
			if (!player->canBuild(obj->getTemplate()))
			{
				DEBUG_LOG_RAW((" (NOT BUILDABLE)"));
			}
			DEBUG_LOG_RAW(("\n"));
		}
	}
#endif

}

//-----------------------------------------------------------------------------
Player *PlayerList::getPlayerFromMask( PlayerMaskType mask )
{
	Player *player = nullptr;
	Int i;

	for( i = 0; i < MAX_PLAYER_COUNT; i++ )
	{

		player = getNthPlayer( i );
		if( player && player->getPlayerMask() == mask )
			return player;

	}

	DEBUG_CRASH( ("Player does not exist for mask") );
	return nullptr; // mask not found

}

//-----------------------------------------------------------------------------
Player *PlayerList::getEachPlayerFromMask( PlayerMaskType& maskToAdjust )
{
	Player *player = nullptr;
	Int i;

	for( i = 0; i < MAX_PLAYER_COUNT; i++ )
	{

		player = getNthPlayer( i );
		if ( player && BitIsSet(player->getPlayerMask(), maskToAdjust ))
		{
			maskToAdjust &= (~player->getPlayerMask());
			return player;
		}
	}

	DEBUG_CRASH( ("No players found that contain any matching masks.") );
	maskToAdjust = 0;
	return nullptr; // mask not found
}

//-------------------------------------------------------------------------------------------------
PlayerMaskType PlayerList::getPlayersWithRelationship( Int srcPlayerIndex, UnsignedInt allowedRelationships )
{
	PlayerMaskType retVal = 0;

	if (allowedRelationships == 0)
		return retVal;

	Player *srcPlayer = getNthPlayer(srcPlayerIndex);
	if (!srcPlayer)
		return retVal;

	if (BitIsSet(allowedRelationships, ALLOW_SAME_PLAYER))
		BitSet(retVal, srcPlayer->getPlayerMask());

	for ( Int i = 0; i < getPlayerCount(); ++i )
	{
		Player *player = getNthPlayer(i);
		if (!player)
			continue;

		if (player == srcPlayer)
			continue;

		switch (srcPlayer->getRelationship(player->getDefaultTeam()))
		{
			case ENEMIES:
				if (BitIsSet(allowedRelationships, ALLOW_ENEMIES))
					BitSet(retVal, player->getPlayerMask());
				break;
			case ALLIES:
				if (BitIsSet(allowedRelationships, ALLOW_ALLIES))
					BitSet(retVal, player->getPlayerMask());
				break;
			case NEUTRAL:
				if (BitIsSet(allowedRelationships, ALLOW_NEUTRAL))
					BitSet(retVal, player->getPlayerMask());
				break;
		}
	}

	return retVal;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void PlayerList::crc( Xfer *xfer )
{
	xfer->xferInt( &m_playerCount );

	for( Int i = 0; i < m_playerCount; ++i )
		xfer->xferSnapshot( m_players[ i ] );
}

// ------------------------------------------------------------------------------------------------
/** Xfer Method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void PlayerList::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// xfer the player count
	Int playerCount = m_playerCount;
	xfer->xferInt( &playerCount );

	//
	// sanity, the player count read from the file should match our player count that
	// was setup from the bare bones map load since that data can't change during run time
	//
	if( playerCount != m_playerCount )
	{

		DEBUG_CRASH(( "Invalid player count '%d', should be '%d'", playerCount, m_playerCount ));
		throw SC_INVALID_DATA;

	}

	// xfer each of the player data
	for( Int i = 0; i < playerCount; ++i )
		xfer->xferSnapshot( m_players[ i ] );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void PlayerList::loadPostProcess()
{

}

//-----------------------------------------------------------------------------
void PlayerList::setSlotIndex(Int playerIndex, Int slotIndex)
{
	if (playerIndex >= 0 && playerIndex < ARRAY_SIZE(m_slotIndices))
	{
		const Int oldSlotIndex = m_slotIndices[playerIndex];
		if (oldSlotIndex >= 0 && oldSlotIndex < ARRAY_SIZE(m_slotToPlayerIndices))
		{
			m_slotToPlayerIndices[oldSlotIndex] = -1;
		}

		m_slotIndices[playerIndex] = slotIndex;

		if (slotIndex >= 0 && slotIndex < ARRAY_SIZE(m_slotToPlayerIndices))
		{
			m_slotToPlayerIndices[slotIndex] = playerIndex;
		}
	}
}

//-----------------------------------------------------------------------------
Int PlayerList::getSlotIndex(Int playerIndex) const
{
	if (playerIndex >= 0 && playerIndex < ARRAY_SIZE(m_slotIndices))
	{
		return m_slotIndices[playerIndex];
	}

	return -1;
}

//-----------------------------------------------------------------------------
Int PlayerList::getPlayerIndexFromSlotIndex(Int slotIndex) const
{
	if (slotIndex >= 0 && slotIndex < ARRAY_SIZE(m_slotToPlayerIndices))
	{
		return m_slotToPlayerIndices[slotIndex];
	}

	return -1;
}

//-----------------------------------------------------------------------------
Player *PlayerList::getPlayerFromSlotIndex(Int slotIndex) const
{
	const Int playerIndex = getPlayerIndexFromSlotIndex(slotIndex);
	if (playerIndex >= 0 && playerIndex < m_playerCount)
	{
		return m_players[playerIndex];
	}

	return nullptr;
}

//MODDD - new. Variant of above (see notes below)
Player *PlayerList::getPlayerFromSlotIndex_requireOccupied(Int slotIndex) const
{
	if (TheGameInfo != nullptr)
	{
		GameSlot *slot = TheGameInfo->getSlot(slotIndex);
		if (!slot->isOccupied())
		{
			return nullptr;
		}
	}

	return getPlayerFromSlotIndex(slotIndex);
}

//MODDD - new. Variant of above that includes a filter for coming from a slot that's actively participating in the game
// (filled by someone, not observer) if that information is available ('TheGameInfo' is present).
// Mainly a convenience feature to bundle whether there is 'TheGameInfo' to check and the result of a player-lookup being
// null into one call (places using this tend not to need any other info about slots).
Player *PlayerList::getPlayerFromSlotIndex_requireActiveParticipant(Int slotIndex) const
{
	// If 'TheGameInfo' is null, this lookup for slot info to verify being non-open/closed & non-observer isn't possible.
	// This is the case for playing the single player campaign in the original/retail state of the game, not my co-op
	// additions. Of course 'you' are here playing the game as the only valid 'slot' (local player), so skipping this works
	// out just fine then.
	if (TheGameInfo != nullptr)
	{
		GameSlot *slot = TheGameInfo->getSlot(slotIndex);
		if (!slot->isParticipant())
		{
			// regardless of a player being connected (ex: observer), not relevant for the caller
			return nullptr;
		}
	}

	// passed/skipped the above filter - pass along whatever the base variant finds
	// and may as well bundle in a 'is-active' check
	Player* p = getPlayerFromSlotIndex(slotIndex);
	if (p == nullptr || !p->isPlayerActive())
	{
		return nullptr;
	}
	return p;
}

//-----------------------------------------------------------------------------
void PlayerList::assignSlotIndices(const GameInfo& gameInfo)
{
	AsciiString playerName;

	for (Int i = 0; i < MAX_SLOTS; ++i)
	{
		const GameSlot* slot = gameInfo.getConstSlot(i);
		if (!slot || !slot->isOccupied())
			continue;

		playerName.format("player%d", i);

		if (Player* player = findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(playerName)))
		{
			setSlotIndex(player->getPlayerIndex(), i);
		}
	}
}
