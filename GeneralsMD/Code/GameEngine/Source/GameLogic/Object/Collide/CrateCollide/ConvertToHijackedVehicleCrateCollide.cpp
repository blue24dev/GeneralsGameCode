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
// FILE: ConvertToHijackedVehicleCrateCollide.cpp
// Author: Mark Lorenzen, July 2002
// Desc:   A crate (actually a terrorist - mobile crate) that makes the target vehicle switch
//				 sides, and kills its driver
//	@todo	 Needs to set the science of that vehicle (dozer) so still can build same stuff as always
//
///////////////////////////////////////////////////////////////////////////////////////////////////



// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Radar.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"
#include "GameClient/Eva.h"
#include "GameClient/InGameUI.h"  // useful for printing quick debug strings when we need to

#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/ConvertToHijackedVehicleCrateCollide.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/Module/DozerAIUpdate.h"


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ConvertToHijackedVehicleCrateCollide::ConvertToHijackedVehicleCrateCollide( Thing *thing, const ModuleData* moduleData ) : CrateCollide( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ConvertToHijackedVehicleCrateCollide::~ConvertToHijackedVehicleCrateCollide()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ConvertToHijackedVehicleCrateCollide::isValidToExecute( const Object *other ) const
{
	if( !CrateCollide::isValidToExecute(other) )
	{
		return FALSE;
	}

	if( other->isEffectivelyDead() )
	{
		return FALSE;// can't hijack a dead vehicle
	}

	if( other->isKindOf( KINDOF_IMMUNE_TO_CAPTURE ) )
	{
		return FALSE; //Kris: Patch 1.03 -- Prevent hijackers from being able to hijack battle buses.
	}

	if( other->isKindOf( KINDOF_AIRCRAFT ) || other->isKindOf( KINDOF_BOAT ) )
	{
		//Can't hijack planes and boats!
		return FALSE;
	}

	if( other->isKindOf( KINDOF_DRONE ) )
	{
		//Can't hijack drones!
		return FALSE;
	}

	if( other->getStatusBits().test( OBJECT_STATUS_HIJACKED ) )
	{
		return FALSE;// oops, sorry, I'll jack the next one.
	}

	Relationship r = getObject()->getRelationship( other );
	//Only hijack enemy objects
	if( r != ENEMIES )
	{
		return FALSE;
	}

	//MODDD - disagree with this point, the passengers can just get kicked out then - think jarmen kell's pilot snipe & immediately taking the vehicle
	/*
	if( other->isKindOf( KINDOF_TRANSPORT ) )
	{
		//Kris: Allow empty transports to be hijacked.
		if( other->getContain() && other->getContain()->getContainCount() > 0 )
		{
			return FALSE;// dustin sez: do not jack vehicles that may carry hostile passengers
		}
	}
	*/

	//MODDD - redundant, already checked for above
	/*
	//Kris: Make sure you can't hijack any aircraft (or hijack-enter).
	if( other->isKindOf( KINDOF_AIRCRAFT ) )
	{
		return FALSE;
	}
	*/

	//VeterancyLevel veterancyLevel = other->getVeterancyLevel();
	//if( veterancyLevel >= LEVEL_ELITE )
	//{
	//	return FALSE;
	//}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ConvertToHijackedVehicleCrateCollide::executeCrateBehavior( Object *other )
{
	//Check to make sure that the other object is also the goal object in the AIUpdateInterface
	//in order to prevent an unintentional conversion simply by having the terrorist walk too close
	//to it.
	//Assume ai is valid because CrateCollide::isValidToExecute(other) checks it.
	Object *obj = getObject();
	AIUpdateInterface* ai = obj->getAIUpdateInterface();
	if (ai && ai->getGoalObject() != other)
	{
		return false;
	}

	TheRadar->tryInfiltrationEvent( other );

	//Before the actual defection takes place, play the "vehicle stolen" EVA
	//event if the local player is the victim!
	if( other->isLocallyViewed() )
	{
		TheEva->setShouldPlay( EVA_VehicleStolen );
	}

	other->setTeam( obj->getControllingPlayer()->getDefaultTeam() );
	
	//MODDD - since this skips the kick-out from calling 'Object::setTeam' instead of 'Object::defect', handle that here if needed.
	// And, going off 'Object::defect', the kick-out happens after calling 'setTeam'.
	// ---
	ContainModuleInterface *ct = getContain();
	if (ct && ct->isKickOutOnCapture())
	{
		ct->removeAllContained( TRUE );
	}
	// ---

	other->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_HIJACKED ) );// I claim this car in the name of the GLA

	AIUpdateInterface* targetAI = other->getAIUpdateInterface();
	targetAI->aiMoveToPosition( other->getPosition(), CMD_FROM_AI );
	targetAI->aiIdle( CMD_FROM_AI );


	//Just in case this target is a dozer, lets make him stop al his dozer tasks, like building and repairing,
	//So the previous owner does not benefit from these tasks
	DozerAIInterface * dozerAI = targetAI->getDozerAIInterface();
	if ( dozerAI )
	{
		dozerAI->cancelAllTasks();
	}

	AudioEventRTS hijackEvent( "HijackDriver", obj->getID() );
	TheAudio->addAudioEvent( &hijackEvent );

	//In order to make things easier for the designers, we are going to transfer the hijacker's name
	//to the car... so the designer can control the car with their scripts.
	TheScriptEngine->transferObjectName( obj->getName(), other );

	ExperienceTracker *targetExp = other->getExperienceTracker();
	ExperienceTracker *jackerExp = obj->getExperienceTracker();
	if ( targetExp && jackerExp )
	{
		//MODDD - as-is, a hijacked vehicle uses the highest veterancy level between the hijacker and victim unit.
		// Changing to replace the victim's veterancy with the hijacker's instead.
		// Also, require the target to be trainable for a veterancy transfer.
		/*
		VeterancyLevel highestLevel = MAX(targetExp->getVeterancyLevel(),jackerExp->getVeterancyLevel());
		jackerExp->setVeterancyLevel( highestLevel, FALSE );
		targetExp->setVeterancyLevel( highestLevel, FALSE );
		*/
		// Note that it doesn't matter if the hijacker is trainable or not - if the hijacker has 0 veterancy and the victim
		// is trainable, the victim takes veterancy #0.
		if ( targetExp->isTrainable() )
		{
			targetExp->setVeterancyLevel( jackerExp->getVeterancyLevel(), FALSE );
		}
	}

	//MODDD - moving this from below so we can terminate early if this object is missing a 'HijackerUpdate' module.
	// Without this, being 'stored' in a vehicle (invisible / uncollidable, keep in-sync with its position) won't be able
	// to work. Some odd behavior such as enemies attacking the invisible hijacker forever at the same location he was
	// at when a vehicle was hijacked has been observed in this case, though it could just be the 'obj->setStatus' call
	// in the original 'if( hijackerUpdate )' block further down not running.
	// The point is, I doubt preserving the unit should go any further if 'HijackerUpdate' is missing.
	//MODDD - TODO - If a mod wants to support a unit that can hijack but uses a different unit for ejection systems
	// (ex: a biker or flying hijacker from Contra hijacks something -> it dies -> spawns infantry hijacker), an additional
	// field for what the hijacker should spawn in some hijacker-related module should suffice.
	// ---
	static NameKeyType key_HijackerUpdate = NAMEKEY( "HijackerUpdate" );
	HijackerUpdate *hijackerUpdate = (HijackerUpdate*)obj->findUpdateModule( key_HijackerUpdate );
	if ( !hijackerUpdate )
	{
		TheGameLogic->destroyObject( obj );
		return TRUE;
	}
	// ---
	
	Bool targetCanEject = FALSE;
	BehaviorModule **dmi = nullptr;
	for( dmi = other->getBehaviorModules(); *dmi; ++dmi )
	{
		if( (*dmi)->getEjectPilotDieInterface() )
		{
			targetCanEject = TRUE;
			break;
		}
	}

	if ( ! targetCanEject )
	{
		TheGameLogic->destroyObject( obj );
		return TRUE;
	}

	//MODDD - moved to above, condition no longer needed here either
	/*
	// I we have made it this far, we are going to ride in this vehicle for a while
	// get the name of the hijackerupdate
	static NameKeyType key_HijackerUpdate = NAMEKEY( "HijackerUpdate" );
	HijackerUpdate *hijackerUpdate = (HijackerUpdate*)obj->findUpdateModule( key_HijackerUpdate );
	if( hijackerUpdate )
	*/
	{
		hijackerUpdate->setTargetObject( other );
		hijackerUpdate->setIsInVehicle( TRUE );
		hijackerUpdate->setUpdate( TRUE );

		// flag bits so hijacker won't be selectible or collideable
		//while within the vehicle
		obj->setStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );
	}

	// THIS BLOCK HIDES THE HIJACKER AND REMOVES HIM FROM PARTITION MANAGER
	// remove object from its group (if any)
	obj->leaveGroup();
	if( ai )
	{
		//By setting him to idle, we will prevent him from entering the target after this gets called.
		ai->aiIdle( CMD_FROM_AI );
	}

	//This is kinda special... we will endow our new ride with our vision and shroud range, since we are driving
	//MODDD - disagree with this. Keep the victim unit's own stats or else its enemy acquisition behavior may be affected - is that really necessary?
	/*
	other->setVisionRange(getObject()->getVisionRange());
	other->setShroudClearingRange(getObject()->getShroudClearingRange());
	*/

	// remove rider from partition manager
	ThePartitionManager->unRegisterObject( obj );

	// hide the drawable associated with rider
	if( obj->getDrawable() )
		obj->getDrawable()->setDrawableHidden( true );

	// By returning FALSE, we will not remove the object (Hijacker)
	return FALSE;
//	return TRUE;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ConvertToHijackedVehicleCrateCollide::crc( Xfer *xfer )
{

	// extend base class
	CrateCollide::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ConvertToHijackedVehicleCrateCollide::xfer( Xfer *xfer )
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
void ConvertToHijackedVehicleCrateCollide::loadPostProcess()
{

	// extend base class
	CrateCollide::loadPostProcess();

}
