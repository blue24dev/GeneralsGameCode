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

// FILE: MoneyCrateCollide.cpp ///////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, March 2002
// Desc:   A crate that gives x money to the collider
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#include "Common/AudioEventRTS.h"
#include "Common/MiscAudio.h"
#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/MoneyCrateCollide.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MoneyCrateCollide::MoneyCrateCollide( Thing *thing, const ModuleData* moduleData ) : CrateCollide( thing, moduleData )
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MoneyCrateCollide::~MoneyCrateCollide()
{

}

#include "Common/PlayerList.h"
//-------------------------------------------------------------------------------------------------
Bool MoneyCrateCollide::executeCrateBehavior( Object *other )
{
	UnsignedInt money = getMoneyCrateCollideModuleData()->m_moneyProvided;

	money += getUpgradedSupplyBoost(other);

	//MODDD - new vars
	Real moneyPreScalar = 1.0f;
	// (not involved without necessarily being 'mine' for now)
	//Bool fromARenewableSource = false;
	Bool fromMyRenewableSource = false;
	MoneySourceType moneySource;
	Bool runMajorCheats;

	//MODDD - money cheat check.
	// Question: did this crate come from a renewable income source (supply drop zone), or was it granted for some other
	// reason (ex: spawned by a sold plane or player-issued supply drop gift in the Contra mod) or simply present in the
	// map from the beginning for any player to grab?
	// Could check for providing an upgrade boost (!getMoneyCrateCollideModuleData()->m_upgradeBoost.empty()) since that
	// catches most cases, but this misses 'TechSupplyDropZoneCrate' from the Contra mod that might still want to be
	// adjusted, since oil derrick income rate can be.
	// See 'extra.cpp' for some automatic adjustments that decide whether the money granted by touching a crate will be
	// adjusted by cheats and/or the 'RENEWABLE_MONEY_SOURCE_HALF_EFFECTIVE' setting (whichever is applicable).
	if (getObject()->m_OCLSourcePlayerIndex != -1)
	{
		// this much is known
		//fromARenewableSource = true;

		// First, decide whether this is the same player that caused the crate to be created (ex: supply drop zone owner)
		// as the player that has a unit touching the crate to collect it for them
		Int sourcePlayerIndex = getObject()->m_OCLSourcePlayerIndex;
		Bool fromTechStructure;
		// Note that 'playerIndex' can have a flag baked into it indicating whether this crate was produced by a neutral tech
		// structure, captured or not (in case of non-capturable flat areas, like neutral supply drop zones in several mods).
		if (sourcePlayerIndex & (1 << 31))
		{
			sourcePlayerIndex &= ~(1 << 31);
			fromTechStructure = true;
		}
		else
		{
			fromTechStructure = false;
		}
		
		// Regardless of whether the crate was picked up by the intended player, the half-effective setting still applies
#if RENEWABLE_MONEY_SOURCE_HALF_EFFECTIVE
		if (!fromTechStructure)
		{
			moneyPreScalar = 0.5f;
		}
		else
		{
			moneyPreScalar = 0.75f;
		}
#endif

		const PlayerIndex collectingPlayerIndex = other->getControllingPlayer()->getPlayerIndex();
		fromMyRenewableSource = (
			collectingPlayerIndex == sourcePlayerIndex ||
			(fromTechStructure && ThePlayerList->isPlayerUnaffiliated(ThePlayerList->getNthPlayer(sourcePlayerIndex)))
		);
	}

	//MODDD - decide the 'moneySource'
	if (fromMyRenewableSource)
	{
		// from my supply drop zone means the highest extent of scalar cheats applies
		moneySource = MONEYSOURCE_RENEWABLE;
		runMajorCheats = true;
	}
	else
	{
		// stolen from someone else's supply drop zone or simply found
		moneySource = MONEYSOURCE_GATHERED;
		runMajorCheats = false;
	}

	//MODDD - replacing 'deposit'/'addMoneyEarned' with this
	// Provide 'moneyPreScalar' to be applied in addition to the internal scalar decided by the presence of cheats or not.
	// This will still correctly determine how much money is produced by scalar cheats if applicable.
	RECEIVE_MONEY_FILTER_EX(other->getControllingPlayer(), money, runMajorCheats, moneyPreScalar, moneySource)

	//Play a crate pickup sound.
	AudioEventRTS soundToPlay = TheAudio->getMiscAudio()->m_crateMoney;
	soundToPlay.setObjectID( other->getID() );
	TheAudio->addAudioEvent(&soundToPlay);

	return TRUE;
}

//MODDD - new, to add a check for already being destroyed this frame
Bool MoneyCrateCollide::isValidToExecute( const Object *other ) const
{
	//MODDD - needed when the crate's been destroyed by something else in the same frame to avoid double-dipping.
	// ex: money crates now that they can have an execution triggered the same frame they were going to collide with
	// something anyway it seems?
	if (getObject()->isDestroyed())
	{
		return FALSE;
	}

	return CrateCollide::isValidToExecute(other);
}

//------------------------------------------------------------------------------------------------
Int MoneyCrateCollide::getUpgradedSupplyBoost( Object *other ) const
{
	//MODDD - condensed into a utility
	return rts_extra::getUpgradedSupplyBoost(other, &getMoneyCrateCollideModuleData()->m_upgradeBoost);
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void MoneyCrateCollide::crc( Xfer *xfer )
{

	// extend base class
	CrateCollide::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void MoneyCrateCollide::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	CrateCollide::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void MoneyCrateCollide::loadPostProcess()
{

	// extend base class
	CrateCollide::loadPostProcess();

}
