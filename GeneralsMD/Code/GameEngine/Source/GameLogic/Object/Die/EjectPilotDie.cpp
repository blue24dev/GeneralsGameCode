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

// FILE: EjectPilotDie.cpp ///////////////////////////////////////////////////////////////////////////
// Author: Steven Johnson, April 2002
// Desc:   Create an object upon this object's death
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

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

//MODDD
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/HelicopterSlowDeathUpdate.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ObjectCreationList/GenericObjectCreationNugget.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
EjectPilotDieModuleData::EjectPilotDieModuleData() :
	m_oclInAir(nullptr),
	m_oclOnGround(nullptr),
	m_invulnerableTime(0)
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void EjectPilotDieModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  DieModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "AirCreationList",		INI::parseObjectCreationList,		nullptr, offsetof( EjectPilotDieModuleData, m_oclInAir ) },
		{ "GroundCreationList",		INI::parseObjectCreationList,		nullptr, offsetof( EjectPilotDieModuleData, m_oclOnGround ) },
		{	"InvulnerableTime",  INI::parseDurationUnsignedInt, nullptr, offsetof(EjectPilotDieModuleData, m_invulnerableTime ) },

		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
EjectPilotDie::EjectPilotDie( Thing *thing, const ModuleData* moduleData ) : DieModule( thing, moduleData )
{
	//MODDD - for retail behavior, assume this is enabled until told otherwise
	m_enabled = TRUE;
	//MODDD - init
	m_hijackerID = INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
EjectPilotDie::~EjectPilotDie()
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//MODDD - disarming mines gives experience. Removed 'const' on 'dyingObject'
/*static*/ void EjectPilotDie::ejectPilot(const ObjectCreationList* ocl, Object* dyingObject, const Object* damageDealer)
{
	if (!ocl || !dyingObject)
		return;	// it's OK for damageDealer to be null

	ObjectCreationList::create(ocl, dyingObject, damageDealer);

	AudioEventRTS voiceEject = *(dyingObject->getTemplate()->getPerUnitSound("VoiceEject"));
	voiceEject.setPosition( dyingObject->getPosition() );
	voiceEject.setPlayerIndex( dyingObject->getControllingPlayer()->getPlayerIndex() );
	TheAudio->addAudioEvent(&voiceEject);

	AudioEventRTS soundEject = *(dyingObject->getTemplate()->getPerUnitSound("SoundEject"));
	soundEject.setPosition( dyingObject->getPosition() );
	TheAudio->addAudioEvent(&soundEject);
}

//MODDD - new, to handle re-drawing/re-enabling-tangibility for the emerging hijacker who took this vehicle instead of
// a pilot. Also takes the OCL that would've been used to spawn the pilot in case of a force that's needed to keep
// the hijacker alive (ex: an upwards force on a mid-air hijacker parachuting - missing that causes problems when
// leaving a mid-air helicopter).
void EjectPilotDie::ejectHijacker(Object* hijacker, Object* dyingObject, const ObjectCreationList* oclReference)
{
	//MODDD - see 'ejectPilot'
	if (!hijacker || !dyingObject)
		return;

	// (to keep retail code happy without renaming)
	Object* obj = hijacker;

	//MODDD - notify the HijackerUpdate so it doesn't try to force our position to say, a crashing helicopter even after
	// we've ejected
	HijackerUpdate *hijackerUpdate = hijacker->getHijackerUpdate();
	hijackerUpdate->setTargetObject( nullptr );
	hijackerUpdate->setIsInVehicle( FALSE );
	hijackerUpdate->setUpdate( FALSE );

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
		obj->setPosition(&ejectPos);
		wasTargetAirborne = dyingObject->isSignificantlyAboveTerrain();
	}
	/*
	else
	{
		// *shrug*
		ejectPos = *obj->getPosition();
		wasTargetAirborne = obj->isSignificantlyAboveTerrain();
	}
	*/

	//THIS BLOCK RESTORES HIJACKER TO PARTITION MANAGER AND UNHIDES HIM
	ThePartitionManager->registerObject( obj );

	if( obj->getDrawable() )
	{
		// so it is time to unhide ourselves and be a pedestrian hijacker again
		obj->getDrawable()->setDrawableHidden( false );
	}

	// We won't come back here until and unless we have hijacked another vehicle
	obj->clearStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );

	AIUpdateInterface* ai = obj->getAIUpdateInterface();
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
		HijackerUpdate *hijackerUpdate = obj->getHijackerUpdate();
		const AsciiString& parachuteName = hijackerUpdate->getParachuteName();
		const ThingTemplate* putInContainerTmpl = TheThingFactory->findTemplate(parachuteName);
		// ---

		DEBUG_ASSERTCRASH(putInContainerTmpl,("DeliverPayload: PutInContainer %s not found!",parachuteName.str()));
		if (putInContainerTmpl)
		{
			Object* container = TheThingFactory->newObject( putInContainerTmpl, obj->getTeam() );
			//MODDD - member var 'm_ejectPos' to a function-local one
			container->setPosition(&ejectPos);
			if (container->getContain()->isValidContainerFor(obj, true))
			{
				container->getContain()->addToContain(obj);
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
}

//MODDD - Check to see if there is some module that supports pilot ejection - if this returns 'true', it can be assumed
// something was found and the hijacker is attached to one of the supporting modules. If this returns 'false', the caller
// should delete the hijacker since there won't be an opportunity for thie hijacker to be released.
// Existing comments are referring to script around where this originally was.
Bool EjectPilotDie::checkForTargetEjectSupport(Object* obj, Object* other)
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
	HijackerUpdate *hijackerUpdate = obj->getHijackerUpdate();
	if ( !hijackerUpdate )
	{
		return FALSE;
	}
	// ---

	//MODDD - replaced
	//Bool targetCanEject = FALSE;
	Bool foundModuleForHijacker = false;

	EjectPilotDie* ejectPilotDie_chosen = nullptr;

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
		hsdb->setHijacker(obj);
		foundModuleForHijacker = true;
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
				ejectPilotDie_chosen = ejectPilotDieTest;
				ejectPilotDie_chosen->setHijacker(obj);
				foundModuleForHijacker = true;
			}
			else
			{
				// subsequent EjectPilotDie modules are turned off
				ejectPilotDieTest->setEnabled(FALSE);
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

	// decided to hijack 'other' - let 'hijackerUpdate' know
	hijackerUpdate->setTargetObject( other );
	hijackerUpdate->setIsInVehicle( TRUE );
	hijackerUpdate->setUpdate( TRUE );
	return TRUE;
}

//MODDD - TODO - if there is a hijacker anywhere in this object, remove/kill it.
// Use this when a vehicle is depiloted so the associated hijacker should no longer exist.
/*
void EjectPilotDie::clearHijackerInObject()
{

}
*/

//MODDD - utility to kill the existing hijacker if there is one - use this before changing a module's
// "m_hijackerID" so an existing intangible one waiting to come out of this vehicle doesn't remain
// stuck in limbo
void EjectPilotDie::checkCleanHijacker(ObjectID existingHijackerID)
{
	if (existingHijackerID != INVALID_ID)
	{
		Object* existingHijacker = TheGameLogic->findObjectByID(existingHijackerID);
		if (existingHijacker != nullptr)
		{
			HijackerUpdate* hijackerUpdate = existingHijacker->getHijackerUpdate();
			hijackerUpdate->setTargetObject( nullptr );
			hijackerUpdate->setIsInVehicle( FALSE );
			hijackerUpdate->setUpdate( FALSE );
			//MODDD - TODO - should the hijacker be made tangible before being killed? not sure
			existingHijacker->kill();
		}
	}
}

//-------------------------------------------------------------------------------------------------
/** The die callback. */
//-------------------------------------------------------------------------------------------------
void EjectPilotDie::onDie( const DamageInfo * damageInfo )
{
	//MODDD - check to see if this module should even do anything.
	// This is a safety feature in case some 'EjectPilotDie' on this object is already handling ejecting a hijacker, and
	// the others are set to do nothing then (ex: the 'Shockwave' mod has multiple 'EjectPilotDie' modules on the same
	// Object in the INI in many cases).
	if (!m_enabled)
		return;

	//MODDD - caching for several calls to 'getObject()'
	Object* dyingObject = getObject();

	//MODDD - this is too condensed to be useful for here.
	// The check for 'RequiredStatus' would block the rest of this call if a hijacker is using this 'EjectPilotDie' because
	// in retail, 'RequiredStatus' includes 'HIJACKED' all over the place.
	// Must have a way to exclude that part in particular if clearly going on to eject the hijacker is OK.
	// Checking for the death-type being correct is still fine.
	//if (!isDieApplicable(damageInfo))
	//	return;
	// Overriding 'isDieApplicable' could make sense, but several other 'DieModule' interfaces specify this on their own
	// so working that out everywhere looks like a mess at the moment.
	if (!_isDieApplicable(dyingObject, damageInfo))
		return;
	
	const EjectPilotDieModuleData* d = getEjectPilotDieModuleData();
	const ObjectCreationList* ocl = dyingObject->isSignificantlyAboveTerrain() ? d->m_oclInAir : d->m_oclOnGround;

	//MODDD - check for if this has an associated hijacker - go that route instead if so.
	if (m_hijackerID != INVALID_ID)
	{
		Object* hijacker = TheGameLogic->findObjectByID(m_hijackerID);
		ejectHijacker(hijacker, dyingObject, ocl);
		return;
	}

	Object* damageDealer = TheGameLogic->findObjectByID( damageInfo->in.m_sourceID );
	ejectPilot(ocl, dyingObject, damageDealer);
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void EjectPilotDie::crc( Xfer *xfer )
{

	// extend base class
	DieModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void EjectPilotDie::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	DieModule::xfer( xfer );

	//MODDD - new
	xfer->xferBool( &m_enabled );
	xfer->xferObjectID( &m_hijackerID );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void EjectPilotDie::loadPostProcess()
{

	// extend base class
	DieModule::loadPostProcess();

}

//MODDD - new
void EjectPilotDie::setEnabled( Bool enabled )
{
	m_enabled = enabled;
}

//MODDD - new
void EjectPilotDie::setHijacker( Object* hijacker )
{
	EjectPilotDie::checkCleanHijacker(m_hijackerID);
	m_hijackerID = hijacker->getID();
}

//MODDD - essentially a custom version of
//   DieModule::isDieApplicable -> getDieModuleData()->isDieApplicable(getObject(), ...) -> DieModuleData::isDieApplicable(...) -> m_dieMuxData.isDieApplicable
Bool EjectPilotDie::_isDieApplicable( const Object* obj, const DamageInfo *damageInfo )
{
	const DieMuxData& muxData = getDieModuleData()->m_dieMuxData;

	DeathTypeFlags deathTypes = muxData.m_deathTypes;
	VeterancyLevelFlags veterancyLevels;
	ObjectStatusMaskType exemptStatus = muxData.m_exemptStatus;
	ObjectStatusMaskType requiredStatus = muxData.m_requiredStatus;
	
	if (m_hijackerID != INVALID_ID)
	{
		// hijacked-thing/hijacker's veterancy has no impact on being able to come out
		veterancyLevels = VETERANCY_LEVEL_FLAGS_ALL;
		// Ignore the 'HIJACKED' status
		exemptStatus.set(OBJECT_STATUS_HIJACKED, 0);
	}
	else
	{
		veterancyLevels = muxData.m_veterancyLevels;
	}

	return isDieApplicableCustom(obj, damageInfo, deathTypes, veterancyLevels, exemptStatus, requiredStatus);
}
