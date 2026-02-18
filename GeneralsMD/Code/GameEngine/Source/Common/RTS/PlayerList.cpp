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
#include "GameNetwork/NetworkDefs.h"

//MODDD
#include "GameNetwork/GameInfo.h"


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
	init();
}

//-----------------------------------------------------------------------------
void PlayerList::newGame()
{
	Int i;

	DEBUG_ASSERTCRASH(this != nullptr, ("null this"));

	reset();

	// ok, now create the rest of players we need.
	Bool setLocal = false;
	for( i = 0; i < TheSidesList->getNumSides(); i++)
	{
		Dict *d = TheSidesList->getSideInfo(i)->getDict();
		AsciiString pname = d->getAsciiString(TheKey_playerName);
		if (pname.isEmpty())
			continue;	// it's neutral, which we've already done, so skip it.

		/// @todo The Player class should have a reset() method, instead of directly calling initFromDict() (MSB)
		Player* p = m_players[m_playerCount++];

		//MODDD - quick hack. Carry the 'slotIndex' from the side over to this player.
		// (do this early since some stuff in 'initFromDict' might need to know this in time)
		p->slotIndex = TheSidesList->getSideInfo(i)->slotIndex;

		p->initFromDict(d);

		// Multiplayer override
		Bool exists;	// throwaway, since we don't care if it exists
		if (d->getBool(TheKey_multiplayerIsLocal, &exists))
		{
			DEBUG_LOG(("Player %s is multiplayer local", pname.str()));
			setLocalPlayer(p);
			setLocal = true;
		}
		
		//MODDD - NOTE - this block should never be needed to tell who the local player is for 'CAMPAIGN_FORCE'.
		// This is determined by setting 'TheKey_multiplayerIsLocal' (block above) for single-player (skirmish) and
		// network mode. You could argue that the skirmish game mode should also handle this in advance for consistency
		// regardless of 'CAMPAIGN_FORCE' here.
#if !CAMPAIGN_FORCE
		if (!setLocal && !TheNetwork && d->getBool(TheKey_playerIsHuman))
		{
			setLocalPlayer(p);
			setLocal = true;
		}
#else
		// Do a check for having the 'IsHuman' bool anyway.
		// UPDATE - no longer a good assumption in case of non-computer-marked players that aren't intended for someone to play as.
		// See the '#if CAMPAIGN_FORCE' block below after this loop through sides.
		/*
		if (d->getBool(TheKey_playerIsHuman))
		{
			m_humanPlayerRefs[m_humanPlayerRefsSoftCount] = p;
			++m_humanPlayerRefsSoftCount;
		}
		*/
#endif

		// Set the build list.
		p->setBuildList(TheSidesList->getSideInfo(i)->getBuildList());
		// Build list is attached to player now, so release it from the side info.
		TheSidesList->getSideInfo(i)->releaseBuildList();
	}

#if CAMPAIGN_FORCE
	// After creating the players from the sides above, find the human players for 'm_humanPlayerRefs'.
	if (TheGameInfo != nullptr)
	{
		// Normal case: a skirmish/network-loaded map used in the campaign.
		// First, assume there is the first player, always named "ThePlayer".
		AsciiString targetPlayerName;
		targetPlayerName = "ThePlayer";
		Player* playerRef;
		playerRef = ThePlayerList->findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(targetPlayerName));
		// Some other attempts
		if (playerRef == nullptr)
		{
			targetPlayerName = "Player";
			playerRef = ThePlayerList->findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(targetPlayerName));
		}
		if (playerRef == nullptr)
		{
			targetPlayerName = "player0";
			playerRef = ThePlayerList->findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(targetPlayerName));
		}
		m_humanPlayerRefs[m_humanPlayerRefsSoftCount] = playerRef;
		++m_humanPlayerRefsSoftCount;

		// Now for the remaining possible players 1-7 (#2 to #8 from 1-based counting), search for the expected
		// name: player<1-7>.
		for (int i = 1; i < 8; i++)
		{
			targetPlayerName.format("player%d", i);
			playerRef = ThePlayerList->findPlayerWithNameKey(TheNameKeyGenerator->nameToKey(targetPlayerName));
			if (playerRef != nullptr)
			{
			  // Add to the list and keep searching
				m_humanPlayerRefs[m_humanPlayerRefsSoftCount] = playerRef;
				++m_humanPlayerRefsSoftCount;
			}
			else
			{
				// Not found? Stop searching
				break;
			}
		}
	}
	else
	{
		// Shell map: find the first human-marked player, add to the list of player refs
		for (int i = 0; i < MAX_PLAYER_COUNT; i++)
		{
			Player* playerRef = getNthPlayer(i);
			if (playerRef == nullptr) continue;
			if (playerRef->getPlayerType() == PLAYER_HUMAN) {
				m_humanPlayerRefs[m_humanPlayerRefsSoftCount] = playerRef;
				++m_humanPlayerRefsSoftCount;
				break;
			}
		}
	}
#endif

	if (!setLocal)
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
				setLocal = true;
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

}

//-----------------------------------------------------------------------------
void PlayerList::init()
{
	m_playerCount = 1;
	
#if CAMPAIGN_FORCE
	m_humanPlayerRefsSoftCount = 0;
#endif

	m_players[0]->init(nullptr);

	for (int i = 1; i < MAX_PLAYER_COUNT; i++)
		m_players[i]->init(nullptr);

	// call setLocalPlayer so that becomingLocalPlayer() gets called appropriately
	setLocalPlayer(m_players[0]);

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
void PlayerList::updateTeamStates(void)
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
	if (t)
	{
		//DEBUG_LOG(("assigned obj %08lx to team %s",obj,owner.str()));
	}
	else
	{
		DEBUG_CRASH(("no team or player named %s could be found!", owner.str()));
		t = getNeutralPlayer()->getDefaultTeam();
	}
	return t;
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
void PlayerList::loadPostProcess( void )
{

}

