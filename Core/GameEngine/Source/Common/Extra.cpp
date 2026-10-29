//MODDD - new file

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Extra.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/KindOf.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"
#include "Common/NameKeyGenerator.h"
#include "Common/ScoreKeeper.h"
#include "Common/Recorder.h"
#include "GameNetwork/GameInfo.h"
#include "GameClient/ControlBar.h"

#include "GameLogic/Weapon.h"
#include "GameLogic/Object.h"
#include "GameLogic/Locomotor.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/GameLogic.h"

#include "GameLogic/ObjectCreationList/DeliverPayloadNugget.h"
#include "GameLogic/ObjectCreationList/GenericObjectCreationNugget.h"

#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/JetAIUpdate.h"
#include "GameLogic/Module/HackInternetAIUpdate.h"
#include "GameLogic/Module/AutoDepositUpdate.h"
//#include "GameLogic/Module/DeliverPayloadAIUpdate.h"

#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/CreateModule.h"
#include "GameLogic/Module/SpecialPowerModule.h"
#include "GameLogic/Module/OCLSpecialPower.h"
#include "GameLogic/Module/SalvageCrateCollide.h"
#include "GameLogic/Module/ActiveBody.h"
#include "GameLogic/Module/StructureBody.h"
#if RTS_ZEROHOUR
#include "GameLogic/Module/UndeadBody.h"
#endif
#include "GameLogic/Module/MaxHealthUpgrade.h"
#include "GameLogic/Module/RebuildHoleExposeDie.h"
#include "GameLogic/Module/StealthUpdate.h"
#include "GameLogic/Module/StealthDetectorUpdate.h"
#include "GameLogic/Module/SpecialAbilityUpdate.h"
#include "GameLogic/Module/ActiveShroudUpgrade.h"
#include "GameLogic/Module/CashHackSpecialPower.h"
#include "GameLogic/Module/OCLUpdate.h"
#include "GameLogic/Module/InstantDeathBehavior.h"
#include "GameLogic/Module/CreateObjectDie.h"
#include "GameLogic/Module/MinefieldBehavior.h"

#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/TransportContain.h"

//MODDD - debugging (things seem to build fine without these now?)
/*
#include <fstream>
#include <iostream>
#include <iomanip>
*/

// It would make sense to consider 'CUSTOM_ATTRIBUTE_CHANGES' sections "MODDD - for me only".

// Other new functions that handle post-parsing edits:
//   WeaponTemplateSet::validateAutoChooseSources
//   ThingTemplate::makeNonCivilianGarrisonableStructureCapturableHack (since moved to here, since "setKindOf" was added)

// Set this in code to let breakpoints work in release mode, as opposed to the usual 'int x; x = 4;' thing I usually do.
// Ex:
//   if (obj->getTemplate()->getName() == "xyz") {
//     g_dummy = 4;
//   }
int g_dummy = 0;

#if EXTRA_DEBUG_HELP
std::vector<int> g_destroyObjectSource;
#endif

namespace rts_extra
{

//MODDD - bugfix for non-shared abilities on buildings being unusable on RETAIL_COMPATIBLE_CRC=0
// Condensed several snippets of copied script into this, with a few additions to fix the bug on buildings
// with non-shared special powers (ex: strategy center, battle plans & CIA intelligence) starting with
// endless cooldowns on RETAIL_COMPATIBLE_CRC=0.
void call_objectOnBuildComplete(Object* obj, Bool checkForSpecialPowerModuleCreateCalls)
{
	BehaviorModule** m;

	// This for-loop is as-is behavior
	for (m = obj->getBehaviorModules(); *m; ++m)
	{
		CreateModuleInterface* create = (*m)->getCreate();
		if (!create)
			continue;

		create->onBuildComplete();
	}

	// The rest of below is new
	// ---------------------------
	if (!checkForSpecialPowerModuleCreateCalls)
	{
		// skip further down - ex: made by a warfactory. The SpecialPowerModule constructor already calls 'onSpecialPowerCreation' if it
		// isn't under construction at the time (structures block this since they start at in-construction / 0%-progress technically).
		// So for vehicles, etc., as-is behavior already works.
		return;
	}

	// First, see if there is a "SpecialPowerCreate" module.
	// If so, this module's 'onBuildComplete' would have already run above, which would have called 'onSpecialPowerCreation' already
	// (no need to do so manually further down).
	Bool foundSpecialPowerCreateModule = FALSE;
	static NameKeyType key_SpecialPowerCreate = NAMEKEY("SpecialPowerCreate");
	for (m = obj->getBehaviorModules(); *m; ++m)
	{
		if ((*m)->getModuleNameKey() == key_SpecialPowerCreate)
		{
			foundSpecialPowerCreateModule = TRUE;
			break;
		}
	}

	if (!foundSpecialPowerCreateModule)
	{
		// Didn't find any 'SpecialPowerCreate' modules - call 'notifyBuildComplete'
		for (m = obj->getBehaviorModules(); *m; ++m)
		{
			SpecialPowerModuleInterface* sp = (*m)->getSpecialPower();
			if (!sp)
				continue;

			sp->notifyBuildComplete();
		}
	}
}

Real getShroudClearingRangeForLookAdjusted(const Object* obj)
{
	Real shroudClearingRange = obj->getShroudClearingRange();
	// not touching 0 or negative
	if (shroudClearingRange <= 0)
	{
		return shroudClearingRange;
	}

	// Not sure if 'isUsingAirborneLocomotor' or 'isSignificantlyAboveTerrain' is best here.
	// Whichever the choice is, make sure this is kept in-sync with 'Object::onUpdatePost()' calling for a shroud-clear
	// update based on that changing between frames.
	if (obj->isKindOf(KINDOF_AIRCRAFT) && !obj->isUsingAirborneLocomotor())
	{
		// If this aircraft is grounded, reduce its shroud-clearing range by half.
		// The retail choices are clearly under the assumption it's in the air, but suddenly increasing the sight of the
		// airfield just for being parked is a tad silly.
		shroudClearingRange *= 0.5f;
	}

#if CUSTOM_ATTRIBUTE_CHANGES
	if (!obj->isKindOf(KINDOF_AIRCRAFT))
	{
		// boost the final shroud-clearing range, the bigger the bonus the smaller it is (diminishing returns)
		if (shroudClearingRange <= 100)
		{
			shroudClearingRange = shroudClearingRange * 2.50f;
		}
		else if (shroudClearingRange <= 200)
		{
			shroudClearingRange = 100 * 2.00f + (shroudClearingRange - 100) * 1.40f;
		}
		else if (shroudClearingRange <= 300)
		{
			shroudClearingRange = 100 * 2.00f + 100 * 1.40f + (shroudClearingRange - 200) * 1.10f;
		}
		else
		{
			shroudClearingRange = 100 * 2.00f + 100 * 1.40f + 100 * 1.10f + (shroudClearingRange - 300);
		}
	}
	else
	{
		// lower the boost for aircraft - often does the job as it comes
		if (shroudClearingRange <= 100)
		{
			shroudClearingRange = shroudClearingRange * 1.75f;
		}
		else if (shroudClearingRange <= 200)
		{
			shroudClearingRange = 100 * 1.50f + (shroudClearingRange - 100) * 1.20f;
		}
		else if (shroudClearingRange <= 300)
		{
			shroudClearingRange = 100 * 1.50f + 100 * 1.20f + (shroudClearingRange - 200) * 1.05f;
		}
		else
		{
			shroudClearingRange = 100 * 1.50f + 100 * 1.20f + 100 * 1.05f + (shroudClearingRange - 300);
		}
	}
#endif
	return shroudClearingRange;
}

Bool isCurrentGameCampaign()
{
#if FORCE_GAME_CONTEXT == FGC_GENERALS_CHALLENGE
	return false;
#elif FORCE_GAME_CONTEXT == FGC_CAMPAIGN
	return true;
#else
	// if 'TheGameInfo' is null, this is a campaign map (single player with no 'GameInfo').
	return (TheGameInfo == nullptr);
#endif
}

Int getUpgradedSupplyBoost(const Object* collectingObject, const std::list<upgradePair>* upgradeBoostList)
{
	Player *player = collectingObject->getControllingPlayer();
	if (!player) return 0;

	// Loop through the upgrade pairs and see if an upgrade is present that adds supply boost
	std::list<upgradePair>::const_iterator it = upgradeBoostList->begin();
	while (it != upgradeBoostList->end())
	{
		upgradePair info = *it;

		// Check if the player has the desired upgrade. If so return the boost
		static const UpgradeTemplate *upgradeTemplate = TheUpgradeCenter->findUpgrade( info.type.c_str() );
		if (upgradeTemplate && player->hasUpgradeComplete(upgradeTemplate))
		{
			return info.amount;
		}

		// check next
		++it;
	}

	return 0;
}

#if RTS_ZEROHOUR
// Fix a potential issue where structures are (I suspect) unintentionally immune to subdual (ex: microwave) because
// their subdual damage cap is less than the max health (source of the now-present 'm_subdualDamageToDisable').
// The point of the subdual damage cap is to exceed the 'subdualDamageToDisable' a bit so that a disabled object remains
// disabled for a little even if the disabling source is stopped (subdual damage is 'healed' every frame).
// If the subdual damage cap is under 'subdualDamageToDisable', the disabling point will never be reached - cut by the cap first.
// Most likely, this is because the subdual settings were copied from retail & the max health was adjusted without
// re-adjusting the subdual settings.
// A fix is to see if the subdual cap is under 'subdualDamageToDisable + <bare minimum extra, or nothing here>' and if so, enforce a more normal minimum.
void checkActiveBodyModuleDataSubdualAttributes(ActiveBodyModuleData* _data)
{
	if (_data->m_subdualDamageCap <= 0 || _data->m_subdualDamageToDisable <= 0)
	{
		// not even set / deliberately 0? going to assume this is for a reason - don't touch
		return;
	}

	// First, check to see if the cap matches the sub-damage-to-disable.
	if (_data->m_subdualDamageCap == _data->m_subdualDamageToDisable)
	{
		// Push the cap up a bit so the disabler stopping doesn't mean the affected object instantly goes back to normal.
		if (_data->m_subdualDamageCap >= 1000)
		{
			_data->m_subdualDamageCap = _data->m_subdualDamageToDisable + 200;
		}
		else if (_data->m_subdualDamageCap >= 500)
		{
			_data->m_subdualDamageCap = _data->m_subdualDamageToDisable + 100;
		}
		else if (_data->m_subdualDamageCap >= 250)
		{
			_data->m_subdualDamageCap = _data->m_subdualDamageToDisable + 50;
		}
		else
		{
			_data->m_subdualDamageCap = _data->m_subdualDamageToDisable + 20;
		}
		return;
	}

	// Is the cap less than sub-damage-to-disable? It would be impossible to disable then.
	if (_data->m_subdualDamageCap < _data->m_subdualDamageToDisable)
	{
		// Does it make more sense to go with the way it would have to be for proper setup as-is (push the cap up -> harder to disable than retail)
		// or preserve the difficulty to disable in retail, assuming the subdual settings were copied from there (push subdualDamageToDisable down ->
		// expressing this as-is would be impossible without modifying the max-health to accomplish this).
		// Going with the latter (act like retail since that's most likely the modder's intent), as long as it is sensible to.
		if (_data->m_subdualDamageCap >= 1000)
		{
			_data->m_subdualDamageToDisable = _data->m_subdualDamageCap - 200;
		}
		else if (_data->m_subdualDamageCap >= 500)
		{
			_data->m_subdualDamageToDisable = _data->m_subdualDamageCap - 100;
		}
		else
		{
			// former way it is
			_data->m_subdualDamageCap = _data->m_subdualDamageToDisable + 200;
		}
		return;
	}
}
#endif

//MODDD - make buildings that are normally built by the player (non-civilian) and garrisonable, capturable.
// Typically this combo has the "IMMUNE_TO_CAPTURE" flag (ex: 'GLAPalace' in retail generals), though I fail to see why.
// Why should a poorly defended building with poor anti-infantry garrisoned be impossible to capture?
// This excludes base defenses, which are fine to remain uncapturable.
void makeNonCivilianGarrisonableStructureCapturableHack(ThingTemplate* _this)
{
	// does not apply to base defenses
	if (_this->isKindOf(KINDOF_FS_BASE_DEFENSE) || _this->isKindOf(KINDOF_TECH_BASE_DEFENSE))
	{
		return;
	}

	// The palace has 'GARRISONABLE_UNTIL_DESTROYED', but the internet center doesn't - can't depend on that flag.
	// Fine to only check for 'IMMUNE_TO_CAPTURE'. Civilian buildings are uncapturable through a different way, see
	// 'ActionManager::canCaptureBuilding': having a garrison contain module with an original team (including its default
	// state at the start of the game) that isn't enemies with the player makes it uncapturable.
	if (_this->isKindOf(KINDOF_IMMUNE_TO_CAPTURE))
	{
		_this->setKindOf(KINDOF_IMMUNE_TO_CAPTURE, 0);
		// While we're at it, add the CAPTURABLE flag if it's missing.
		// Seems it's only needed to make the building capturable if it's neutral to the current player (ex: civilian
		// controlled tech buildings at the start of skirmish games like oil derricks). Typically any enemy building without
		// 'IMMUNE_TO_CAPTURE' is capturable with or without this flag. Still, seems proper to just add the flag as player-
		// built structures tend to have it (if they ever occur neutral, capturable too).
		// The building-hack-disable ability also mentions the 'CAPTURABLE' flag in a few places, oddly enough.
		if (!_this->isKindOf(KINDOF_CAPTURABLE))
		{
			_this->setKindOf(KINDOF_CAPTURABLE, 1);
		}
	}
}

// Go through a ThingTemplate that's recently been parsed from the INI files
// (ex: 'AmericaVehicle.ini' -> 'Object AmericaVehicleHumvee')
// and see if it has any stats or modules that should be adjusted for either bug fixes or additional attribute edits
// (ex: 30% more health for all units). The 'CUSTOM_ATTRIBUTE_CHANGES' will decide if latter changes are applied,
// otherwise, only the minimum bug-fixing changes will be applied.
// Most of these 'bugs' are from TheSuperHackers changes that cause new undesired behavior in the retail game or
// mods with macro settings such as 'PRESERVE_RETAIL_BEHAVIOR' (since split into several more specific settings)
// set to off. I still want to keep the setting that way because it being off introduces several other fixes &
// efficiency improvements (pathfinding) that are good to keep.
// Bug example: the Europe 'field promotion' ability in the 'Rise of the Reds' mod will only work on one unit
// in the radius even though the support power's tooltip suggests and retail EXE behavior indeed does affect any number
// of units in the radius. Sometimes it's possible to fix these issues without manual INI edits so existing mods
// continue to work as expected.
void automaticThingTemplateChanges(ThingTemplate* _this)
{
	// Don't do the stat adjustments for re-skinned things. They share the same module data so changes to each
	// reskinned item has been shown to be cumulative.
	// As for what things in particular are cumulative or need to be re-individually applied (base stats like buildCost, etc.), I don't know.
	// Perhaps because they're copied from a template that's been edited by this method at an earlier point, the changes will carry over
	// to them anyway. Feel free to do your own testing if you want.
	// However, a strange case from doing the 'ActiveBody' boost on everything including reskins, is the cumulative health boost can
	// go so high (default hole health in the INI's is absurdly high for some reason at 9999999.9) that it causes the float to lose
	// precision at some ridiculous value like 2e13. When 'ActiveBody' is created for a newly created hole, the 'RebuildHoleExposesDie'
	// that created the hole object calls 'body->setMaxHealth' on it, which does 'internalChangeHealth( maxHealth - m_currentHealth );'
	// and usually works as expected (sets current health to max health). In this case, with a currentHealth so high, the maxHealth that's
	// properly set (ex: 800) minus "2e13" is completely lost to its lack of precision so the currentHealth ends up becoming 0.
	// That causes 'setCorrectDamageState' to run which thinks the object is rubble for having a health of 0, which makes the hole unselectable
	// and undamageable (but you can build over it with a dozer to immediately delete it). STRANGE STRANGE ENGINE.
	if (_this->m_reskinnedFrom != nullptr)
	{
		return;
	}

#if CUSTOM_ATTRIBUTE_CHANGES
	// Keep track of the vision before my tampering in case stealth detection needs to know what it was
	// (my 'tampering' has been moved below the iteration through modules since - whoops)
	Real originalVision = _this->m_visionRange;
#endif

	makeNonCivilianGarrisonableStructureCapturableHack(_this);
	
	static NameKeyType SalvageCrateCollideNameKey = NAMEKEY("SalvageCrateCollide");
#if RTS_ZEROHOUR
	static NameKeyType ConvertToHijackedVehicleCrateCollideNameKey = NAMEKEY("ConvertToHijackedVehicleCrateCollide");
#endif
	static NameKeyType StealthUpdateNameKey = NAMEKEY("StealthUpdate");
	static NameKeyType ActiveBodyNameKey = NAMEKEY("ActiveBody");
	static NameKeyType StructureBodyNameKey = NAMEKEY("StructureBody");
#if RTS_ZEROHOUR
	static NameKeyType UndeadBodyNameKey = NAMEKEY("UndeadBody");
#endif
#if CUSTOM_ATTRIBUTE_CHANGES
	static NameKeyType RebuildHoleExposeDieNameKey = NAMEKEY("RebuildHoleExposeDie");
	static NameKeyType MaxHealthUpgradeNameKey = NAMEKEY("MaxHealthUpgrade");
	static NameKeyType StealthDetectorUpdateNameKey = NAMEKEY("StealthDetectorUpdate");
	static NameKeyType SpecialAbilityUpdateNameKey = NAMEKEY("SpecialAbilityUpdate");
	static NameKeyType ActiveShroudUpgradeNameKey = NAMEKEY("ActiveShroudUpgrade");
	static NameKeyType OCLSpecialPowerNameKey = NAMEKEY("OCLSpecialPower");
	static NameKeyType CashHackSpecialPowerNameKey = NAMEKEY("CashHackSpecialPower");
#endif
	static NameKeyType AutoDepositUpdateNameKey = NAMEKEY("AutoDepositUpdate");
	static NameKeyType HackInternetAIUpdateNameKey = NAMEKEY("HackInternetAIUpdate");
	static NameKeyType SpawnBehaviorNameKey = NAMEKEY("SpawnBehavior");
	static NameKeyType MinefieldBehaviorNameKey = NAMEKEY("MinefieldBehavior");

	static NameKeyType TransportContainNameKey = NAMEKEY("TransportContain");
	static NameKeyType OverlordContainNameKey = NAMEKEY("OverlordContain");
	static NameKeyType HelixContainNameKey = NAMEKEY("HelixContain");
	static NameKeyType RiderChangeContainNameKey = NAMEKEY("RiderChangeContain");
	static NameKeyType CaveContainNameKey = NAMEKEY("CaveContain");
	static NameKeyType GarrisonContainNameKey = NAMEKEY("GarrisonContain");
	static NameKeyType HealContainNameKey = NAMEKEY("HealContain");
	static NameKeyType InternetHackContainNameKey = NAMEKEY("InternetHackContain");
	static NameKeyType MobNexusContainNameKey = NAMEKEY("MobNexusContain");
	static NameKeyType ParachuteContainNameKey = NAMEKEY("ParachuteContain");
	static NameKeyType RailedTransportContainNameKey = NAMEKEY("RailedTransportContain");
	static NameKeyType TunnelContainNameKey = NAMEKEY("TunnelContain");
	static NameKeyType JetAIUpdateNameKey = NAMEKEY("JetAIUpdate");
	static NameKeyType ChinookAIUpdateNameKey = NAMEKEY("ChinookAIUpdate");

#if CUSTOM_ATTRIBUTE_CHANGES
	Bool foundStealthDetectorUpdate = false;
	Bool foundActiveShroudUpgrade = false;
	StealthDetectorUpdateModuleData* stealthDetectorData = nullptr;
#endif
	Bool foundSpawnBehavior = false;
	Bool foundContainBehavior = false;
	// what a specific check
	Bool foundJetAIUpdateBehavior = false;
	Bool foundChinookAIUpdateBehavior = false;
	AIUpdateModuleData* aiUpdateModuleData = nullptr;
	Bool _canOccupyAirFieldParkingSpace = false;
	
	// Whether this thing gets a cost reduction for being a renewable income source at the very end.
	// There is checking kindof's like FS_BLACK_MARKET or FS_SUPPLY_DROP_ZONE, but I'd rather have a more accurate check
	// in case some mod has something that lacks either of these flags but is clearly able to (primarily?) generate money.
	// Determining this for hackers and black markets is easy (having AutoDepositUpdate or HackInternetAIUpdate).
	// For supply drop zones, things are trickier - this is done by a 'OCLUpdate' that creates a plane to deliver the money
	// crates. However, this has the potential to get even trickier to decide - see the Contra mod's cybernetic's general
	// supply drop zone - it use a phase of over 4 OCLs that end up producing the money crates.
	// Then there's the neutral tech structure options as seen in the Contra and Rise of the Reds mods, both are
	// uncapturable & have crates dropped at regular intervals for whoever grabs them.
	// Unfortunately, OCL-lookups aren't guaranteed to work here at thing-parsing (what if OCLs haven't been parsed yet?
	// What if the Thing spawned by an OCL hasn't been parsed yet because it occurs later in the things to be parsed?).
	// Handling 'OCLUpdate' handling in 'automaticChangesPostINIParsing'.  you know what, all of them, may as well.
	//Bool renewableMoneySourceCostReduction = false;

	// See if any modules need automatic adjustments.
	// This is how health changes are applied, since health is stored in a module (ex: 'ActiveBody') instead of on the
	// ThingTemplate itself. Also, 'const' is just to keep the compiler happy for most of these calls to get to the
	// modules, the actual module data to be edited has that casted out so it is modifiable.
	Int modIdx;
	const ModuleInfo& mi = _this->getBehaviorModuleInfo();
	for (modIdx = 0; modIdx < mi.getCount(); ++modIdx)
	{
		AsciiString modName = mi.getNthName(modIdx);
		if (modName.isEmpty())
			continue;
		const ModuleData* data = mi.getNthData(modIdx);
		NameKeyType modNameKey = NAMEKEY(modName);

		if( modNameKey == SalvageCrateCollideNameKey )
		{
			SalvageCrateCollideModuleData* _data = (SalvageCrateCollideModuleData*)data;
			// If a salvage crate is large enough, assume it's meant to apply to multiple units.
			// Ex: 'field promotion' in 'Rise of the Reds' uses an invisible crate to decide whatever to boost the rank of
			// (whatever's touching the crate).
			// A code change caused crates to stop applying after one item triggers the effect. A since-added 'allowMultiPickup'
			// setting restores the original behavior - desired in this case.
			// Assume that makes sense if any part of the collision is large enough.
			if (
				_this->m_geometryInfo.getMinorRadius() >= 20 ||
				_this->m_geometryInfo.getMajorRadius() >= 20
				// no need to include 'getRawHeight'.
			)
			{
				_data->m_allowMultiPickup = TRUE;
			}
		}
#if RTS_ZEROHOUR
		else if ( modNameKey == ConvertToHijackedVehicleCrateCollideNameKey )
		{
			// Don't need to do anything to the module data - just force 'hijackGuard' on for this object in the INI.
			// ex: flying hijackers in the Contra mod - even if they use the fake guard button, this flag is needed
			// for some hijack commands to take precedence over standard attack ones
			_this->m_hijackGuard = true;
		}
#endif
		else if( modNameKey == StealthUpdateNameKey )
		{
			// This change is part of a bugfix for attack cycles becoming stealthed when evacuated.
			// This was technically possible in retail, but never happened because the 'StealthDelay' from the rider's 'StealthUpdate'
			// (likely the default from default/Object.ini) was long enough it was never reached before the recently evac'd bike would
			// self destruct. Currently, the attack bike will use the 'stealthDelay' from the bike itself when immediately evac'd
			// (has its own 'StealthUpdate'), but because it only defines 'UseRiderStealth' and the default object's 'StealthUpdate'
			// is wrapped by 'OverrideableByLikeKind', the attack bike's 'StealthUpdate' completely replaces anything unspecified
			// with hardcoded defaults from here instead of anything specified in the default object's 'StealthUpdate'.
			// Defaulting 'm_innateStealth' to 'false' instead of 'true' is possible, but it seems like some mod out there could rely
			// on the original assumption for immediate stealth behavior (intended stealth behavior no longer works as expected).
			// Going with a more surgical fix: if 'm_innateStealth' is unspecified, imply a default of 'false' for it if
			// 'UseRiderStealth = Yes' is set.
			StealthUpdateModuleData* _data = (StealthUpdateModuleData*)data;
			(void)_data;

#if RTS_ZEROHOUR
			// check for the new default - set based off 'useRiderStealth'
			if (_data->m_innateStealth == 0xFFu)
			{
				if (_data->m_useRiderStealth)
				{
					_data->m_innateStealth = 0;
				}
				else
				{
					_data->m_innateStealth = 1;
				}
			}
#endif
		}
		// For whether it makes sense to apply health changes, checking for 'ActiveBody', 'StructureBody', and 'UndeadBody' should be enough.
		// Other body subclasses are 'ImmortalBody' and 'HighlanderBody', which don't look to be for normal commandable/attackable things.
		else if( modNameKey == ActiveBodyNameKey )
		{
			ActiveBodyModuleData* _data = (ActiveBodyModuleData*)data;
			(void)_data;

#if RTS_ZEROHOUR
			// turned into a helper because copying that across 3 places would be obnoxious. Yay inheritance.
			checkActiveBodyModuleDataSubdualAttributes(_data);
#endif
#if CUSTOM_ATTRIBUTE_CHANGES
			// Assume if a health value is so absurdly high it shouldn't be touched
			if (_data->m_maxHealth < 1000000.0f)
			{
				Real healthMulti = getHealthMulti(_this);
				if (healthMulti != 1.0f)
				{
					_data->m_initialHealth *= healthMulti;
					_data->m_maxHealth *= healthMulti;
				}
			}
#endif
		}
		else if( modNameKey == StructureBodyNameKey )
		{
			StructureBodyModuleData* _data = (StructureBodyModuleData*)data;
			(void)_data;
			
#if RTS_ZEROHOUR
			checkActiveBodyModuleDataSubdualAttributes(_data);
#endif
#if CUSTOM_ATTRIBUTE_CHANGES
			// Assume if a health value is so absurdly high it shouldn't be touched
			if (_data->m_maxHealth < 1000000.0f)
			{
				Real healthMulti = getHealthMulti(_this);
				if (healthMulti != 1.0f)
				{
					_data->m_initialHealth *= healthMulti;
					_data->m_maxHealth *= healthMulti;
				}
			}
#endif
		}
#if RTS_ZEROHOUR
		else if( modNameKey == UndeadBodyNameKey )
		{
			UndeadBodyModuleData* _data = (UndeadBodyModuleData*)data;
			(void)_data;

			checkActiveBodyModuleDataSubdualAttributes(_data);
			
#if CUSTOM_ATTRIBUTE_CHANGES
			// Assume if a health value is so absurdly high it shouldn't be touched
			if (_data->m_maxHealth < 1000000.0f)
			{
				Real healthMulti = getHealthMulti(_this);
				if (healthMulti != 1.0f)
				{
					_data->m_initialHealth *= healthMulti;
					_data->m_maxHealth *= healthMulti;
					_data->m_secondLifeMaxHealth *= healthMulti;
				}
			}
#endif
		}
#endif
#if CUSTOM_ATTRIBUTE_CHANGES
		else if( modNameKey == RebuildHoleExposeDieNameKey )
		{
			RebuildHoleExposeDieModuleData* _data = (RebuildHoleExposeDieModuleData*)data;
			// all holes get the same boost.
			_data->m_holeMaxHealth *= 1.75f;
		}
		else if( modNameKey == MaxHealthUpgradeNameKey )
		{
			MaxHealthUpgradeModuleData* _data = (MaxHealthUpgradeModuleData*)data;
			Real healthMulti = getHealthMulti(_this);
			if (healthMulti != 1.0f)
			{
				_data->m_addMaxHealth *= healthMulti;
			}
		}
		else if( modNameKey == StealthDetectorUpdateNameKey )
		{
			foundStealthDetectorUpdate = true;
			StealthDetectorUpdateModuleData* _data = (StealthDetectorUpdateModuleData*)data;
			stealthDetectorData = _data;
			if (_data->m_detectionRange > 0.0f)
			{
				_data->m_detectionRange *= 1.12f;
			}
			else
			{
				// Normally, the stealth detection range would come from the unit's vision if the detection range isn't explicitly
				// provided, but I don't want stealth detection to get the same boost as vision, so establish the 'detectionRange'
				// here from the vision as it was before my tampering, and with a different bonus since 'detectionRange' is normally
				// always less than vision when specified anyway (not manually defined -> more than it would have been -> less bonus).
				_data->m_detectionRange = originalVision * 1.08f;
			}
		}
		else if( modNameKey == SpecialAbilityUpdateNameKey )
		{
			SpecialAbilityUpdateModuleData* _data = (SpecialAbilityUpdateModuleData*)data;
			// NOTE - this constant comes from SPECIAL_ABILITY_HUGE_DISTANCE in SpecialAbilityUpdate.h
			if (_data->m_startAbilityRange > 0 && _data->m_startAbilityRange < 10000000.0f)
			{
				_data->m_startAbilityRange *= 1.15f;
			}
		}
		else if( modNameKey == ActiveShroudUpgradeNameKey )
		{
			foundActiveShroudUpgrade = true;
		}
		else if( modNameKey == CashHackSpecialPowerNameKey )
		{
			CashHackSpecialPowerModuleData* _data = (CashHackSpecialPowerModuleData*)data;
			// cut the amount stolen by 60%, infinite-range cash hack special powers are obnoxious
			_data->m_defaultAmountToSteal *= 0.4;
			std::vector<CashHackSpecialPowerModuleData::Upgrades>::iterator it;
			for (it = _data->m_upgrades.begin(); it != _data->m_upgrades.end(); ++it)
			{
				it->m_amountToSteal *= 0.4;
			}
		}
#endif
		else if( modNameKey == AutoDepositUpdateNameKey )
		{
#if RENEWABLE_MONEY_SOURCE_HALF_EFFECTIVE
			AutoDepositUpdateModuleData* _data = (AutoDepositUpdateModuleData*)data;

			Real scalar;
			// Note: a check for not being a faction structure, like ('not' of this)
			//   Bool Object::isFactionStructure() -> isAnyKindOf( KINDOFMASK_FS );
			// could work too
			// Actually, beware: Contra's forever-neutral-controlled (uncapturable) 'TechSupplyDropZone' still has 'FS_SUPPLY_DROPZONE'.
			if (!_this->isKindOf(KINDOF_TECH_BUILDING))
			{
				// normal: half as efficient
				scalar = 0.5f;
			}
			else
			{
				// For tech buildings, less steep of a reduction.
				// This makes tech buildings more rewarding to have since they're impacted less by the income rate reduction than player-built sources.
				scalar = 0.75;
			}

			// 1000 / ? = 1500
			// 1000 = 1500 * ?

			// check - if the delay is low enough, adjust the duration instead
			if (_data->m_depositFrame / LOGICFRAMES_PER_SECOND < 5)
			{
				// duration increased (dividing by the below-1 scalar increases the duration)
				_data->m_depositFrame = (UnsignedInt) ((Real)_data->m_depositFrame / scalar);
			}
			else
			{
				// duration is unaffected, adjust the amount per deposit
				_data->m_depositAmount = (Int) ((Real)_data->m_depositAmount * scalar);
			}
#endif
			//renewableMoneySourceCostReduction = true;
		}
		else if( modNameKey == HackInternetAIUpdateNameKey )
		{
			// Note - since the money from this ability is often small per update (5 or 6 credits in retail?), going to double
			// its time & double its cash so there isn't a net change in the amount of income for a given time period.
			// The point is to reduce the round error from some cheats (ex: +10% income -> completely lost to round error in anything under 10).
			HackInternetAIUpdateModuleData* _data = (HackInternetAIUpdateModuleData*)data;
			(void)_data;

			// could make this depend on CUSTOM_ATTRIBUTE_CHANGES? No net change so I don't think I need to.
			_data->m_cashUpdateDelay *= 2;
#if RTS_ZEROHOUR
			_data->m_cashUpdateDelayFast *= 2;
#endif
			// increase the XP per hack to rank up in the same amount of time
			_data->m_xpPerCashUpdate *= 2;
			_data->m_regularCashAmount *= 2;
			_data->m_veteranCashAmount *= 2;
			_data->m_eliteCashAmount *= 2;
			_data->m_heroicCashAmount *= 2;
#if RENEWABLE_MONEY_SOURCE_HALF_EFFECTIVE
			// double the delay again w/o adjusting cash amount per update to halve the income
			_data->m_cashUpdateDelay *= 2;
#if RTS_ZEROHOUR
			_data->m_cashUpdateDelayFast *= 2;
#endif
			// XP boost will be 1.5 times instead since this the overall pace of the game is slower.
			// An income source ranking up at the same rate as the original game with everything else slowed down (most factions
			// don't get rank-up-able income sources) would be an unfair advantage, I feel.
			// This should be possible without roundoff error even in retail's XP of 1, since it is guaranteed doubled above now:
			// [at least 2] times 1.5 -> 3
			_data->m_xpPerCashUpdate *= 1.5;
#endif
			//renewableMoneySourceCostReduction = true;
		}
		else if ( modNameKey == SpawnBehaviorNameKey )
		{
			foundSpawnBehavior = true;
		}
#if DONT_RUN_OVER_NEUTRAL_UNITS
		else if ( modNameKey == MinefieldBehaviorNameKey )
		{
			// don't let mines trigger on neutrals - go go gadget geneva convention
			MinefieldBehaviorModuleData* _data = (MinefieldBehaviorModuleData*)data;
			// If this mine is detonated by enemies and includes neutrals or allies, make it only enemies (clip neutrals/allies)
			if ((_data->m_detonatedBy & (1 << ENEMIES)) && (_data->m_detonatedBy & ((1 << NEUTRAL) | (1 << ALLIES))) )
			{
				_data->m_detonatedBy = (1 << ENEMIES);
			}
		}
#endif
		else if ( modNameKey == TransportContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == OverlordContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == HelixContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == RiderChangeContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == CaveContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == GarrisonContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == HealContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == InternetHackContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == MobNexusContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == ParachuteContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == RailedTransportContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == TunnelContainNameKey )
		{
			foundContainBehavior = true;
		}
		else if ( modNameKey == JetAIUpdateNameKey )
		{
			foundJetAIUpdateBehavior = true;
			aiUpdateModuleData = (AIUpdateModuleData*)data;

			JetAIUpdateModuleData* _data = (JetAIUpdateModuleData*)data;
			// Copying the 'JetAIUpdate::canOccupyAirFieldParkingSpace' check
			_canOccupyAirFieldParkingSpace = _data->m_needsRunway;

			// If this new field isn't specified, default it from whether this has the 'PRODUCED_FROM_HELIPAD' kindof for the
			// best retail accuracy, or at least clarifty of intent (checking 'm_needsRunway' may be an alternative, though I don't agree as much).
			if (_data->m_usesJetTakeoff == 0xFFu)
			{
				if (_this->isKindOf(KINDOF_PRODUCED_AT_HELIPAD))
				{
					_data->m_usesJetTakeoff = 0;
				}
				else
				{
					_data->m_usesJetTakeoff = 1;
				}
			}
		}
		else if ( modNameKey == ChinookAIUpdateNameKey )
		{
			foundChinookAIUpdateBehavior = true;
			aiUpdateModuleData = (AIUpdateModuleData*)data;
			// Copying the 'ChinookAIUpdate::canOccupyAirFieldParkingSpace' check
			_canOccupyAirFieldParkingSpace = false;
		}

	}

	// This is here to disable the strange hackery of having 'KINDOF_SPAWNS_ARE_THE_WEAPONS' for the stinger soldier as of retail INI,
	// even though this flag is clearly intended for the thing that handles the spawning (stinger site).
	// A few other changes elsewhere in the codebase should make this no longer necessary.
	if (_this->isKindOf(KINDOF_SPAWNS_ARE_THE_WEAPONS) && !foundSpawnBehavior)
	{
		_this->setKindOf(KINDOF_SPAWNS_ARE_THE_WEAPONS, 0);
	}

	// This checks to see if a helicopter lacks a contain behavior (ex: comanche from retail).
	// A contain of some kind is needed at a bare minimum so its 'objectEnterExitInfo' map is available for the
	// helicopter's AIUpdate to see if something is trying to enter it so it knows to land.
	// For things that don't need to respond to a request for entry (ex: vehicles: crusader tank lacks a contain behavior
	// too), this isn't an issue.
	//KindOfMaskType kindOfTest;
	//kindOfTest.set(KINDOF_AIRCRAFT);
	// now jets too, so a jet can wait on taking off autonomously (ex: resuming guard mode after reload) if a pilot is
	// running towards it.
	//kindOfTest.set(KINDOF_PRODUCED_AT_HELIPAD);
	//if (!foundContainBehavior && _this->isAllKindOf(kindOfTest))

	if (_this->isKindOf(KINDOF_AIRCRAFT))
	{
		// checks for 'ChinookAIUpdate' too for combat(trainable) helicopters that actually use that instead of 'JetAIUpdate'
		if (foundJetAIUpdateBehavior || foundChinookAIUpdateBehavior)
		{
			// send what's been figured from reading the specific module
			_this->m_canOccupyAirFieldParkingSpace = _canOccupyAirFieldParkingSpace;

			if (!foundContainBehavior)
			{
				// Add a bogus 'TransportContain' with a capacity of 0 slots that forbids everything - that should do it.
				static AsciiString TransportContainName = AsciiString("TransportContain");
				AsciiString moduleTag = AsciiString("Module_BOGUSTRANSPORT12345");
				// would've used 'getBehaviorModuleInfo()', but this is a non-const reference that allows items to be added
				ModuleInfo& mi = _this->m_behaviorModuleInfo;
				ModuleData* data = TheModuleFactory->newModuleDataFromINI(nullptr, TransportContainName, MODULETYPE_BEHAVIOR, moduleTag);
		
				TransportContainModuleData* _data = (TransportContainModuleData*)data;
				_data->m_slotCapacity = 0;
				_data->m_allowInsideKindOf.clear();
				_data->m_forbidInsideKindOf.set();
				_data->m_numberOfExitPaths = 0;

				mi.addModuleInfo(_this, TransportContainName, moduleTag, data, TransportContain::getInterfaceMask(), false, false);
			}
		}
		else
		{
			// an AIRCRAFT not using a JetAIUpdate nor ChinookAIUpdate?
			// Have to decide if this is a helicopter or not. All I got's the retail PRODUCET_AT_HELIPAD check.
			_this->m_canOccupyAirFieldParkingSpace = !_this->isKindOf(KINDOF_PRODUCED_AT_HELIPAD);
		}
	}

	// (this section now occurs below the module iteration block in case the presence of some module affects the stat
	// changes below)
#if CUSTOM_ATTRIBUTE_CHANGES
	// Temp hack. Add active-shroud-generation to anything that is a stealth detector.
	// This lets shroud generation be easy to see/test in mods that never use the shroud generating module (including retail).
	// Requires the "MODDD - for me only" block in 'UpgradeModule.cpp' to be enabled so lacking a condition still lets this work.
	/*
  if (foundStealthDetectorUpdate && !foundActiveShroudUpgrade)
	{
		static AsciiString ActiveShroudUpgradeName = AsciiString("ActiveShroudUpgrade");
		// the detector needs to support types other than 'MINE' and 'DEMOTRAP'.
		// why bother if not?
		// (also allow if 'extraDetectKindof' has no bits set - means this wasn't specified so imply everything)
		KindOfMaskType testMask;
		testMask.set(KINDOF_MINE);
		testMask.set(KINDOF_DEMOTRAP);
		testMask.flip();
		Real visionRange = _this->friend_calcVisionRange();
		if (
			(
				!stealthDetectorData->m_extraDetectKindof.any() ||
				stealthDetectorData->m_extraDetectKindof.getAnd(testMask).any()
			)
			// even if you're a detector, a vision of 0 tells me something isn't quite right
			&& visionRange > 0
		)
		{
			// proceed
			//mi.getInfoVector().push_back(Nugget(modName, AsciiString("Module_GENERATED4567"), data, interfaceMask, inheritable, overrideableByLikeKind));

			AsciiString moduleTag = AsciiString("Module_GENERATED4567");
			// would've used 'getBehaviorModuleInfo()', but this is a non-const reference that allows items to be added
			ModuleInfo& mi2 = _this->m_behaviorModuleInfo;
			ModuleData* data = TheModuleFactory->newModuleDataFromINI(nullptr, ActiveShroudUpgradeName, MODULETYPE_BEHAVIOR, moduleTag);
		
			ActiveShroudUpgradeModuleData* mdCast = (ActiveShroudUpgradeModuleData*)data;
			Real testRange = stealthDetectorData->m_detectionRange;
			if (testRange == 0) {
				testRange = visionRange;
			}
			mdCast->m_newShroudRange = testRange * 0.75f;

			// ?
			//mi2.addModuleInfo(_this, ActiveShroudUpgradeName, moduleTag, data, (MODULEINTERFACE_UPDATE), false, false);
			mi2.addModuleInfo(_this, ActiveShroudUpgradeName, moduleTag, data, ActiveShroudUpgrade::getInterfaceMask(), false, false);

			//BehaviorModule* newMod = (BehaviorModule*)TheModuleFactory->newModule(this, modName, data, MODULETYPE_BEHAVIOR);
			//*curB++ = newMod;
		}
  }
	*/
#endif

#if CUSTOM_ATTRIBUTE_CHANGES
	// This area is called after ThingFactory::parseObjectDefinition's 'ini->initFromINI( thingTemplate...' call,
	// so any hackish edits to apply to everything can go here.
	if (_this->isKindOf(KINDOF_STRUCTURE))
	{
#if RTS_ZEROHOUR
		if (_this->isKindOf(KINDOF_FS_SUPERWEAPON))
		{
			// superweapons cost a bit more but already take a while to build - less factor there
			_this->m_buildCost *= 1.25f;
			_this->m_buildTime *= 1.05f;
		}
		else
#endif
		if(_this->isKindOf(KINDOF_FS_BASE_DEFENSE))
		{
			// to be less spammable since each one's health boost is more noticeable
			_this->m_buildCost *= 1.15f;
			_this->m_visionRange *= 1.5f;
		}
		else
		{
			// all other buildings
			_this->m_buildTime *= 1.15f;
			_this->m_visionRange *= 1.25f;
		}
	}
	else
	{
		// non-buildings
		_this->m_buildTime *= 1.08f;

		// enforce a minimum of 180 vision first.
		// Note this happens before the multiplication so the real effective minimum is higher.
		// TODO - can you iterate through weapons to see what the longest range is to see if this should be automatically boosted?
		if (_this->m_visionRange > 0)
		{
			if (_this->m_visionRange < 180.0f)
			{
				_this->m_visionRange = 180.0f;
			}
			_this->m_visionRange *= 1.3f;
		}
	}

	// EXTRA SLOW-DOWN FOR EVERYTHING
	//_this->m_buildTime *= 1.35f;
	_this->m_buildTime *= 1.80f;

	// Make things that are exclusively dozers cheaper.
	// This that are dozers and harvesters at the same time (GLA workers) don't need as much of a reduction.
	if (_this->isKindOf(KINDOF_DOZER))
	{
		if (_this->isKindOf(KINDOF_HARVESTER))
		{
			// Worker
			// (200 -> 130)
			_this->m_buildCost *= 0.65f;
		}
		else
		{
			// (1000 -> 500)
			// Normal dozer (vehicle)
			_this->m_buildCost *= 0.50f;
		}
	}
	else
	{
		// Also, if something isn't a dozer and is a harvester (chinook, supply truck), cut the price by 25%.
		if (_this->isKindOf(KINDOF_HARVESTER) && !(_this->isKindOf(KINDOF_CAN_ATTACK)) )
		{
			_this->m_buildCost *= 0.75f;
		}
#if RTS_ZEROHOUR
		// and those
		else if (_this->isKindOf(KINDOF_FS_SUPPLY_CENTER))
		{
			_this->m_buildCost *= 0.75f;
		}
		else if (_this->isKindOf(KINDOF_FS_WARFACTORY))
		{
			_this->m_buildCost *= 0.75f;
		}
#endif
	}

	// NOTE - handling 'shroudClearingRange' changes in 'getShroudClearingRangeForLookAdjusted' instead for better control during runtime
	// (ex: diminishing returns for a range boosted by bonuses during the game)
#endif

}

// prototypes
void automaticChangesPostINIParsing_things();
void automaticChangesPostINIParsing_thing(ThingTemplate* _this);
Bool automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate(ObjectCreationList* ocl, std::set<ObjectCreationList*>& processedOCLList);
Bool automaticChangesPostINIParsing_thing_hasMoneyCrateCollide(const ThingTemplate* suspectMoneyCrate);
Bool automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate_checkSpawned(const ThingTemplate* spawnedByOCL, std::set<ObjectCreationList*>& processedOCLList);
void automaticChangesPostINIParsing_OCLs();
void automaticChangesPostINIParsing_OCL(ObjectCreationList& ocl, std::set<AsciiString>& transportProcessedList);

// What to do after parsing everything else.
// Ex: 'TheControlBar' is unavailable until this point ('ControlBar::init' handles parsing in 'CommandButton.ini', not 'GameEngine::init'
// that does it for thing templates and a host of other things).
// For reference, 'ControlBar::init' is reached by 'initSubsystem(TheGameClient...' in 'GameEngine::init', which is called after thing templates are handled.
void automaticChangesPostINIParsing()
{
	automaticChangesPostINIParsing_things();
	automaticChangesPostINIParsing_OCLs();
}

void automaticChangesPostINIParsing_things()
{
	ThingTemplateHashMap* templateHashMap = TheThingFactory->getTemplateHashMap();
	ThingTemplateHashMap::iterator it;
	for (it = templateHashMap->begin(); it != templateHashMap->end(); ++it)
	{
		ThingTemplate* _this = (*it).second;
		automaticChangesPostINIParsing_thing(_this);
	}
}

//MODDD - temp help
//int g_callDepth;

void automaticChangesPostINIParsing_thing(ThingTemplate* _this)
{
	//TODO - should overrides be blocked like the '_this->m_reskinnedFrom != nullptr' check elsewhere? Not sure, see if doing this for the base
	// implicitly carries over to overrides for this post-parsing run-through.
	// My guess is this should be re-run for things that are reskins of something else,
	// because ThingFactory.cpp has 'thingTemplate->copyFrom(reskinTmpl);' - I'm lead to believe by the post-parsing point
	// (all things loaded in), every reskin is an independent copy of the thing it's reskinned from -> changes to the
	// first encountered parent don't affect the reskinned thing. This is unlike during parsing where changes to a parent
	// would affect things that are reskins of it because these children would read the then-modified parent to be copied
	// from when they are parsed later (repeating changes in that case would be redundant).

	static NameKeyType OCLSpecialPowerNameKey = NAMEKEY("OCLSpecialPower");
	static NameKeyType AutoDepositUpdateNameKey = NAMEKEY("AutoDepositUpdate");
	static NameKeyType HackInternetAIUpdateNameKey = NAMEKEY("HackInternetAIUpdate");
	static NameKeyType OCLUpdateNameKey = NAMEKEY("OCLUpdate");

#if defined(RENEWABLE_MONEY_SOURCE_COST_SCALAR)
	Bool renewableMoneySourceCostReduction = false;
#endif
#if EXTRA_MONEY_CRATE_OCL_TRACKING
	// more specific check for supply drop zones (know to let OCLs know they're for adjusted-value moeny crates)
	Bool renewableMoneyOCLSource = false;
#endif

	//g_callDepth = 0;

	Int modIdx;
	const ModuleInfo& mi = _this->getBehaviorModuleInfo();
	for (modIdx = 0; modIdx < mi.getCount(); ++modIdx)
	{
		AsciiString modName = mi.getNthName(modIdx);
		const ModuleData* data = mi.getNthData(modIdx);
		if (modName.isEmpty())
			continue;
		NameKeyType modNameKey = NAMEKEY(modName);

		if( modNameKey == OCLSpecialPowerNameKey )
		{
			OCLSpecialPowerModuleData* _data = (OCLSpecialPowerModuleData*)data;

			if (_data->m_createLoc == CREATE_AT_LOCATION)
			{
				// See if this is an OCL special power that requires the user to place something on the map with a build preview, such as the sneak attack.
				// In several mods, the 'ReferenceObject' field is either missing or referring to a nonexistent object - sometimes, just the incorrect variant
				// such as a stealth-gen sneak attack tunnel for the chem general's ability, but that detail is probably negligible.
				// This field was just for the AI to do finer collision checks with the resulting object, but this is now needed even for player use because of
				// a TheSuperHacker's change in 'ActionManager::canDoSpecialPowerAtLocation' with RETAIL_COMPATIBLE_CRC=0: the 'reference object' is used to see
				// if the object is placeable on the game's end so the client saying it's buildable isn't blindly trusted (anti-cheat measure).
				const ThingTemplate* referenceTTRef = nullptr;
				if (!_data->m_referenceThingName.isEmpty())
				{
					referenceTTRef = TheThingFactory->findTemplate( _data->m_referenceThingName );
				}

				if (referenceTTRef == nullptr)
				{
					// No 'ReferenceObject' field, or whatever was provided wasn't found in the list of game objects - need to figure out what the field should be.
					// I did find that there is always a CommandButton that can be used to decide what this should be: Command = SPECIAL_POWER_CONSTRUCT or SPECIAL_POWER_CONSTRUCT_FROM_SHORTCUT,
					// has an 'Object' field that would match the special power's 'ReferenceObject' in retail -> copy it over to apply the fix.
					// The only issue is, there isn't a way to tell which abilities ever have a button to use them with placement (special power needs a valid 'ReferenceObject')
					// without checking every single button & coming up empty or finding one of the expected 'Command'.
					const CommandButton* commandButtons = TheControlBar->getCommandButtons();
					const CommandButton* commandButton;
					for( commandButton = commandButtons; commandButton != nullptr; commandButton = commandButton->getNext() )
					{
						if (commandButton->getCommandType() == GUI_COMMAND_SPECIAL_POWER_CONSTRUCT || commandButton->getCommandType() == GUI_COMMAND_SPECIAL_POWER_CONSTRUCT_FROM_SHORTCUT)
						{
							if (commandButton->getThingTemplate() != nullptr)
							{
								referenceTTRef = commandButton->getThingTemplate();
								break;
							}
						}
					}

					if (referenceTTRef != nullptr)
					{
						// we got em'.
						_data->m_referenceThingName = referenceTTRef->getName();
					}
				}
			}
		}
		else if ( modNameKey == AutoDepositUpdateNameKey )
		{
#if defined(RENEWABLE_MONEY_SOURCE_COST_SCALAR)
			// has this at all -> yes for now
			renewableMoneySourceCostReduction = true;
#endif
		}
		else if ( modNameKey == HackInternetAIUpdateNameKey )
		{
#if defined(RENEWABLE_MONEY_SOURCE_COST_SCALAR)
			renewableMoneySourceCostReduction = true;
#endif
		}
		else if ( modNameKey == OCLUpdateNameKey )
		{
			OCLUpdateModuleData* _data = (OCLUpdateModuleData*)data;
			(void)_data;
			
#if defined(RENEWABLE_MONEY_SOURCE_COST_SCALAR) || EXTRA_MONEY_CRATE_OCL_TRACKING
			// Does this eventually lead to a money crate?
			// ---
			// Contra's TechReinforcementPad used 'FactionOCL' in the INI instead - not handling that case for now
			// (would leave 'm_ocl' here null)
			ObjectCreationList* ocl = (ObjectCreationList*)_data->m_ocl;
			if (ocl != nullptr)
			{
				
				// NOTE - have an unexpected problem: a stackoverflow from endless recursion.
				// See this example from the Contra mod of OCLs:
				/*
				ObjectCreationList OCL_RadiationInfantry CreateObject  ObjectNames = RadiationInfantry
				Object RadiationInfantry Behavior = InstantDeathBehavior ModuleTag_12 OCL        = OCL_FlamingInfantryOnlyDie
					ObjectCreationList OCL_FlamingInfantryOnlyDie CreateObject ObjectNames = FlamingInfantryOnlyDie
					Object FlamingInfantryOnlyDie
							Behavior = InstantDeathBehavior OCL        = OCL_RadiationInfantry
				// Whenever a ThingTemplate checks for OCLs reached by 'OCL_RadiationInfantry', it will endlessly include this cycle.
				// I'll use a set of ObjectCreationList pointers to see if an OCL has ever been parsed through any recursion.
				// Also see something like this from PartitionManager.cpp's 'PartitionManager::getClosestObjects':
				/*
				static Int theIterFlag = 1;	// nonzero, thanks
				++theIterFlag;
				...
				<for loop>
					if (thisMod->friend_getDoneFlag() == theIterFlag)
						continue;
					thisMod->friend_setDoneFlag(theIterFlag);
					<rest of the script>
				*/
				// NOTE - since then, adding a test for InstantDeathBehavior, "m_deathTypes == DEATH_TYPE_FLAGS_ALL"
				// appears to have stopped this and likely any other examples alone for Contra. The recursion issue
				// was encountered before this check was added.
				// Could argue this 'processedOCLList' could be removed as an endless recursion with no-condition death
				// modules should not be possible - wouldn't that lead to some unit in-game that never truly ends?.
				std::set<ObjectCreationList*> processedOCLList;

				if (automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate(ocl, processedOCLList))
				{
#if defined(RENEWABLE_MONEY_SOURCE_COST_SCALAR)
					renewableMoneySourceCostReduction = true;
#endif
#if EXTRA_MONEY_CRATE_OCL_TRACKING
					// this too
					renewableMoneyOCLSource = true;
#endif
				}
			}
#endif
		}
	}

	// Now to reduce the value of the renewable income source.
	// Does not apply to limited units that are likely capable of a lot more like the infantry general black lotus in the Contra mod.
#if defined(RENEWABLE_MONEY_SOURCE_COST_SCALAR)
	if (
		renewableMoneySourceCostReduction &&
		!(_this->isMaxSimultaneousDeterminedBySuperweaponRestriction() || _this->getMaxSimultaneousOfType() == 1)
	)
	{
		// note that tech structures often never define 'BuildCost' (leftover default of 0)
		//MODDD - TODO - could there be a discrepency between value for targeting by special powers & the actual build value of
		// something, so things retain the retail value regardless of post-parsing hackery? Just a thought.
		if (_this->m_buildCost > 0)
		{
			_this->m_buildCost *= (Real)RENEWABLE_MONEY_SOURCE_COST_SCALAR;
		}
	}
#endif
#if EXTRA_MONEY_CRATE_OCL_TRACKING
	if (renewableMoneyOCLSource)
	{
		_this->m_isRenewableMoneyOCLSource = true;
	}
#endif
}

Bool automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate_helper(const ThingTemplate* tt, ObjectCreationList* ocl, std::set<ObjectCreationList*>& processedOCLList)
{
	Bool leadsToMoneyCrate = false;
	// If this has a MoneyCrateCollide, we're good
	if (automaticChangesPostINIParsing_thing_hasMoneyCrateCollide(tt))
	{
		// also let this OCL know to look out for being called from something that can spawn money crates
#if EXTRA_MONEY_CRATE_OCL_TRACKING
		ocl->m_checkForRenewableMoneySource = true;
#endif
		leadsToMoneyCrate = true;
	}

	// check for having some death OCL that leads to a money crate anyway in case OCLs in a deeper chain should be marked
	// (this is needed to recognize the cyberneteics general supply drop zones as renewable money sources anyway)
	Bool outcome = automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate_checkSpawned(tt, processedOCLList);
	if (outcome)
	{
#if EXTRA_MONEY_CRATE_OCL_TRACKING
		ocl->m_checkForRenewableMoneySource = true;
#endif
		leadsToMoneyCrate = true;
	}
	return leadsToMoneyCrate;
}

Bool automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate(ObjectCreationList* ocl, std::set<ObjectCreationList*>& processedOCLList)
{
	// aaAAA more "VS6-didn't-have-this" shenanigans
	//if (processedOCLList.contains(ocl))
	if (processedOCLList.find(ocl) != processedOCLList.end())
	{
		// endless recursion safety - stop, already checked before
		return false;
	}
	processedOCLList.insert(ocl);

	Bool leadsToMoneyCrate = false;

	/*
	++g_callDepth;

	// (this includes lateral calls too, but oh well, at absurd enough of a number even that shouldn't happen)
	// proper way is to send this through every call & increase by 1 before making a recursive call
	if (g_callDepth == 100)
	{
		// how?
		int x;
		x = 4;
	}
	*/

	const ObjectCreationNuggetVector* nuggetList = ocl->getNuggetList();
	ObjectCreationNuggetVector::const_iterator itN;
	for (itN = nuggetList->begin(); itN != nuggetList->end(); ++itN)
	{
		ObjectCreationNugget* nugget = *itN;
		if (nugget->getTypeID() == OCL_NUGGET_TYPE_DELIVER_PAYLOAD)
		{
			const DeliverPayloadNugget* deliverPayloadNugget = (const DeliverPayloadNugget*)nugget;

			// Oh - so it turns out either the typical direct DeliverPayloadNugget fields can be used, or its 'm_data' field
			// (a 'DeliverPayloadData' instance) for additional info that may be directly given to the same 'DeliverPayload'
			// OCL instance - parsing redirects like so in ObjectCreationList.cpp:
			//   p.add(DeliverPayloadData::getFieldParse(), offsetof( DeliverPayloadNugget, m_data ));
			// ...is being this much harder to understand really worth not just copying m_data's field parser table rows to
			// DeliverPayloadData's.  REALLY GUYS.    REALLY.
			const std::vector<DeliverPayloadNugget::Payload>& payloadSpawnOrders = deliverPayloadNugget->getPayload();
			if (payloadSpawnOrders.size() > 0)
			{
				std::vector<DeliverPayloadNugget::Payload>::const_iterator itPayload;
				for (itPayload = payloadSpawnOrders.begin(); itPayload != payloadSpawnOrders.end(); ++itPayload)
				{
					// Actually one 'Payload' at a time is a 'thing-to-spawn' - 'how-many-of-that-to-spawn' pair.
					const DeliverPayloadNugget::Payload& payload = *itPayload;
					if (payload.m_payloadCount > 0 && !payload.m_payloadName.isEmpty())
					{
						const ThingTemplate* payloadTTRef = TheThingFactory->findTemplate( payload.m_payloadName );
						if (payloadTTRef != nullptr)
						{
							if (automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate_helper(payloadTTRef, ocl, processedOCLList))
							{
								leadsToMoneyCrate = true;
							}
						}
					}
				}
			}
			
			// also, check if 'VisiblePayloadTemplateName' was specified
			const DeliverPayloadData& plData = deliverPayloadNugget->getDeliverPayloadData();
			if (!plData.m_visiblePayloadTemplateName.isEmpty())
			{
				const ThingTemplate* payloadTTRef = TheThingFactory->findTemplate( plData.m_visiblePayloadTemplateName );
				if (payloadTTRef != nullptr)
				{
					if (automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate_helper(payloadTTRef, ocl, processedOCLList))
					{
						leadsToMoneyCrate = true;
					}
				}
			}
		}
		else if (nugget->getTypeID() == OCL_NUGGET_TYPE_CREATE_OBJECT)
		{
			const GenericObjectCreationNugget* genericNugget = (const GenericObjectCreationNugget*)nugget;
			const std::vector<AsciiString>& names = genericNugget->getNames();
			std::vector<AsciiString>::const_iterator itNames;
			for (itNames = names.begin(); itNames != names.end(); ++itNames)
			{
				const AsciiString& name = *itNames;
				const ThingTemplate* ttRef = TheThingFactory->findTemplate( name );
				if (ttRef != nullptr)
				{
					if (automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate_helper(ttRef, ocl, processedOCLList))
					{
						leadsToMoneyCrate = true;
					}
				}
			}
		}
	}

	return leadsToMoneyCrate;
}

Bool automaticChangesPostINIParsing_thing_hasMoneyCrateCollide(const ThingTemplate* suspectMoneyCrate)
{
	static NameKeyType MoneyCrateCollideNameKey = NAMEKEY("MoneyCrateCollide");
	Int modIdx;
	const ModuleInfo& mi = suspectMoneyCrate->getBehaviorModuleInfo();
	for (modIdx = 0; modIdx < mi.getCount(); ++modIdx)
	{
		AsciiString modName = mi.getNthName(modIdx);
		//const ModuleData* data = mi.getNthData(modIdx);
		if (modName.isEmpty())
			continue;
		NameKeyType modNameKey = NAMEKEY(modName);

		if( modNameKey == MoneyCrateCollideNameKey )
		{
			// found one
			return true;
		}
	}
	// didn't find any
	return false;
}

// Check: does this thing have 'InstantDeathBehavior' or 'CreateObjectDie' that calls an OCL that then leads to a
// money crate?
/*
Behavior = InstantDeathBehavior ModuleTag_Death01
	DeathTypes       = ALL
	FX               = FX_DropPod2
	OCL              = Cybr_OCL_MoneyDropPod2
End
Behavior = CreateObjectDie ModuleTag_06
	CreationList = Cybr_OCL_MoneyDropPod3
End
*/
Bool automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate_checkSpawned(const ThingTemplate* spawnedByOCL, std::set<ObjectCreationList*>& processedOCLList)
{
	Bool leadsToMoneyCrate = false;
	static NameKeyType InstantDeathBehaviorNameKey = NAMEKEY("InstantDeathBehavior");
	static NameKeyType CreateObjectDieNameKey = NAMEKEY("CreateObjectDie");
	Int modIdx;
	const ModuleInfo& mi = spawnedByOCL->getBehaviorModuleInfo();
	for (modIdx = 0; modIdx < mi.getCount(); ++modIdx)
	{
		AsciiString modName = mi.getNthName(modIdx);
		const ModuleData* data = mi.getNthData(modIdx);
		if (modName.isEmpty())
			continue;
		NameKeyType modNameKey = NAMEKEY(modName);

		if( modNameKey == InstantDeathBehaviorNameKey )
		{
			InstantDeathBehaviorModuleData* _data = (InstantDeathBehaviorModuleData*)data;

			// oh good grief you devs sure loved your inheritance back then didn't`cha
			// TODO - test for this in CreateObjectDie if possible, and test these:
			/*
				m_deathTypes(DEATH_TYPE_FLAGS_ALL),
				m_veterancyLevels(VETERANCY_LEVEL_FLAGS_ALL)

				???
				// all 'exempt' bits must be clear for us to run.
				if( !obj->getStatusBits().testForNone( m_exemptStatus ) )
					return false;

				// all 'required' bits must be set for us to run.
				if( !obj->getStatusBits().testForAll( m_requiredStatus ) )
					return false;
			*/
			if (_data->m_dieMuxData.m_deathTypes == DEATH_TYPE_FLAGS_ALL)
			{
				//_data->m_deathTypes
				// Strange that this allows for multiple OCLs internally when only a "OCL" field is parsed.
				// Maybe there can be multiple "OCL" fields and each one adds another OCL to this list?
				// Regardless, checking all of the list will do.
				OCLVec::iterator itOCL;
				for (itOCL = _data->m_ocls.begin(); itOCL != _data->m_ocls.end(); ++itOCL)
				{
					// recursive call ahoy
					ObjectCreationList* ocl = (ObjectCreationList*)(*itOCL);
					if (ocl != nullptr)
					{
						if (automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate(ocl, processedOCLList))
						{
#if EXTRA_MONEY_CRATE_OCL_TRACKING
							ocl->m_checkForRenewableMoneySource = true;
#endif
							leadsToMoneyCrate = true;
						}
					}
				}
			}
		}
		else if( modNameKey == CreateObjectDieNameKey )
		{
			CreateObjectDieModuleData* _data = (CreateObjectDieModuleData*)data;

			// recursive call ahoy
			ObjectCreationList* ocl = (ObjectCreationList*)_data->m_ocl;
			if (ocl != nullptr)
			{
				if (automaticChangesPostINIParsing_thing_queryLeadsToMoneyCrate(ocl, processedOCLList))
				{
#if EXTRA_MONEY_CRATE_OCL_TRACKING
					ocl->m_checkForRenewableMoneySource = true;
#endif
					leadsToMoneyCrate = true;
				}
			}
		}
	}
	return leadsToMoneyCrate;
}

void automaticChangesPostINIParsing_OCLs()
{
	//std::set<ThingTemplate*> wat;
	std::set<AsciiString> transportProcessedList;

	// Note: why is there also a list of all nuggets ever made (ObjectCreationListStore::m_nuggets)?
	// That seems redundant with the fact that every nugget belongs to some OCL and iterating through all OCLs will
	// already reach every nugget.
	// Note that this should not be confused with 'ObjectCreationList::m_nuggets', which is just the nuggets of one
	// particular OCL ("ocl.getNuggetList()" in 'automaticChangesPostINIParsing_OCL').
	// All I can figure is for 'TheObjectCreationListStore''s deconstructor to be able to handle deleting all the nuggets.
	// Why not leave this up to each OCL item to deal with its own nugget list instead? I have no idea.
	ObjectCreationListMap* oclListMap = TheObjectCreationListStore->getItemListMap();
	ObjectCreationListMap::iterator it;
	for (it = oclListMap->begin(); it != oclListMap->end(); ++it)
	{
		ObjectCreationList& ocl = (*it).second;
		automaticChangesPostINIParsing_OCL(ocl, transportProcessedList);
	}
}
void automaticChangesPostINIParsing_OCL(ObjectCreationList& ocl, std::set<AsciiString>& transportProcessedList)
{
	ObjectCreationNuggetVector* nuggetList = ocl.getNuggetList();
	ObjectCreationNuggetVector::iterator itN;
	for (itN = nuggetList->begin(); itN != nuggetList->end(); ++itN)
	{
		ObjectCreationNugget* nugget = *itN;
		if (nugget->getTypeID() == OCL_NUGGET_TYPE_DELIVER_PAYLOAD)
		{
			const DeliverPayloadNugget* deliverPayloadNugget = (const DeliverPayloadNugget*)nugget;
			const AsciiString& transportName = deliverPayloadNugget->getTransportName();
			if (transportName.isEmpty())
			{
				continue;
			}
			// If this thing has previously been checked, don't do it again
			//if (transportProcessedList.contains(transportName))
			if (transportProcessedList.find(transportName) != transportProcessedList.end())
			{
				continue;
			}
			transportProcessedList.insert(transportName);

			const ThingTemplate* transportTTRef = TheThingFactory->findTemplate( transportName );
			if (transportTTRef != nullptr)
			{
				// If this thing is ever used as a transport and lacks all three of these flags, add FORCEATTACKABLE
				// so that left-clicking it to attack will work. No longer has to be force-attack thanks to another small change elsewhere.
				if (
					!transportTTRef->isKindOf(KINDOF_UNATTACKABLE) &&
					!transportTTRef->isKindOf(KINDOF_SELECTABLE) &&
					!transportTTRef->isKindOf(KINDOF_FORCEATTACKABLE)
				)
				{
					((ThingTemplate*)transportTTRef)->setKindOf(KINDOF_FORCEATTACKABLE, 1);
				}
			}
		}
	}
}


#if CUSTOM_ATTRIBUTE_CHANGES

void automaticWeaponTemplateChanges(WeaponTemplate* _this)
{
	
}

void automaticUpgradeTemplateChanges(UpgradeTemplate* _this)
{
	//_this->m_buildTime *= 1.1f * 1.35f;
	_this->m_buildTime *= 1.1f * 1.80f;
}

// Override the weapon bonuses from the 'GameData.ini' file.
// These better fit other far-reaching stat changes such as health and weapon range.
// See 'automaticGlobalDataChanges' for other 'GameData.ini' overrides.
void automaticGlobalDataWeaponBonusChanges()
{
	// I prefer some changes to the garrisoned bonuses, they seem a bit excessive to me
	// As of retail, garrison bonuses are
	// * Damage: +25%
	// * Range: +33%
	TheWritableGlobalData->m_weaponBonusSet->getWeaponBonus()[WEAPONBONUSCONDITION_GARRISONED].setField(WeaponBonus::DAMAGE, 1.00f);
	TheWritableGlobalData->m_weaponBonusSet->getWeaponBonus()[WEAPONBONUSCONDITION_GARRISONED].setField(WeaponBonus::RANGE, 1.20f);
}

void addCustomWeaponBonuses(const Weapon* _this, const Object* source, WeaponBonus& bonus)
{
#if RTS_ZEROHOUR
	if (source->getTemplate()->isMaxSimultaneousDeterminedBySuperweaponRestriction() || source->getTemplate()->getMaxSimultaneousOfType() == 1)
	{
		// For Contra: super units won't be affected by the health buff.
		// Nor commandos (burtons, etc.)
		// (this is buildings too then)
	}
	else
#endif
	if (source->isKindOf(KINDOF_FS_BASE_DEFENSE))
	{
		// boost the range of base defense weapons
		Real current = bonus.getField(WeaponBonus::RANGE);
		bonus.setField(WeaponBonus::RANGE, current + 0.30f);
	}
	else if (source->isKindOf(KINDOF_INFANTRY))
	{
		// Go ahead and give infantry some constant bonuses since their garrisoned bonus has been toned down.
		Real current = bonus.getField(WeaponBonus::RANGE);
		bonus.setField(WeaponBonus::RANGE, current + 0.25f);
		current = bonus.getField(WeaponBonus::DAMAGE);
		bonus.setField(WeaponBonus::DAMAGE, current + 0.10f);
	}
	else
	{
		// Everything else can get a little more range anyway
		Real current = bonus.getField(WeaponBonus::RANGE);
		bonus.setField(WeaponBonus::RANGE, current + 0.15f);
	}
}

Real getHealthMulti(const ThingTemplate* _this)
{
	if (_this->isKindOf(KINDOF_STRUCTURE))
	{
#if RTS_ZEROHOUR
		if (_this->isKindOf(KINDOF_FS_SUPERWEAPON))
		{
			// superweapons tend to have plenty of health - little boost
			return 1.15f;
		}
		else
#endif
		if(_this->isKindOf(KINDOF_FS_BASE_DEFENSE))
		{
			// get more health
			return 1.5f;
		}
		else
		{
			// buildings otherwise
			return 1.90f;
		}
	}
	else
	{
#if RTS_ZEROHOUR
		// For Contra: super units won't be affected by the health buff.
		// Cheezy way to tell is see if 'maxSimultaneousDeterminedBySuperweaponRestriction' is set for a non-structure.
		// This won't catch things forced to a limit of 1 like commando units, so, suppose that works out nicely.
		if (_this->isMaxSimultaneousDeterminedBySuperweaponRestriction())
		{
			// skip
		}
		else
#endif
		{

			if (!_this->isKindOf(KINDOF_UNATTACKABLE))
			{
				if (_this->isKindOf(KINDOF_DOZER))
				{
					return 1.50f;
				}
				else if (_this->isKindOf(KINDOF_HARVESTER))
				{
					// Be careful with this one, includes combat chinooks in retail generals
					return 1.30f;
				}
				else
				{
					// This is also to make sure this isn't some system/inner-detail thing
					KindOfMaskType tempMask;
					//tempMask.set(KINDOF_INFANTRY);
					tempMask.set(KINDOF_VEHICLE);
					tempMask.set(KINDOF_AIRCRAFT);
					tempMask.set(KINDOF_HUGE_VEHICLE);
					if (_this->isAnyKindOf(tempMask))
					{
						// a non-structure unit (not some weird system/inner-detail thing): have a little more health anyway
						return 1.30f;
					}
					else if (_this->isKindOf(KINDOF_INFANTRY))
					{
						// infantry get a better bonus
						return 1.40f;
					}
				}
			}
		}
	}
	return 1.0f;
}

Real healthAdjustmentFilter(Object* obj, Real healthVal)
{
	Real healthMulti = getHealthMulti(obj->getTemplate());
	Real _healthVal = healthVal;
	if (healthMulti != 1.0f)
	{
		_healthVal *= healthMulti;
	}
	return _healthVal;
}

UnsignedInt specialPowerReloadTimeAdjustmentFilter(const Object* obj, UnsignedInt reloadTime, Bool _isSharedNSync)
{
	UnsignedInt _reloadTime = reloadTime;

	if (!_isSharedNSync)
	{
		// Non-shared special powers, typically superweapons or some single-use + cooldown abilities (ex: radar van scan).
		// 'obj' is always known here and can be used to influence the effect (ex: only change times for superweapons, not radar van scans, etc.).
		// Note that some things have the same appearance as that but are actually tied to firing a weapon instead. Ex: Jarmen Kell's pilot snipe.
#if RTS_ZEROHOUR
		if (obj->isKindOf( KINDOF_FS_SUPERWEAPON ))
		{
			// If the super weapon has a reload time <= 4 minutes, add 2 minutes. Otherwise, add 4 minutes.
			if (reloadTime <= LOGICFRAMES_PER_SECOND * 60 * 4)
			{
				_reloadTime += LOGICFRAMES_PER_SECOND * 60 * 2;
			}
			else
			{
				_reloadTime += LOGICFRAMES_PER_SECOND * 60 * 4;
			}
		}
		else if (obj->getTemplate()->isMaxSimultaneousDeterminedBySuperweaponRestriction())
		{
			// For contra: affect superunit abilities too
			// If the reload time is under 1 minute, add 30 seconds. Probably not too annoying anyway.
			// TODO - check for having infinite range, that's a good indicator of how annoying it could be.
			if (reloadTime < LOGICFRAMES_PER_SECOND * 60 * 1)
			{
				_reloadTime += LOGICFRAMES_PER_SECOND * 30;
			}
			else
			{
				_reloadTime += LOGICFRAMES_PER_SECOND * 60 * 2;
			}
		}
#endif
	}
	else
	{
		// shared ability - ex: spy satelite (just 1 no matter how many command centers you make), any special powers from promotion points.
		// 'obj' is always NULL here, as shared abilities typically stand alone from whatever structure happens to be needed to link to them
		// (in nearly every case, it's the command center anyway).
		// ...
		//_reloadTime += (UnsignedInt)(LOGICFRAMES_PER_SECOND * 60 * 1.5f);
	}
	return _reloadTime;
}

#endif // CUSTOM_ATTRIBUTE_CHANGES


#if CUSTOM_GAME_DATA_CHANGES
void automaticGlobalDataChanges()
{
	// Always have a greater MaxCameraHeight
	if (TheWritableGlobalData->m_maxCameraHeight < 900)
	{
		TheWritableGlobalData->m_maxCameraHeight = 900;
	}
	// Be easier to build in general
	if (TheWritableGlobalData->m_allowedHeightVariationForBuilding < 14.0)
	{
		TheWritableGlobalData->m_allowedHeightVariationForBuilding = 14.0;
	}
	// yes, a whole hour. I want to stare at my breakpoints during a network game maybe.
	TheWritableGlobalData->m_networkPlayerTimeoutTime = 60000 * 60;

	// performance issues when on - forcing off
	// - nevermind, appears to have been fixed
	//TheWritableGlobalData->m_drawEntireTerrain = FALSE;
}
#endif // CUSTOM_GAME_DATA_CHANGES


#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
Real moneyScalarAdjustmentFilter(const Player* player, Bool runMajorCheats)
{
	#if RUN_EXTRA_MONEY_CHEATS
	// The income bonus for AI players can increase over the course of a long game.
	/*
	const UnsignedInt startMin = 8;
	const UnsignedInt endMin = 80;
	const Real startModifier = 1.1f;
	const Real endModifier = 4.0f;
	*/
	const UnsignedInt startMin = 15;
	const UnsignedInt endMin = 60;
	const Real startModifier = 1.15f;
	const Real endModifier = 20.00f;
	#endif

	Real scalar = 1.0f;

	#if RUN_EXTRA_MONEY_CHEATS
	// Note that the logic for steadily-increasing money cheats over time is only for renewable sources to compensate
	// for AI players being less ambitious at building renewable income sources than human players for that point of the game.
	if (player->getPlayerType() == PLAYER_COMPUTER)
	{
		if (runMajorCheats)
		{
			UnsignedInt frame = TheGameLogic->getFrame();
			if (frame <= (LOGICFRAMES_PER_SECOND * 60) * startMin)
			{
				scalar *= startModifier;
			}
			else if(frame <= (LOGICFRAMES_PER_SECOND * 60) * endMin)
			{
				Real fracto = (Real)(frame - ((LOGICFRAMES_PER_SECOND * 60) * startMin)) / (Real)((LOGICFRAMES_PER_SECOND * 60) * (endMin - startMin));
				Real fracto_inv = 1.0f - fracto;
			
				scalar *= endModifier + (startModifier - endModifier) * fracto_inv;
			}
			else
			{
				scalar *= endModifier;
			}
		}
		else
		{
			// non-renewable: lowest bonus only
			scalar *= startModifier;
		}
	}
	#endif

	#if NOOB_MODE
	if (player->getPlayerType() == PLAYER_HUMAN && ThePlayerList->getSlotIndex(player->getPlayerIndex()) == 1)
	{
		scalar *= (Real)NOOB_INCOME_MONEY_SCALAR;
	}
	#endif

	return scalar;
}
#endif

// See if the player should receive extra money on top of the intended amount
UnsignedInt depositMoneyFilter(Player* player, UnsignedInt amountToDeposit, Bool runMajorCheats, MoneySourceType moneySource)
{
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	Real scalar = moneyScalarAdjustmentFilter(player, runMajorCheats);
	if (scalar != 1.0f)
	{
		// Record the money added by cheats (if the scalar is under 1.0, this will be negative - that's technically right)
		// Note that 'RENEWABLE_MONEY_SOURCE_HALF_EFFECTIVE''s reduction is not part of this.
		// ---
		// Getting the difference between the 'amountToDepositFinal' and original input to avoid the risk of more rounding error
		//Int moneyAddedByScalarCheats = (Int)(((Real)amountToDeposit) * (scalar - 1.0f));
		Int amountToDepositFinal = (Int)(((Real)amountToDeposit) * scalar);
		Int moneyAddedByScalarCheats = amountToDepositFinal - (Int)amountToDeposit;
		player->getMoney()->deposit((UnsignedInt)amountToDepositFinal);
		player->getScoreKeeper()->addMoneyEarned(amountToDeposit, moneySource);
		player->getScoreKeeper()->addMoneyEarned(moneyAddedByScalarCheats, MONEYSOURCE_SCALARCHEATS);
		return (UnsignedInt)amountToDepositFinal;
	}
#endif
	
	player->getMoney()->deposit(amountToDeposit);
	player->getScoreKeeper()->addMoneyEarned(amountToDeposit, moneySource);
	return amountToDeposit;
}

// Variant for boost upgrade to offer another boost/source pair
UnsignedInt depositMoneyFilter(Player* player, UnsignedInt amountToDeposit, UnsignedInt amountToDepositBoost, Bool runMajorCheats, MoneySourceType moneySource, MoneySourceType moneySourceBoost)
{
	Int amountToDepositBase = amountToDeposit + amountToDepositBoost;
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	Real scalar = moneyScalarAdjustmentFilter(player, runMajorCheats);
	if (scalar != 1.0f)
	{
		Int amountToDepositFinal = (Int)(((Real)amountToDepositBase) * scalar);
		Int moneyAddedByScalarCheats = amountToDepositFinal - (Int)amountToDeposit;
		player->getMoney()->deposit((UnsignedInt)amountToDepositFinal);
		player->getScoreKeeper()->addMoneyEarned(amountToDeposit, moneySource);
		player->getScoreKeeper()->addMoneyEarned(amountToDepositBoost, moneySourceBoost);
		player->getScoreKeeper()->addMoneyEarned(moneyAddedByScalarCheats, MONEYSOURCE_SCALARCHEATS);
		return (UnsignedInt)amountToDepositFinal;
	}
#endif

	player->getMoney()->deposit(amountToDepositBase);
	player->getScoreKeeper()->addMoneyEarned(amountToDeposit, moneySource);
	player->getScoreKeeper()->addMoneyEarned(amountToDepositBoost, moneySourceBoost);
	return amountToDepositBase;
}

// Variant to specify a scalar to apply in addition to the one decided by the presence of cheats for this player.
// This additional 'preScalar' is not considered part of cheats - its impact won't be part of the
// 'MONEYSOURCE_SCALARCHEATS' money bucket.
UnsignedInt depositMoneyFilter(Player* player, UnsignedInt amountToDeposit, Bool runMajorCheats, Real preScalar, MoneySourceType moneySource)
{
	UnsignedInt amountToDepositAdj = (UnsignedInt)((Real)amountToDeposit * preScalar);
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	Real scalar = moneyScalarAdjustmentFilter(player, runMajorCheats);
	if (scalar != 1.0f)
	{
		// multiply 'amountToDeposit' as a Real with the scalars together before casting to reduce rounding error
		Int amountToDepositFinal = (Int)(((Real)amountToDeposit) * (preScalar * scalar));
		Int moneyAddedByScalarCheats = amountToDepositFinal - amountToDepositAdj;
		player->getMoney()->deposit((UnsignedInt)amountToDepositFinal);
		player->getScoreKeeper()->addMoneyEarned(amountToDepositAdj, moneySource);
		player->getScoreKeeper()->addMoneyEarned(moneyAddedByScalarCheats, MONEYSOURCE_SCALARCHEATS);
		return (UnsignedInt)amountToDepositFinal;
	}
#endif
	
	player->getMoney()->deposit(amountToDepositAdj);
	player->getScoreKeeper()->addMoneyEarned(amountToDepositAdj, moneySource);
	return amountToDepositAdj;
}

// Variant for boost upgrade to offer another boost/source pair
UnsignedInt depositMoneyFilter(Player* player, UnsignedInt amountToDeposit, UnsignedInt amountToDepositBoost, Bool runMajorCheats, Real preScalar, MoneySourceType moneySource, MoneySourceType moneySourceBoost)
{
	Int amountToDepositBase = amountToDeposit + amountToDepositBoost;
	UnsignedInt amountToDepositAdj = (UnsignedInt)((Real)amountToDeposit * preScalar);
	UnsignedInt amountToDepositBaseAdj = (UnsignedInt)((Real)amountToDepositBase * preScalar);
	// determining the boost-adjusted amount this way avoids rounding error (normal + boost must match the total deposited)
	UnsignedInt amountToDepositBoostAdj = amountToDepositBaseAdj - amountToDepositAdj;
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	Real scalar = moneyScalarAdjustmentFilter(player, runMajorCheats);
	if (scalar != 1.0f)
	{
		Int amountToDepositFinal = (Int)(((Real)amountToDepositBase) * (preScalar * scalar));
		Int moneyAddedByScalarCheats = amountToDepositFinal - amountToDepositBaseAdj;
		player->getMoney()->deposit((UnsignedInt)amountToDepositFinal);
		player->getScoreKeeper()->addMoneyEarned(amountToDepositAdj, moneySource);
		player->getScoreKeeper()->addMoneyEarned(amountToDepositBoostAdj, moneySourceBoost);
		player->getScoreKeeper()->addMoneyEarned(moneyAddedByScalarCheats, MONEYSOURCE_SCALARCHEATS);
		return (UnsignedInt)amountToDepositFinal;
	}
#endif
	
	player->getMoney()->deposit(amountToDepositBaseAdj);
	player->getScoreKeeper()->addMoneyEarned(amountToDepositAdj, moneySource);
	player->getScoreKeeper()->addMoneyEarned(amountToDepositBoostAdj, moneySourceBoost);
	return amountToDepositBaseAdj;
}

// Variants of above (non/preScalar & non/boost) to let the user specify whether to do the 'deposit'/'addMoneyEarned' calls.
// Present because 'AudoDepositUpdate.cpp' allows something to specify 'modData->m_isActualMoney' as 'false' to
// avoid making money for any players (still shows a number as though it did) - effectively making it a dummy.
// This will still apply cheats (if applicable) to get the adjusted value to make that displaed number accurate for what
// it would have been regardless.
// If changes are made to above, replace these with fresh copies and add the conditional checks back in.
UnsignedInt depositMoneyFilterConditional(Player* player, UnsignedInt amountToDeposit, Bool runMajorCheats, MoneySourceType moneySource, Bool canApplyMoney)
{
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	Real scalar = moneyScalarAdjustmentFilter(player, runMajorCheats);
	if (scalar != 1.0f)
	{
		Int amountToDepositFinal = (Int)(((Real)amountToDeposit) * scalar);
		Int moneyAddedByScalarCheats = amountToDepositFinal - (Int)amountToDeposit;
		if (canApplyMoney)
		{
			player->getMoney()->deposit((UnsignedInt)amountToDepositFinal);
			player->getScoreKeeper()->addMoneyEarned(amountToDeposit, moneySource);
			player->getScoreKeeper()->addMoneyEarned(moneyAddedByScalarCheats, MONEYSOURCE_SCALARCHEATS);
		}
		return (UnsignedInt)amountToDepositFinal;
	}
#endif

	if (canApplyMoney)
	{
		player->getMoney()->deposit(amountToDeposit);
		player->getScoreKeeper()->addMoneyEarned(amountToDeposit, moneySource);
	}
	return amountToDeposit;
}

UnsignedInt depositMoneyFilterConditional(Player* player, UnsignedInt amountToDeposit, UnsignedInt amountToDepositBoost, Bool runMajorCheats, MoneySourceType moneySource, MoneySourceType moneySourceBoost, Bool canApplyMoney)
{
	Int amountToDepositBase = amountToDeposit + amountToDepositBoost;
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	Real scalar = moneyScalarAdjustmentFilter(player, runMajorCheats);
	if (scalar != 1.0f)
	{
		Int amountToDepositFinal = (Int)(((Real)amountToDepositBase) * scalar);
		Int moneyAddedByScalarCheats = amountToDepositFinal - (Int)amountToDeposit;
		if (canApplyMoney)
		{
			player->getMoney()->deposit((UnsignedInt)amountToDepositFinal);
			player->getScoreKeeper()->addMoneyEarned(amountToDeposit, moneySource);
			player->getScoreKeeper()->addMoneyEarned(amountToDepositBoost, moneySourceBoost);
			player->getScoreKeeper()->addMoneyEarned(moneyAddedByScalarCheats, MONEYSOURCE_SCALARCHEATS);
		}
		return (UnsignedInt)amountToDepositFinal;
	}
#endif

	if (canApplyMoney)
	{
		player->getMoney()->deposit(amountToDepositBase);
		player->getScoreKeeper()->addMoneyEarned(amountToDeposit, moneySource);
		player->getScoreKeeper()->addMoneyEarned(amountToDepositBoost, moneySourceBoost);
	}
	return amountToDepositBase;
}

UnsignedInt depositMoneyFilterConditional(Player* player, UnsignedInt amountToDeposit, Bool runMajorCheats, Real preScalar, MoneySourceType moneySource, Bool canApplyMoney)
{
	UnsignedInt amountToDepositAdj = (UnsignedInt)((Real)amountToDeposit * preScalar);
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	Real scalar = moneyScalarAdjustmentFilter(player, runMajorCheats);
	if (scalar != 1.0f)
	{
		Int amountToDepositFinal = (Int)(((Real)amountToDeposit) * (preScalar * scalar));
		Int moneyAddedByScalarCheats = amountToDepositFinal - amountToDepositAdj;
		if (canApplyMoney)
		{
			player->getMoney()->deposit((UnsignedInt)amountToDepositFinal);
			player->getScoreKeeper()->addMoneyEarned(amountToDepositAdj, moneySource);
			player->getScoreKeeper()->addMoneyEarned(moneyAddedByScalarCheats, MONEYSOURCE_SCALARCHEATS);
		}
		return (UnsignedInt)amountToDepositFinal;
	}
#endif

	if (canApplyMoney)
	{
		player->getMoney()->deposit(amountToDepositAdj);
		player->getScoreKeeper()->addMoneyEarned(amountToDepositAdj, moneySource);
	}
	return amountToDepositAdj;
}

UnsignedInt depositMoneyFilterConditional(Player* player, UnsignedInt amountToDeposit, UnsignedInt amountToDepositBoost, Bool runMajorCheats, Real preScalar, MoneySourceType moneySource, MoneySourceType moneySourceBoost, Bool canApplyMoney)
{
	Int amountToDepositBase = amountToDeposit + amountToDepositBoost;
	UnsignedInt amountToDepositAdj = (UnsignedInt)((Real)amountToDeposit * preScalar);
	UnsignedInt amountToDepositBaseAdj = (UnsignedInt)((Real)amountToDepositBase * preScalar);
	UnsignedInt amountToDepositBoostAdj = amountToDepositBaseAdj - amountToDepositAdj;
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	Real scalar = moneyScalarAdjustmentFilter(player, runMajorCheats);
	if (scalar != 1.0f)
	{
		Int amountToDepositFinal = (Int)(((Real)amountToDepositBase) * (preScalar * scalar));
		Int moneyAddedByScalarCheats = amountToDepositFinal - amountToDepositBaseAdj;
		if (canApplyMoney)
		{
			player->getMoney()->deposit((UnsignedInt)amountToDepositFinal);
			player->getScoreKeeper()->addMoneyEarned(amountToDepositAdj, moneySource);
			player->getScoreKeeper()->addMoneyEarned(amountToDepositBoostAdj, moneySourceBoost);
			player->getScoreKeeper()->addMoneyEarned(moneyAddedByScalarCheats, MONEYSOURCE_SCALARCHEATS);
		}
		return (UnsignedInt)amountToDepositFinal;
	}
#endif

	if (canApplyMoney)
	{
		player->getMoney()->deposit(amountToDepositBaseAdj);
		player->getScoreKeeper()->addMoneyEarned(amountToDepositAdj, moneySource);
		player->getScoreKeeper()->addMoneyEarned(amountToDepositBoostAdj, moneySourceBoost);
	}
	return amountToDepositBaseAdj;
}

#if RUN_BUILD_TIME_CHEATS || NOOB_MODE
Int buildTimeAdjustmentFilter(const Player* player, const ThingTemplate* tt)
{
	#if RUN_BUILD_TIME_CHEATS
	// AI players can build faster over the course of a long game.
	/*
	const UnsignedInt startMin = 8;
	const UnsignedInt endMin = 80;
	const Real startModifier = 1.00f;
	const Real endModifier = 0.76f;
	*/
	const UnsignedInt startMin = 15;
	const UnsignedInt endMin = 60;
	//const Real startModifier = 1.00f;
	//const Real endModifier = 0.70f;
	const Real startModifier = 1.00f;
	const Real endModifier = 0.75f;
	#endif

	Int _buildTime = tt->getBuildTime() * LOGICFRAMES_PER_SECOND;

	#if RUN_BUILD_TIME_CHEATS
	if (player->getPlayerType() == PLAYER_COMPUTER)
	{
		// only for units, not structures - no one wants to deal with faster building replacements
		if (!tt->isKindOf(KINDOF_STRUCTURE))
		{
			UnsignedInt frame = TheGameLogic->getFrame();
			if (frame <= (LOGICFRAMES_PER_SECOND * 60) * startMin)
			{
				_buildTime = (Int)((Real)_buildTime * startModifier);
			}
			else if(frame <= (LOGICFRAMES_PER_SECOND * 60) * endMin)
			{
				Real fracto = (Real)(frame - ((LOGICFRAMES_PER_SECOND * 60) * startMin)) / (Real)((LOGICFRAMES_PER_SECOND * 60) * (endMin - startMin));
				Real fracto_inv = 1.0f - fracto;
			
				_buildTime = (Int)((Real)_buildTime * (endModifier + (startModifier - endModifier) * fracto_inv));
			}
			else
			{
				_buildTime = (Int)((Real)_buildTime * endModifier);
			}
		}
	}
	#endif

	#if NOOB_MODE
	if (player->getPlayerType() == PLAYER_HUMAN && ThePlayerList->getSlotIndex(player->getPlayerIndex()) == 1)
	{
		_buildTime = (Int)((Real)_buildTime * (Real)NOOB_BUILD_TIME_SCALAR);
	}
	#endif

	return _buildTime;
}
#endif // RUN_BUILD_TIME_CHEATS

#if RUN_PLAYER_PROMOTION_EXPERIENCE_RATE_CHEATS || NOOB_MODE
Real playerPromotionExperienceRateFilter(const Player* player, Real expRateModifier)
{
	#if RUN_PLAYER_PROMOTION_EXPERIENCE_RATE_CHEATS
	// AI players receive more experience toward promotions (not individual unit veterancy) per kill over the course of a long game.
	const UnsignedInt startMin = 12;
	const UnsignedInt endMin = 60;
	const Real startModifier = 1.00f;
	const Real endModifier = 1.75f;
	#endif

	Real _expRateModifier = expRateModifier;

	#if RUN_PLAYER_PROMOTION_EXPERIENCE_RATE_CHEATS
	if (player->getPlayerType() == PLAYER_COMPUTER)
	{
		UnsignedInt frame = TheGameLogic->getFrame();
		if (frame <= (LOGICFRAMES_PER_SECOND * 60) * startMin)
		{
			_expRateModifier *= startModifier;
		}
		else if(frame <= (LOGICFRAMES_PER_SECOND * 60) * endMin)
		{
			Real fracto = (Real)(frame - ((LOGICFRAMES_PER_SECOND * 60) * startMin)) / (Real)((LOGICFRAMES_PER_SECOND * 60) * (endMin - startMin));
			Real fracto_inv = 1.0f - fracto;
			
			_expRateModifier *= endModifier + (startModifier - endModifier) * fracto_inv;
		}
		else
		{
			_expRateModifier *= endModifier;
		}
	}
	#endif

	#if NOOB_MODE
	if (player->getPlayerType() == PLAYER_HUMAN && ThePlayerList->getSlotIndex(player->getPlayerIndex()) == 1)
	{
		_expRateModifier *= (Real)NOOB_PLAYER_PROMOTION_EXPERIENCE_RATE_SCALAR;
	}
	#endif

	return _expRateModifier;
}
#endif // RUN_PLAYER_PROMOTION_EXPERIENCE_RATE_CHEATS


int printBuffer_spaces(char* dest, int spacesToAdd)
{
	int i;
	for (i = 0; i < spacesToAdd; ++i)
	{
		dest[i] = ' ';
	}
	return spacesToAdd;
}

// NOTE - does not handle null char termination!
int printBuffer_text(char* dest, size_t destCount, const char* val)
{
	int res = 0;
	int len = strlen(val);
	if (len > destCount)
	{
		// clip it
		len = destCount;
	}
	memcpy(&dest[res], val, len);
	res += len;
	return res;
}

// NOTE - does not handle null char termination!
int printBuffer_textLeftAligned(char* dest, size_t destCount, const char* val)
{
	int res = 0;
	res += printBuffer_text(&dest[0], destCount, val);

	if (res < destCount)
	{
		// add padding spaces to the right
		res += printBuffer_spaces(&dest[res], destCount - res);
	}
	return res;
}

int printBuffer_numberRightAlignedSep(char* dest, size_t destCount, int val)
{
	int res = 0;
	static char buf[64];
	bool isNeg = (val < 0);
	if (isNeg)
	{
		// invert to remove the minus sign from the resulting as-string conversion
		val *= -1;
	}
	//std::string val_s = std::to_string(val);
	snprintf(buf, 64, "%d", val);
	int len = strlen(buf);
	// 1 less to the len if negative - want digits, don't include the minus
	int digitGroupCount = (int)ceil((float)len / 3.0f);
	int digitsThisGroup;
	int commasToAdd = digitGroupCount - 1;
	int adjustedLen = len + commasToAdd;

	int modTest = len % 3;
	if (modTest == 0) modTest += 3;
	digitsThisGroup = modTest;
	
	int read = 0;
	int skip = 0;

	if (isNeg)
	{
		// add 1 for the minus sign
		++adjustedLen;
	}
	
	if (adjustedLen > destCount)
	{
		// uh-oh, not enough space - cut from the left
		skip = adjustedLen - destCount;
		adjustedLen = destCount;
	}
	
	res += printBuffer_spaces(&dest[res], destCount - adjustedLen);

	if (skip > 0)
	{
		// If the resulting number is too many characters (minus sign, commas included), advance 'read' to start reading
		// a later character of the as-string conversion of the number to print.
		// Also shrink vars that affect the final printout like 'digitsThisGroup' and 'digitGroupCount' as more of the
		// number to use for the printout is skipped.
		// This mirrors the final printout loop below.
		// (the minus sign isn't part of this - its space is already factored in as the 1st character if present)
		int digitGroupCount_mem = digitGroupCount;
		int i;
		for (i = 0; i < digitGroupCount_mem; ++i)
		{
			int digitsThisGroup_mem = digitsThisGroup;
			int i2;
			for (i2 = 0; i2 < digitsThisGroup_mem; ++i2)
			{
				--skip;
				++read;
				--digitsThisGroup;
				if (skip == 0)break;
			}
			if (skip == 0)break;
			
			// reset
			digitsThisGroup = 3;

			if (i < digitGroupCount_mem - 1)
			{
				--skip;
				// this skipped comma doesn't correspond to a character in the received number so don't increment 'read' here
				if (skip == 0)
				{
					// still reduce this before leaving!
					--digitGroupCount;
					break;
				}
			}
			--digitGroupCount;
		}
	}

	if (isNeg)
	{
		// minus sign
		dest[res] = '-';
		++res;
	}
	
	int i;
	for (i = 0; i < digitGroupCount; ++i)
	{
		int i2;
		for (i2 = 0; i2 < digitsThisGroup; ++i2)
		{
			dest[res] = buf[read];
			++res;
			++read;
		}
		// any other groups after this have all 3 digits
		digitsThisGroup = 3;
		if (i < digitGroupCount - 1)
		{
			// if another group is coming after, add a group comma
			dest[res] = ',';
			++res;
		}
	}
	return res;
}

// Get a 'hours:minutes:seconds' string from a frame number given 30 frames = 1 sec
AsciiString frameNumberToTimeString(UnsignedInt frame)
{
	UnsignedInt hours = frame / (LOGICFRAMES_PER_SECOND * 60 * 60);
	UnsignedInt minutes = (frame % (LOGICFRAMES_PER_SECOND * 60 * 60)) / (LOGICFRAMES_PER_SECOND * 60);
	UnsignedInt seconds = (frame % (LOGICFRAMES_PER_SECOND * 60)) / (LOGICFRAMES_PER_SECOND);
	AsciiString toRet;
	toRet.format(
		"%u:%02u:%02u",
		hours,
		minutes,
		seconds
	);
	return toRet;
}

// Print out some extra stats per player, since editing ScoreScreen.wnd looks like a nightmarish task. Consider this for
// not just the retail game ScoreScreen.wnd, but also for any mod you come across.
// Namely, goes into more detail on the money received per player: from supply collection, renewable sources like
// supply drop zones or oil derricks, various forms of cheats, etc.
void extraScoreScreenStatsPrintOut()
{
	FILE* outputFile = fopen("ScoreScreenExtra.txt", "a");
	printTimeStamp_noFrame(outputFile);

	AsciiString gameStr;
	const char* gameBuildStr;
	const char* gameTypeStr;
	AsciiString gameDurationStr = frameNumberToTimeString(TheGameLogic->getFrame());

#if FORCE_GAME_CONTEXT == FGC_NONE
	gameBuildStr = "Standard";
#elif FORCE_GAME_CONTEXT == FGC_GENERALS_CHALLENGE
	gameBuildStr = "Generals Challenge";
#elif FORCE_GAME_CONTEXT == FGC_CAMPAIGN
	gameBuildStr = "Campaign";
#else
	// ???
	gameBuildStr = "Unknown";
#endif

	// TODO - test this - is checking 'TheGameLogic->isInInternetGame()', etc. still ok here when
	// "TheGameLogic->isInReplayGame()" is the case?
	// Feels like there should be no reason for a replay to be unable to get that info too.
	if(TheGameLogic->isInInternetGame())
	{
		gameTypeStr = "Internet";
	}
	else if( TheGameLogic->isInLanGame())
	{
		gameTypeStr = "LAN";
	}
	else if( TheGameLogic->isInSkirmishGame())
	{
		gameTypeStr = "Skirmish";
	}
	else
	{
		gameTypeStr = "Campaign";
	}

	const char* mapName = getFileName(TheGlobalData->m_mapName.str());
	gameStr.format("Map:%s - Build:%s - GameType:%s - ElapsedTime:%s", mapName, gameBuildStr, gameTypeStr, gameDurationStr.str());
	if (TheGameLogic->isInReplayGame())
	{
		gameStr.concat(" - Replay");
	}

	fprintf(outputFile, " - %s\n", gameStr.str());
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	fprintf(outputFile, "Player                        |Start         |Gathered      |Gathered UP   |Renewable     |Renewable UP  |Renewable Tech|Renewable T UP|Bounty        |Salvage       |Steal         |Steal (Victim)|Scalar Cheats |Map Script    |Total Collect |Spent         \n");
#else
	fprintf(outputFile, "Player                        |Start         |Gathered      |Gathered UP   |Renewable     |Renewable UP  |Renewable Tech|Renewable T UP|Bounty        |Salvage       |Steal         |Steal (Victim)|Map Script    |Total Collect |Spent         \n");
#endif

	// List the slot players first, like 'grabMultiPlayerInfo' without sorting by score.
	for( Int i = 0; i < ThePlayerList->m_slotSoftMax; ++i)
	{
		// Could filter out observer / unoccupied players with '_requireParticipant', but the score screens hows those so
		// may as well list them as a row here too
		Player *player = ThePlayerList->getPlayerFromSlotIndex(i);
		if (player != nullptr)
		{
			extraScoreScreenStatsPrintOut_player(outputFile, player);
		}
	}

	// For the rest of the computer players baked into the map, using the same checks as ScoreScreen.cpp's 'populateComputerPlayerInfoForGeneralsChallenge'
	//MODDD - TODO - if this generates noise (obviously irrelevant players grabbed) for FGC_NONE skirmish/LAN games, consider '#if' checks for FGC's
	int i;
	for (i = 0; i < ThePlayerList->getPlayerCount(); ++i)
	{
		Player* player = ThePlayerList->getNthPlayer(i);
		if (ThePlayerList->getSlotIndex(player->getPlayerIndex()) != -1)
		{
			continue;
		}

		if (player->isPlayerObserver())
		{
			continue;
		}

		// Exclude neutral and civilian players.
		if (ThePlayerList->isPlayerUnaffiliated(player))
		{
			continue;
		}

		if (!player->getListInScoreScreen())
		{
			continue;
		}

		if (player->getPlayerType() == PLAYER_HUMAN)
		{
			continue;
		}

		// All passed - add this player
		extraScoreScreenStatsPrintOut_player(outputFile, player);
	}

	// add a separating blank line for neatness
	fputs("\n", outputFile);

	fclose(outputFile);
}

void extraScoreScreenStatsPrintOut_player(FILE* outputFile, Player* player)
{
	ScoreKeeper *scoreKpr = player->getScoreKeeper();
	if (!scoreKpr)
		return;

	AsciiString playerNameDisp;
	playerNameDisp.translate(player->getPlayerDisplayName());

	//const char* playerName = TheNameKeyGenerator->keyToName(player->getPlayerNameKey()).str();
	AsciiString playerNameToPrint;
	playerNameToPrint.set(playerNameDisp);

	// Typically, printing the raw player names for slot players isn't interesting because they use auto-generated names
	// like "player0", "player1", etc.
	// For FGC_CAMPAIGN (or a campaign map otherwise), this isn't the case so seeing the map player used for a particular
	// slot may be more interesting.
	Bool canPrintRawPlayerNameForSlotPlayers = rts_extra::isCurrentGameCampaign();

	if (!canPrintRawPlayerNameForSlotPlayers && ThePlayerList->getSlotIndex(player->getPlayerIndex()) != -1)
	{
		// ignored
	}
	else
	{
		// baked-in player being printed out - only include the raw name if it's different from the display name
		const char* playerName = TheNameKeyGenerator->keyToName(player->getPlayerNameKey()).str();
		
		if (playerNameDisp.compare(playerName) == 0)
		{
			// match - nothing to do
		}
		else
		{
			// include the raw name in parenthesis
			playerNameToPrint.concat("(");
			playerNameToPrint.concat(playerName);
			playerNameToPrint.concat(")");
		}
	}

	// Note that start money includes what's added by the 'noob' bonus - I don't think that's worthwhile to separate out.
	Int moneyStartedWith = scoreKpr->getMoneyStartedWith();
	// Money from gathering supplies from finite sources - namely supply piles and supply docks. Also counts touching money
	// crates found on the map or otherwise not called-in by a structure (ex: supply drop zone).
	Int moneyFromGathering = scoreKpr->getMoneyReceived(MONEYSOURCE_GATHERED);
	Int moneyFromGatheringBoost = scoreKpr->getMoneyReceived(MONEYSOURCE_GATHERED_BOOST);
	// Money from continual sources of money - namely black markets, hackers, supply drop zone crates, oil derricks.
	// Going to go ahead and count the initial capture bonus for oil derricks too.
	Int moneyFromRenewable = scoreKpr->getMoneyReceived(MONEYSOURCE_RENEWABLE);
	Int moneyFromRenewableBoost = scoreKpr->getMoneyReceived(MONEYSOURCE_RENEWABLE_BOOST);
	Int moneyFromRenewableTech = scoreKpr->getMoneyReceived(MONEYSOURCE_RENEWABLE_TECH);
	Int moneyFromRenewableTechBoost = scoreKpr->getMoneyReceived(MONEYSOURCE_RENEWABLE_TECH_BOOST);
	// Money from the GLA bounty effect - going to be 0 if this is non-applicable or a promotion point is never spent on that.
	Int moneyFromBounty = scoreKpr->getMoneyReceived(MONEYSOURCE_BOUNTY);
	// Money from collecting salvage piles that happened to give money, also likely only applicable for GLA players.
	Int moneyFromSalvage = scoreKpr->getMoneyReceived(MONEYSOURCE_SALVAGE);
	// Money from stealing from other players - cash hack support power, black lotus cash steal, or saboteur infiltration
	Int moneyFromSteal = scoreKpr->getMoneyReceived(MONEYSOURCE_STEAL);
	// Money lost to other players using steal abilities on me (not factored into the total in the score screen nor generated text file)
	Int moneyFromStealVictim = scoreKpr->getMoneyReceived(MONEYSOURCE_STEAL_VICTIM);
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	// Money from scalar cheats applied to money collected over the course of a game from the 'noob' bonus or AI-player cheats.
	// This is distinct from money granted by map script - see below.
	Int moneyFromScalarCheats = scoreKpr->getMoneyReceived(MONEYSOURCE_SCALARCHEATS);
#endif
	// Money granted to a player after the start of a game by map script, typically very often to make it seem as though
	// a computer player has infinite money - a lot of mods do this for generals challenge maps and possibly skirmish AI
	// for harder difficulties.
	Int moneyFromMapScript = scoreKpr->getMoneyReceived(MONEYSOURCE_MAPSCRIPT);
	// Total money collected over the course of a game - sum of everything above except for start money.
	// Note that the 'supplied collected' shown in the score screen still doesn't factor in money granted by map scripts even after the start of a game, so adding it in here.
	// Also, as said above, the 'steal (victim)' column isn't included in this - it's just neat to see how much was stolen from a player.
	Int moneyTotalCollected = scoreKpr->getTotalMoneyCollected();
	Int moneySpent = scoreKpr->getTotalMoneySpent();

	//FILE* outputFile = fopen("ScoreScreenExtra.txt", "a");
	static char lineBuffer[512];
	int off = 0;

	off += printBuffer_textLeftAligned(&lineBuffer[off], 30, playerNameToPrint.str());

	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyStartedWith);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromGathering);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromGatheringBoost);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromRenewable);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromRenewableBoost);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromRenewableTech);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromRenewableTechBoost);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromBounty);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromSalvage);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromSteal);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromStealVictim);
#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromScalarCheats);
#endif
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyFromMapScript);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneyTotalCollected);
	off += printBuffer_text(&lineBuffer[off], 1, "|");
	off += printBuffer_numberRightAlignedSep(&lineBuffer[off], 14, moneySpent);

	off += printBuffer_text(&lineBuffer[off], 1, "\n");

	fputs(lineBuffer, outputFile);

	//fclose(outputFile);
}


//MODDD - DEBUG
Bool checkIfObjInDestroyList(const Object* objCheck)
{
	for( ObjectPointerListIterator iterator = TheGameLogic->m_objectsToDestroy.begin(); iterator != TheGameLogic->m_objectsToDestroy.end(); iterator++ )
	{
		Object* currentObject = (*iterator);
		if (currentObject == objCheck)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// Note that this doesn't end in a newline character.
// Includes the frame number of the game at the time.
void printTimeStamp(FILE* outputFile)
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	fprintf
	(
		outputFile,
		"%u-%02u-%02u %02u:%02u:%02u.%03u Frame:%u",
		lt.wYear,
		lt.wMonth,
		lt.wDay,
		lt.wHour,
		lt.wMinute,
		lt.wSecond,
		lt.wMilliseconds,
		TheGameLogic->getFrame()
	);
}

// Similar to above but leaves out the frame number
void printTimeStamp_noFrame(FILE* outputFile)
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	fprintf
	(
		outputFile,
		"%u-%02u-%02u %02u:%02u:%02u.%03u",
		lt.wYear,
		lt.wMonth,
		lt.wDay,
		lt.wHour,
		lt.wMinute,
		lt.wSecond,
		lt.wMilliseconds
	);
}

// Print the most basic info to tell what this object is at a glance. That is, template name (americaVehicle-whatever)
// with ID to tell if other places are referring to the exact same object.
// Note that this doesn't end in a newline character.
void printObjectIdentifyingInfo(FILE* outputFile, const Object* objToPrint)
{
	fprintf(outputFile, "%s:%d", objToPrint->getTemplate()->getName().str(), objToPrint->getID());
}

// really, may as well turn that into a utility that gives the string directly
AsciiString getObjectIdentifyingInfoString(const Object* objToPrint)
{
	AsciiString identifier;
	identifier.format("%s:%d", objToPrint->getTemplate()->getName().str(), objToPrint->getID());
	return identifier;
}

void printDeletionCriticalInfo(FILE* outputFile, const Object* objToPrint)
{
	printDeletionCriticalInfo(outputFile, objToPrint, nullptr);
}

void printDeletionCriticalInfo(FILE* outputFile, const Object* objToPrint, const char* extraHeading)
{
	Bool isDestroyed = objToPrint->testStatus(OBJECT_STATUS_DESTROYED);
	Bool isEffectivelyDead = objToPrint->isEffectivelyDead();
	Bool isObjThruInDestroyedList = checkIfObjInDestroyList(objToPrint);

	if (extraHeading != nullptr)
	{
		fprintf(outputFile, "printDeletionCriticalInfo - %s:", extraHeading);
	}
	else
	{
		fprintf(outputFile, "printDeletionCriticalInfo - ");
	}

	printObjectIdentifyingInfo(outputFile, objToPrint);
	fprintf(outputFile, " isDestroyed: %d isEffectivelyDead: %d isObjThruInDestroyedList: %d", isDestroyed, isEffectivelyDead, isObjThruInDestroyedList);
	if (objToPrint->getBodyModule())
	{
		fprintf(outputFile, " health: %.2f", objToPrint->getBodyModule()->getHealth());
	}
	else
	{
		fprintf(outputFile, " health: N/A (no body module)");
	}
	fputs("\n", outputFile);
}

void printItemsInContainedList(FILE* outputFile, const Object* objContainer)
{
	const ContainedItemsList* conList = nullptr;
	ContainModuleInterface* contain = objContainer->getContain();
	if (contain == nullptr)
	{
		fputs("printItemsInContainedList - ERROR - 'objContainer' ", outputFile);
		printObjectIdentifyingInfo(outputFile, objContainer);
		fputs(" has no 'contain' module\n", outputFile);
		return;
	}

	fputs("printItemsInContainedList - ", outputFile);
	printObjectIdentifyingInfo(outputFile, objContainer);
	fputs("\n", outputFile);
	fputs("------------\n", outputFile);

	conList = objContainer->getContain()->getContainedItemsList();
	if (conList->empty())
	{
		// just say that?
		fputs(" (empty)\n", outputFile);
	}
	else
	{
		for(ContainedItemsList::const_iterator it = conList->begin(); it != conList->end(); ++it)
		{
			Object* containedObj = *it;
			printDeletionCriticalInfo(outputFile, containedObj);
		}
	}
	fputs("------------\n", outputFile);
}

void printItemsInContainedList(FILE* outputFile, const Object* objContainer, const Object* objToLookFor)
{
	const ContainedItemsList* conList = nullptr;
	ContainModuleInterface* contain = objContainer->getContain();
	if (contain == nullptr)
	{
		fputs("printItemsInContainedList - ERROR - 'objContainer' ", outputFile);
		printObjectIdentifyingInfo(outputFile, objContainer);
		fputs(" has no 'contain' module\n", outputFile);
		return;
	}
	
	fputs("printItemsInContainedList - ", outputFile);
	printObjectIdentifyingInfo(outputFile, objContainer);
	fputs("\n", outputFile);
	fputs("------------\n", outputFile);
	
	Bool objToLookForFound = FALSE;
	conList = objContainer->getContain()->getContainedItemsList();
	if (conList->empty())
	{
		// just say that?
		fputs(" (empty)\n", outputFile);
	}
	else
	{
		for(ContainedItemsList::const_iterator it = conList->begin(); it != conList->end(); ++it)
		{
			Object* containedObj = *it;
			printDeletionCriticalInfo(outputFile, containedObj);
			if (containedObj->getID() == objToLookFor->getID())
			{
				fputs("^^^ piicl_OBJTOLOOKFOR ^^^", outputFile);
				objToLookForFound = TRUE;
			}
		}
	}
	if (!objToLookForFound)
	{
		fputs("--- WARNING - 'objToLookFor' ", outputFile);
		printObjectIdentifyingInfo(outputFile, objToLookFor);
		fputs(" provided but not found\n", outputFile);
	}
	fputs("------------\n", outputFile);
}

void objectContainedByOnDeleteCheck_printLabel(FILE* outputFile, int callSource)
{
	switch(callSource)
	{
		case 1:
		{
			fputs(" - destroyObject src: ", outputFile);
			// print each member of 'g_destroyObjectSource' to show each layer in case this is a nested call
			// (external 'destroyObject' call triggering a 'destroyObject' call)

			if (g_destroyObjectSource.empty())
			{
				fprintf(outputFile, "<no source - should not be possible!>\n");
				return;
			}

			// oh brother, now ya tell me VS6 doesn't have the science fictional wonder that is 'cbegin'...
			// NOTE TO SELF - use a "#if defined(_MSC_VER) && _MSC_VER < 1300" check to compile script only for VS6.
			// Not worth it in this case, I'll just stick to 'begin', though strangely 'const_iterator' was supported?
			std::vector<int>::const_iterator it = g_destroyObjectSource.begin();

			// first item, handle separately to not prepend a separator
			fprintf(outputFile, "%03d", *it);
			// advance
			++it;

			for (; it != g_destroyObjectSource.end(); ++it)
			{
				fprintf(outputFile, " -> %03d", *it);
			}
			fprintf(outputFile, "\n");
			break;
		}
		case 2:
		{
			fputs(" - processDestroyList\n", outputFile);
			break;
		}
	}
}

void objectContainedByOnDeleteCheck(Object* currentObject, int callSource)
{
	//MODDD - DEBUG - if anything is referring to this object being deleted... THAT'S BAD!
	for (Object* objThru = TheGameLogic->getFirstObject(); objThru; objThru = objThru->getNextObject())
	{
		if (objThru->getContainedBy() != nullptr && objThru->getContainedBy() == currentObject)
		{
			FILE* outputFile = fopen("test_crash_containedByBadMemoryBug.txt", "a");
			printTimeStamp(outputFile);

			objectContainedByOnDeleteCheck_printLabel(outputFile, callSource);

			printObjectIdentifyingInfo(outputFile, currentObject);
			fputs(" is being destroyed but ", outputFile);
			printObjectIdentifyingInfo(outputFile, objThru);
			fputs("->getContainedBy() still refers to it\n", outputFile);
				
			if (objThru->getID() == currentObject->getID())
			{
				fputs("WARNING - they match?\n", outputFile);
			}
				
			printDeletionCriticalInfo(outputFile, currentObject, "obj being deleted");
			printDeletionCriticalInfo(outputFile, objThru, "obj w/ 'getContainedBy' pointing at that");
			printItemsInContainedList(outputFile, currentObject, objThru);
			fclose(outputFile);

			// nope!  need to undersatnd the issue as it happens right now, don't interfere with possible fixes yet
			// (trying it this time)
			objThru->friend_setContainedBy(nullptr);
			//printf("%d%d", isDestroyed, (int)conList);
		}
	}
}

}// namespace rts_extra
