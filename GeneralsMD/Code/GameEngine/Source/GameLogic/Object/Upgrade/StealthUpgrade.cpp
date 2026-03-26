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

// FILE: StealthUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, May 2002
// Desc:	 An upgrade that set's the owner's OBJECT_STATUS_CAN_STEALTH Status
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Xfer.h"
#include "GameLogic/Module/StealthUpgrade.h"
#include "GameLogic/Module/SpawnBehavior.h"
#include "GameLogic/Object.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StealthUpgrade::StealthUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StealthUpgrade::~StealthUpgrade()
{
}

//-------------------------------------------------------------------------------------------------
void StealthUpgrade::upgradeImplementation()
{
	// The logic that does the stealthupdate will notice this and start stealthing
	Object *obj = getObject();
	//MODDD - I think it's better to call the attached 'stealthUpdate' and let it handle how to react to this upgrade
	// instead of doing that here, similar to 'GrantStealthBehavior''s 'stealth->receiveGrant();' call.
	// Could argue that here could even make the exact same call but have a param to avoid the instant reaction.
	// Needing a normal cloak delay from an upgrade turning it on feels more organic vs. clicking to trigger it with a special power like GrantStealthBehavior.
	// For now, keeping those as two separate methods to resemble retail moreso.
 	// ---
	//obj->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_CAN_STEALTH ) );
	// ---
	StealthUpdate* stealth = obj->getStealth();
	if( stealth )
	{
		stealth->receiveUpgrade();
	}
	// ---

	//Grant stealth to spawns if applicable.
	if( obj->isKindOf( KINDOF_SPAWNS_ARE_THE_WEAPONS ) )
	{
		SpawnBehaviorInterface *sbInterface = obj->getSpawnBehaviorInterface();
		if( sbInterface )
		{
			sbInterface->giveSlavesStealthUpgrade( TRUE );
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void StealthUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void StealthUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void StealthUpgrade::loadPostProcess()
{

	// extend base class
	UpgradeModule::loadPostProcess();

}
