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
#include "GameLogic/HijackUtil.h"

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

//-------------------------------------------------------------------------------------------------
/** The die callback. */
//-------------------------------------------------------------------------------------------------
void EjectPilotDie::onDie( const DamageInfo * damageInfo )
{
	//MODDD - caching for several calls to 'getObject()'
	Object* dyingObject = getObject();

	//MODDD - check to see if this vehicle expects to eject a hijacker
	NameKeyType ejectHijackerTag = dyingObject->getModuleTagNameKeyToEjectHijacker();
	if (ejectHijackerTag != NAMEKEY_INVALID)
	{
		// There's a hijacker in this vehicle - is this the module that's supposed to eject it?
		if (ejectHijackerTag == this->getModuleTagNameKey())
		{
			// yes, proceed
			if (dyingObject->getHijackerInMeID() != INVALID_ID)
			{
				// Overriding 'isDieApplicable' could make sense, but several other 'DieModule' interfaces specify this on their own
				// so working that out everywhere looks like a mess at the moment.
				if (!isDieApplicableHijacked(dyingObject, damageInfo))
				{
					// if the hijacker can't come out in the module that's supposed to eject it, time to die!
					HijackUtil::clearHijackerInObject(dyingObject, TRUE);
					return;
				}

				// Get the OCL anyway for physics to apply to the emerging hijacker just like a pilot spawned on-the-spot
				const EjectPilotDieModuleData* d = getEjectPilotDieModuleData();
				const ObjectCreationList* ocl = dyingObject->isSignificantlyAboveTerrain() ? d->m_oclInAir : d->m_oclOnGround;
				Object* hijacker = TheGameLogic->findObjectByID(dyingObject->getHijackerInMeID());
				HijackUtil::ejectHijacker(hijacker, dyingObject, ocl);
				return;
			}
			// (it should not be possible to fail to eject the hijacker here, but if 'the place to do so' can't,
			// I guess allow a fall-thru to default behavior?)
		}
		else
		{
			// stop - ejecting a pilot when somewhere else is ejecting the hijacker is bad
			return;
		}
	}

	//MODDD - NOTE - the rest of this route is for checking to see if ejecting a pilot is ok per retail
	
	if (!isDieApplicable(damageInfo))
		return;

	const EjectPilotDieModuleData* d = getEjectPilotDieModuleData();
	const ObjectCreationList* ocl = dyingObject->isSignificantlyAboveTerrain() ? d->m_oclInAir : d->m_oclOnGround;

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
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void EjectPilotDie::loadPostProcess()
{

	// extend base class
	DieModule::loadPostProcess();

}

//MODDD - essentially a custom version of
// DieModule::isDieApplicable -> getDieModuleData()->isDieApplicable(getObject(), ...) -> DieModuleData::isDieApplicable(...) -> m_dieMuxData.isDieApplicable
// to use for checking whether to let a hijacker come out (obviously allow the HIJACKED status, as well as any veterancy)
Bool EjectPilotDie::isDieApplicableHijacked( const Object* obj, const DamageInfo *damageInfo )
{
	const DieMuxData& muxData = getDieModuleData()->m_dieMuxData;

	DeathTypeFlags deathTypes = muxData.m_deathTypes;
	// forced to allow all veterancy levels
	VeterancyLevelFlags veterancyLevels = VETERANCY_LEVEL_FLAGS_ALL;
	ObjectStatusMaskType exemptStatus = muxData.m_exemptStatus;
	ObjectStatusMaskType requiredStatus = muxData.m_requiredStatus;

	// allow this obviously
	exemptStatus.set(OBJECT_STATUS_HIJACKED, 0);

	return isDieApplicableCustom(obj, damageInfo, deathTypes, veterancyLevels, exemptStatus, requiredStatus);
}
