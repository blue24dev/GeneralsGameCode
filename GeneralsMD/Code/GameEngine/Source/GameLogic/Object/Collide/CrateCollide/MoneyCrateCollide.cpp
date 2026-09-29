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

#if MONEY_AUTO_ADJUSTMENT_SUPPORT
	//MODDD - money cheat check.
	// Question: did this crate come from a renewable income source (supply drop zone), or was it granted for some other
	// reason (ex: spawned by a sold plane or player-issued supply drop gift in the Contra mod) or simply present in the
	// map from the beginning for any player to grab?
	// Could check for providing an upgrade boost (!getMoneyCrateCollideModuleData()->m_upgradeBoost.empty()) since that
	// catches most cases, but this misses 'TechSupplyDropZoneCrate' from the Contra mod that might still want to be
	// adjusted, since oil derrick income rate can be.
	// See 'extra.cpp' for some automatic adjustments that decide whether the money granted by touching a crate will be
	// adjusted by cheats and/or the 'RENEWABLE_MONEY_SOURCE_HALF_EFFECTIVE' setting (whichever is applicable).
	if (getObject()->m_runExtraChecksOnMoneyCrateCollideInObjs_playerIndex != -1)
	{
		Real moneyScalar = 1.0f;
		// First, decide whether this is the same player that caused the crate to be created (ex: supply drop zone owner)
		// as the player that has a unit touching the crate to collect it for them
		Int sourcePlayerIndex = getObject()->m_runExtraChecksOnMoneyCrateCollideInObjs_playerIndex;
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

#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
		// run cheats if the collector is the same as the source, or if this is a tech structure and the source player is neutral/civilian
		// (there's still pre-source-code-release co-op maps using the civilian player as a participating 'player', but
		// this shouldn't cause too much extra weirdness)
		const PlayerIndex collectingPlayerIndex = other->getControllingPlayer()->getPlayerIndex();
		if (
			collectingPlayerIndex == sourcePlayerIndex ||
			(fromTechStructure && ThePlayerList->isPlayerUnaffiliated(ThePlayerList->getNthPlayer(sourcePlayerIndex)))
		)
		{
			// Actually, since another scalar is being applied, get the scalar from this and apply it at the end to reduce
			// round-off error ('APPLY_MONEY_CHEAT' saves to 'money', which includes truncating to an int).
			//APPLY_MONEY_CHEAT(other->getControllingPlayer(), money)
			// 'moneyScalarAdjustmentFilter' comes from breaking down APPLY_MONEY_CHEAT -> getCheatAdjustedMoneyAmount
			moneyScalar *= moneyScalarAdjustmentFilter(other->getControllingPlayer());
		}
#endif

		// Regardless of whether the crate was picked up by the intended player, the half-effective setting still applies
#if RENEWABLE_MONEY_SOURCE_HALF_EFFECTIVE
		if (!fromTechStructure)
		{
			moneyScalar *= 0.5f;
		}
		else
		{
			moneyScalar *= 0.75f;
		}
		money = (UnsignedInt)((Real)money * moneyScalar);
#endif
	}
#endif

	other->getControllingPlayer()->getMoney()->deposit( money );
	other->getControllingPlayer()->getScoreKeeper()->addMoneyEarned( money );

	//Play a crate pickup sound.
	AudioEventRTS soundToPlay = TheAudio->getMiscAudio()->m_crateMoney;
	soundToPlay.setObjectID( other->getID() );
	TheAudio->addAudioEvent(&soundToPlay);

	return TRUE;
}

//------------------------------------------------------------------------------------------------
Int MoneyCrateCollide::getUpgradedSupplyBoost( Object *other ) const
{
	//MODDD - condensed into a utility
	return ::getUpgradedSupplyBoost(other, &getMoneyCrateCollideModuleData()->m_upgradeBoost);
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
