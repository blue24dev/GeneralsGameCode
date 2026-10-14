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

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// FILE: HijackerUpdate.cpp
// Author: Mark Lorenzen, July 2002
// Desc:   Allows hijacker to keep with his hijacked vehicle (though hidden) until it dies, then
// to become a hijacker once more
//
/////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Module/EjectPilotDie.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/ExperienceTracker.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
HijackerUpdate::HijackerUpdate( Thing *thing, const ModuleData *moduleData ) : UpdateModule( thing, moduleData )
{
	m_targetID = INVALID_ID;
	setUpdate( FALSE );
	setIsInVehicle( FALSE );
	//MODDD - removed
	//m_wasTargetAirborne = false;
	//m_ejectPos.zero();
//	m_ejectPilotDMI = nullptr;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
HijackerUpdate::~HijackerUpdate()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime HijackerUpdate::update()
{
/// @todo srj use SLEEPY_UPDATE here

	if ( ! m_update) // have not flagged for updating
	{
		return UPDATE_SLEEP_NONE;
	}

	if (m_isInVehicle)
	{
		Object *obj = getObject();

		// If hijacker has hijacked a vehicle, he needs to move along with it...
		//Continually reset position of hijacker to match the position of the target.
		Object *target = getTargetObject();
		if( target )
		{
			// @todo I think we should test for ! IsEffectivelyDead() as well, here
			obj->setPosition( target->getPosition() );
			//MODDD - removed
			//m_wasTargetAirborne = target->isSignificantlyAboveTerrain();
			//m_ejectPos = *target->getPosition();

			// So, if while I am driving this American war vehicle, I gain skill points, I get to keep them when I wreck the vehicle
			ExperienceTracker *targetExp = target->getExperienceTracker();
			ExperienceTracker *jackerExp = obj->getExperienceTracker();
			if ( targetExp && jackerExp )
			{
				//MODDD - per a similar change in ConvertToHijackedVehicleCrateCollide.cpp (copy hijacker veterancy to victim),
				// adding a is-trainable check and only transferring a higher level in the target to the hijacker
				/*
				VeterancyLevel highestLevel = MAX(targetExp->getVeterancyLevel(),jackerExp->getVeterancyLevel());
				jackerExp->setVeterancyLevel( highestLevel );
				targetExp->setVeterancyLevel( highestLevel );
				*/
				if ( targetExp->isTrainable() && jackerExp->isTrainable() )
				{
					VeterancyLevel targetVetLevel = targetExp->getVeterancyLevel();
					if ( targetVetLevel > jackerExp->getVeterancyLevel() )
					{
						jackerExp->setVeterancyLevel( targetVetLevel, FALSE );
					}
				}
			}

		}
		else // the car we have been "driving" is dead now, and has safely ejected us
		{
			//MODDD - in the case a hijacker gets a helicopter with 'HelicopterSlowDeathBehavior''s 'OCLEjectPilot' instead
			// of 'EjectPilotDie' (the former will take precedence anyway if found), the hijacker would have already found
			// the 'HelicopterSlowDeathBehavior' and slipped himself in so it knows to re-show the hijacker in place of the
			// normally ejected pilot instead. Not sure why something like this wasn't done for 'EjectPilotDie' to begin with.
			// Done for both cases - existing block handling hijacker re-drawing/tangibility moved to a static
			// 'EjectPilotDie::ejectHijacker' similar to 'EjectPilotDie::ejectPilot'.
			// This call will be left to 'EjectPilotDie' in the hijacked vehicle.

			setTargetObject( nullptr );
			setIsInVehicle( FALSE );
			setUpdate( FALSE );
			//MODDD - removed
			//m_wasTargetAirborne = false;

		}

	}
	else	// not in vehicle
	{
		//MODDD - removed
		//m_wasTargetAirborne = false;
	}

	return UPDATE_SLEEP_NONE;
}



void HijackerUpdate::setTargetObject( const Object *object )
{
  if( object )
  {
    m_targetID = object->getID();

		// here we also test the target to see whether it ejects pilots
		// when it dies... if so, stores a pointer to that diemoduleinterface
		// nullptr if not...

//		BehaviorModule **dmi = nullptr;
//		for( dmi = object->getBehaviorModules(); *dmi; ++dmi )
//		{
//			m_ejectPilotDMI = (*dmi)->getEjectPilotDieInterface();
//			if( m_ejectPilotDMI )
//				return;
//		}  // end for dmi
  }
	else
	{
		m_targetID = INVALID_ID;
//		m_ejectPilotDMI = nullptr;
	}

}

Object* HijackerUpdate::getTargetObject() const
{
  if( m_targetID != INVALID_ID )
  {
    return TheGameLogic->findObjectByID( m_targetID );
  }
  return nullptr;
}

//MODDD
const AsciiString& HijackerUpdate::getParachuteName() const
{
	return getHijackerUpdateModuleData()->m_parachuteName;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void HijackerUpdate::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void HijackerUpdate::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// target ID
	xfer->xferObjectID( &m_targetID );

	// eject pos
	//MODDD - removed
	//xfer->xferCoord3D( &m_ejectPos );

	// update
	xfer->xferBool( &m_update );

	// is in vehicle
	xfer->xferBool( &m_isInVehicle );

	// was target airborne
	//MODDD - removed
	//xfer->xferBool( &m_wasTargetAirborne );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void HijackerUpdate::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

	// set the target object, this will also tie up the m_ejectPilotDMI pointer
	Object *obj = TheGameLogic->findObjectByID( m_targetID );
	setTargetObject( obj );

}
