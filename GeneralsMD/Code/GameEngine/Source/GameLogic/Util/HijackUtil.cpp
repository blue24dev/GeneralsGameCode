//MODDD - new file

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameLogic/HijackUtil.h"
#include "Common/GameAudio.h"
#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/EjectPilotDie.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/HelicopterSlowDeathUpdate.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ObjectCreationList/GenericObjectCreationNugget.h"

namespace HijackUtil
{

//MODDD - utility to kill the existing hijacker if there is one - use this before changing a module's
// "m_hijackerID" so an existing intangible one waiting to come out of this vehicle doesn't remain
// stuck in limbo
void deleteExistingHijacker(ObjectID hijackerID)
{
	if (hijackerID != INVALID_ID)
	{
		Object* existingHijacker = TheGameLogic->findObjectByID(hijackerID);
		if (existingHijacker != nullptr)
		{
			HijackerUpdate* hijackerUpdate = existingHijacker->getHijackerUpdate();
			hijackerUpdate->setTargetObject( nullptr );
			//MODDD - TODO - should the hijacker be made tangible before being killed? not sure
			existingHijacker->kill();
		}
	}
}

//MODDD - new, to handle re-drawing/re-enabling-tangibility for the emerging hijacker who took this vehicle instead of
// a pilot. Also takes the OCL that would've been used to spawn the pilot in case of a force that's needed to keep
// the hijacker alive (ex: an upwards force on a mid-air hijacker parachuting - missing that causes problems when
// leaving a mid-air helicopter).
// (note: 'hijacker' param was originally named 'obj' in the script as-is)
void ejectHijacker(Object* hijacker, Object* dyingObject, const ObjectCreationList* oclReference)
{
	//MODDD - see 'ejectPilot'
	if (!hijacker || !dyingObject)
		return;

	//MODDD - notify the HijackerUpdate so it doesn't try to force our position to say, a crashing helicopter even after
	// we've ejected
	HijackerUpdate *hijackerUpdate = hijacker->getHijackerUpdate();
	hijackerUpdate->setTargetObject( nullptr );

	//MODDD - I don't see why not
	// Note - the pilot-creation-OCL this is replacing would give up if the 'primary' object sent were null.
	// Assuming that should never be the case here. And the hijacker's position is kept in-sync with the hijacked
	// vehicle every frame leading up to this anyway.
	//MODDD - also, recording these in advance
	Coord3D ejectPos;
	Bool wasTargetAirborne;
	// Hm, nevermind considering 'dyingObject' is null here - see 'ejectPilot' that gives up early in that case.
	//if (dyingObject != nullptr)
	{
		ejectPos = *dyingObject->getPosition();
		hijacker->setPosition(&ejectPos);
		wasTargetAirborne = dyingObject->isSignificantlyAboveTerrain();
	}
	/*
	else
	{
		// *shrug*
		ejectPos = *hijacker->getPosition();
		wasTargetAirborne = hijacker->isSignificantlyAboveTerrain();
	}
	*/

	//THIS BLOCK RESTORES HIJACKER TO PARTITION MANAGER AND UNHIDES HIM
	ThePartitionManager->registerObject( hijacker );

	if( hijacker->getDrawable() )
	{
		// so it is time to unhide ourselves and be a pedestrian hijacker again
		hijacker->getDrawable()->setDrawableHidden( false );
	}

	// We won't come back here until and unless we have hijacked another vehicle
	hijacker->clearStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );

	AIUpdateInterface* ai = hijacker->getAIUpdateInterface();
	if ( ai )
	{
		ai->aiIdle( CMD_FROM_AI );
	}
	
	//MODDD - member var 'm_wasTargetAirborne' to a function-local one
	if (wasTargetAirborne)
	{
		//MODDD - Get the parachute to use from the hijacker's 'HijackUpdate' module, through the Object reference to the hijacker now.
		//const ThingTemplate* putInContainerTmpl = TheThingFactory->findTemplate(getHijackerUpdateModuleData()->m_parachuteName);
		// ---
		HijackerUpdate *hijackerUpdate = hijacker->getHijackerUpdate();
		const AsciiString& parachuteName = hijackerUpdate->getParachuteName();
		const ThingTemplate* putInContainerTmpl = TheThingFactory->findTemplate(parachuteName);
		// ---

		DEBUG_ASSERTCRASH(putInContainerTmpl,("DeliverPayload: PutInContainer %s not found!",parachuteName.str()));
		if (putInContainerTmpl)
		{
			Object* container = TheThingFactory->newObject( putInContainerTmpl, hijacker->getTeam() );
			//MODDD - member var 'm_ejectPos' to a function-local one
			container->setPosition(&ejectPos);
			if (container->getContain()->isValidContainerFor(hijacker, true))
			{
				container->getContain()->addToContain(hijacker);
			}
			else
			{
				DEBUG_CRASH(("DeliverPayload: PutInContainer %s is full, or not valid for the payload!",parachuteName.str()));
			}
		}

	}

	//MODDD - check for a force to apply from the OCL supplied
	oclReference->getNuggetList();
	const ObjectCreationNuggetVector* nuggetList = oclReference->getNuggetList();
	ObjectCreationNuggetVector::const_iterator itN;
	for (itN = nuggetList->begin(); itN != nuggetList->end(); ++itN)
	{
		ObjectCreationNugget* nugget = *itN;
		if (nugget->getTypeID() == OCL_NUGGET_TYPE_CREATE_OBJECT)
		{
			const GenericObjectCreationNugget* genericNugget = (const GenericObjectCreationNugget*)nugget;
			genericNugget->applyDisposition(hijacker, dyingObject->getPosition(), nullptr, 0.0, dyingObject);
		}
	}

	//MODDD - I suppose the eject sound is fitting (the voice is probably fine-tailored to the intended pilot unit).
	//MODDD - TODO - If there's some way to reach for the hijacker's evil laugh (on-hijack voice), that could work in place
	// of 'VoiceEject' from 'EjectPilotDie' in the INI - providing a new "HijackedVoiceEject" field would be much better
	// though.
	AudioEventRTS soundEject = *(dyingObject->getTemplate()->getPerUnitSound("SoundEject"));
	soundEject.setPosition( dyingObject->getPosition() );
	TheAudio->addAudioEvent(&soundEject);

	//MODDD - finally, remove the association with the object so it doesn't try to kill the hijacker when it's deleted
	dyingObject->rawSetHijackerInMeID(INVALID_ID);
	dyingObject->setModuleTagNameKeyToEjectHijacker(NAMEKEY_INVALID);
}

//MODDD - Check to see if there is some module that supports pilot ejection - if this returns 'true', it can be assumed
// something was found and the hijacker is attached to one of the supporting modules. If this returns 'false', the caller
// should delete the hijacker since there won't be an opportunity for thie hijacker to be released.
// Existing comments are referring to script around where this originally was.
Bool checkForTargetEjectSupport(Object* hijacker, Object* other)
{
	//MODDD - moving this from below so we can terminate early if this object is missing a 'HijackerUpdate' module.
	// Without this, being 'stored' in a vehicle (invisible / uncollidable, keep in-sync with its position) won't be able
	// to work. Some odd behavior such as enemies attacking the invisible hijacker forever at the same location he was
	// at when a vehicle was hijacked has been observed in this case, though it could just be the 'obj->setStatus' call
	// in the original 'if( hijackerUpdate )' block further down not running.
	// The point is, I doubt preserving the unit should go any further if 'HijackerUpdate' is missing.
	//MODDD - TODO - If a mod wants to support a unit that can hijack but uses a different unit for ejection systems
	// (ex: a biker or flying hijacker from Contra hijacks something -> it dies -> spawns infantry hijacker), an additional
	// field for what template to spawn from the hijacked vehicle if dead in some hijacker-related module should suffice.
	// Ex: HijackerUpdate::m_templateOnTargetEject, "TemplateOnTargetEject" in the INI.
	// Perhaps even whether to use an ejection system at all even if found: "AllowEject" defaulting to "Yes"/true.
	// ---
	HijackerUpdate *hijackerUpdate = hijacker->getHijackerUpdate();
	if ( !hijackerUpdate )
	{
		return FALSE;
	}
	// ---

	//MODDD - replaced
	//Bool targetCanEject = FALSE;
	Bool foundModuleForHijacker = false;

	//EjectPilotDie* ejectPilotDie_chosen = nullptr;

	//MODDD - Turns out it's possible for there to be multiple 'EjectPilotDie' modules on the same Object in the INI.
	// (example: 'Shockwave' mod).
	// Instead of stopping at the first 'EjectPilotDie' module encountered, record the first one but disable all the other
	// ones so that only the one we pick to associate with the hijacker ends up spawning anything.
	// Doing anything else to the returned 'EjectPilotDie' will be left to the caller.

	//MODDD - But first - check to see if there's a 
	static NameKeyType key_HelicopterSlowDeathBehavior = NAMEKEY( "HelicopterSlowDeathBehavior" );
	HelicopterSlowDeathBehavior *hsdb = (HelicopterSlowDeathBehavior*)other->findUpdateModule( key_HelicopterSlowDeathBehavior );
	if (hsdb != nullptr && hsdb->hasPilotEjectSupport())
	{
		foundModuleForHijacker = true;
		other->setModuleTagNameKeyToEjectHijacker(hsdb->getModuleTagNameKey());
	}

	BehaviorModule **dmi = nullptr;
	for( dmi = other->getBehaviorModules(); *dmi; ++dmi )
	{
		//MODDD - save a reference to this - much deeper checks with this
		EjectPilotDie* ejectPilotDieTest = (*dmi)->getEjectPilotDie();
		if(ejectPilotDieTest != nullptr)
		{
			if (!foundModuleForHijacker)
			{
				// first EjectPilotDie module: choose that
				//ejectPilotDie_chosen = ejectPilotDieTest;
				foundModuleForHijacker = true;
				other->setModuleTagNameKeyToEjectHijacker(ejectPilotDieTest->getModuleTagNameKey());
			}
			else
			{
				// subsequent EjectPilotDie modules are turned off
				// (no longer necessary - each module will check to see if it's the one ejecting the hijacker by a confirmation
				// with the object it belongs to)
			}
			//targetCanEject = TRUE;
			//break;
		}
	}

	//if ( ! targetCanEject )
	if (!foundModuleForHijacker)
	{
		return FALSE;
	}
	
	other->setHijackerInMeID(hijacker->getID());

	//MODDD - the remaining block is mostly from the rest of 'ConvertToHijackedVehicleCrateCollide::executeCrateBehavior'.
	hijackerUpdate->setTargetObject( other );

	// flag bits so hijacker won't be selectible or collideable
	//while within the vehicle
	hijacker->setStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );

	// THIS BLOCK HIDES THE HIJACKER AND REMOVES HIM FROM PARTITION MANAGER
	// remove object from its group (if any)
	hijacker->leaveGroup();
	AIUpdateInterface* ai = hijacker->getAIUpdateInterface();
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
	ThePartitionManager->unRegisterObject( hijacker );

	// hide the drawable associated with rider
	if( hijacker->getDrawable() )
		hijacker->getDrawable()->setDrawableHidden( true );

	return TRUE;
}

//MODDD - if there is a hijacker anywhere in this object, remove/kill it.
// Use this when a vehicle is depiloted so the associated hijacker should no longer exist.
void clearHijackerInObject(Object* obj)
{
	// includes a check to kill the hijacker if present, so this works out nicely
	obj->setHijackerInMeID(INVALID_ID);
	obj->setModuleTagNameKeyToEjectHijacker(NAMEKEY_INVALID);
}

}// namespace HijackUtil
