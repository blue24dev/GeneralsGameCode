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

// FILE: UpgradeModule.cpp ////////////////////////////////////////////////////////////////////////
// Author: Johnson, Day, Smallwood September 2002
// Desc:   Upgrade module basic implementations
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"
#include "GameLogic/Module/UpgradeModule.h"


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void UpgradeModule::crc( Xfer *xfer )
{

	// extend base class
	BehaviorModule::crc( xfer );

	// extned base class
	UpgradeMux::upgradeMuxCRC( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void UpgradeModule::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// call base class
	BehaviorModule::xfer( xfer );

	// extend base class
	UpgradeMux::upgradeMuxXfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void UpgradeModule::loadPostProcess( void )
{

	// call base class
	BehaviorModule::loadPostProcess();

	// extend base class
	UpgradeMux::upgradeMuxLoadPostProcess();

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpgradeMux::UpgradeMux() : m_upgradeExecuted(false)
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool UpgradeMux::isAlreadyUpgraded() const
{
	return m_upgradeExecuted;
}

// ------------------------------------------------------------------------------------------------
// ***DANGER! DANGER! Don't use this, unless you are forcing an already made upgrade to refresh!!
// ------------------------------------------------------------------------------------------------
void UpgradeMux::forceRefreshUpgrade()
{
	if( m_upgradeExecuted )
	{
		//Only do this if we've already made the upgrade!
		upgradeImplementation();
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool UpgradeMux::attemptUpgrade( UpgradeMaskType keyMask )
{
	if (wouldUpgrade(keyMask))
	{
		// If I have an activation condition, and I haven't activated, and this key matches my condition.
		giveSelfUpgrade();
		return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool UpgradeMux::wouldUpgrade( UpgradeMaskType keyMask ) const
{
	UpgradeMaskType activation, conflicting;
	getUpgradeActivationMasks(activation, conflicting);

	//MODDD - if an upgrade lacks any 'activation' criteria (ex: empty 'TriggeredBy' criteria), doesn't it make
	// sense to just allow it unconditionally so it isn't forever unattainable?
	// Update: I would think so, but it seems some things in retail are dummied out by having no criteria to
	// activate them.
	// Ex: 'PatriotMissileEMP' has 'FireWeaponWhenDeadBehavior', with only 'DeathWeapon=ScudStormDamageWeapon' specified, not
	// 'StartsActive' (implied 'No'?) nor 'TriggeredBy'.
	// With the fix below, this dormant behavior will run and these EMP missiles will be horrifically overpowered.
	// Better question is why this module is in the INI to begin with, if never having an effect is the best case scenario.
	// TLDR: introduce this 'fix' to a mod ecosystem that never expected this at your own risk.

	//MODDD - for me only (enable anyway?)
	/*
	if(!activation.any() && !m_upgradeExecuted) {
	  return TRUE;
	}
	*/

	//Make sure we have activation conditions and we haven't performed the upgrade already.
	if( activation.any() && keyMask.any() && !m_upgradeExecuted )
	{
		//Okay, make sure we don't have any conflicting upgrades
		if( !keyMask.testForAny( conflicting) )
		{
			//Finally check to see if our upgrade conditions match.
			if( requiresAllActivationUpgrades() )
			{
				//Make sure ALL triggers requirements are upgraded
				if( keyMask.testForAll( activation ) )
				{
					return TRUE;
				}
			}
			else
			{
				//Check if ANY trigger requirements are met.
				if( keyMask.testForAny( activation ) )
				{
					return TRUE;
				}
			}
		}
	}
	//We can't upgrade!
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
void UpgradeMux::giveSelfUpgrade()
{
	// If I have an activation condition, and I haven't activated, and this key matches my condition.
	performUpgradeFX();
	processUpgradeRemoval();// Need to execute removals first, to prevent both being on for a moment.
	
	upgradeImplementation();

	//MODDD - UPDATE. Below is referring to an earlier point where I moved 'setUpgradeExecuted' above 'upgradeImplementation'.
	// However, this caused a bug: China satelite hack upgrades (basically 'SpyVisionSpecialPower' - see that '.h' file)
	// no longer work because that file's 'upgradeImplementation' checks 'isAlreadyUpgraded()' for not being the case,
	// which would be the case even the very first time upgradeImplementation is called when
	// 'upgradeImplementation'/'setUpgradeExecuted' are swapped.
	// As for why bugs weren't far more common for other upgrades (or why they lack similar 'isAlreadyUpgraded()' checks),
	// I have no idea. You could argue 'upgradeImplementation' for any upgrade should never need that check if that's
	// already handled in whatever's calling there but I'm not sure I can prove that.
	// Long story short, a more acute fix is in RiderChangeContain.h/.cpp - changing its 'm_containing' field type
	// to be a number instead of a bool so it can be incremented/decremented on calling 'RiderChangeContain::onContaining'
	// instead of only to true/false. This better copes with recursive 'onContaining' calls instead of trying to prevent
	// them entirely & doesn't require the 'upgradeImplementation'/'setUpgradeExecuted' swap in here - best of both worlds.
	// ---
	//MODDD - moved this from after 'upgradeImplementation' to here, before.
	// It is possible for an upgrade to change the current rider (bike/using that logic), leading to reaching
	// this same 'giveSelfUpgrade' recursively as sending veterency re-runs upgrade checks if the veterency
	// isn't the default 0 (normal). This can cause 'RiderChangeContain::onContaining' to be called
	// recursively, which ends in 'm_containing = FALSE' and causes the original
	// 'RiderChangeContain::onContaining' call to see 'm_containing' is FALSE and make the game think that
	// the bike/obj has no occupant and needs to be scuttled (self destruct).
	// And somewhere in there, thinks there are 2 occupants in the bike at a time instead of 1 (assertion triggered).
	// Example: sentry drones with veterency in Firestorm exploding when the nano-armor-upgrade is selected
	// from the promotion menu.
	// Chain of events for that particular case appears to start at 'RiderChangeContain::onRemoving''s
	// 'bikeTracker->setExperienceAndLevel( 0, FALSE )'.
	// I doubt making the 'ExperienceTracker::setExperienceAndLevel' -> 'm_parent->onVeterancyLevelChanged' call
	// skip 'updateUpgradeModules()' is necessary, but just an idea - not sure what the purpose of that is here.
	// ---

	setUpgradeExecuted(true);

}

//-------------------------------------------------------------------------------------------------
Bool UpgradeMux::testUpgradeConditions( UpgradeMaskType keyMask ) const
{
	UpgradeMaskType activation, conflicting;
	getUpgradeActivationMasks(activation, conflicting);

	//Okay, make sure we don't have any conflicting upgrades
	if( !keyMask.any() || !keyMask.testForAny( conflicting ) )
	{
		//Make sure we have activation conditions
		if( activation.any() )
		{
			//Finally check to see if our upgrade conditions match.
			if( requiresAllActivationUpgrades() )
			{
				//Make sure ALL triggers requirements are upgraded
				if( keyMask.testForAll( activation ) )
				{
					return TRUE;
				}
			}
			else
			{
				//Check if ANY trigger requirements are met.
				if( keyMask.testForAny( activation ) )
				{
					return TRUE;
				}
			}
		}
		else
		{
			//This *upgrade* is relying only on not having conflicts.
			return true;
		}
	}
	//We can't upgrade!
	return false;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool UpgradeMux::resetUpgrade( UpgradeMaskType keyMask )
{
	UpgradeMaskType activation, conflicting;
	getUpgradeActivationMasks(activation, conflicting);
	if( keyMask.testForAny( activation ) && m_upgradeExecuted )
	{
		m_upgradeExecuted = false;
		return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void UpgradeMux::upgradeMuxCRC( Xfer *xfer )
{

	// just call the regular xfer, it's simple
	upgradeMuxXfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer
	* Version Info
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void UpgradeMux::upgradeMuxXfer( Xfer *xfer )
{

	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// upgrade executed
	xfer->xferBool( &m_upgradeExecuted );

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void UpgradeMux::upgradeMuxLoadPostProcess( void )
{

}
