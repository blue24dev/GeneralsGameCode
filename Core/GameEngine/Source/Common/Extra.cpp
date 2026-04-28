//MODDD - new file

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Extra.h"
#include "Common/Player.h"
#include "Common/KindOf.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"
#include "GameNetwork/GameInfo.h"
#include "GameClient/ControlBar.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/Object.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/AutoDepositUpdate.h"
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
#include "GameLogic/Module/StealthDetectorUpdate.h"
#include "GameLogic/Module/SpecialAbilityUpdate.h"
#include "GameLogic/Module/ActiveShroudUpgrade.h"
#include "GameLogic/Module/CashHackSpecialPower.h"

// It would make sense to consider 'CUSTOM_ATTRIBUTE_CHANGES' sections "MODDD - for me only".

// Other new functions that handle post-parsing edits:
//   WeaponTemplateSet::validateAutoChooseSources
//   ThingTemplate::makeNonCivilianGarrisonableStructureCapturableHack

// Set this in code to let breakpoints work in release mode, as opposed to the usual 'int x; x = 4;' thing I usually do.
// Ex:
//   if (obj->getTemplate()->getName() == "xyz") {
//     g_dummy = 4;
//   }
int g_dummy = 0;

#if EXTRA_DEBUG_HELP
int g_destroyObjectSource = 0;
#endif

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
	if (_data->m_subdualDamageCap <= 0)
	{
		// not even set / deliberately 0? going to assume this is for a reason - don't touch
		return;
	}

	// First, check to see if the cap matches the sub-damage-to-disable.
	if (_data->m_subdualDamageCap == _data->m_subdualDamageToDisable)
	{
		// Push the cap up a bit so the disabler stopping doesn't instantly go back to normal.
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

// Go through a ThingTemplate that's recently been parsed from the INI files
// (ex: 'AmericaVehicle.ini' -> 'Object AmericaVehicleHumvee')
// and see if it has any stats or modules that should be adjusted for either bug fixes or additional attribute edits
// (ex: 30% more health for all units). The 'CUSTOM_ATTRIBUTE_CHANGES' will decide if latter changes are applied,
// otherwise, only the minimum bug-fixing changes will be applied.
// Most of these 'bugs' are from TheSuperHackers changes that cause new undesired behavior in the retail game or
// mods with macro settings such as 'PRESERVE_RETAIL_BEHAVIOR' set to off. I still want to keep the setting that way
// because it being off introduces several other fixes & efficiency improvements (pathfinding) that are good to keep.
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
	Real originalVision = _this->m_visionRange;
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

	// Beware of side effects like revealed fog of war that doesn't un-reveal. This is not well understood.
	// Checking for being above 0 first appears to fix this. Are negative values used in some places?
	if (_this->m_shroudClearingRange > 0)
	{
		_this->m_shroudClearingRange *= 1.50f;
	}
#endif
	
	static NameKeyType SalvageCrateCollideNameKey = NAMEKEY("SalvageCrateCollide");
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
	
#if CUSTOM_ATTRIBUTE_CHANGES
	Bool foundStealthDetectorUpdate = false;
	Bool foundActiveShroudUpgrade = false;
	StealthDetectorUpdateModuleData* stealthDetectorData = nullptr;
#endif

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
	}

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

			mi2.addModuleInfo(_this, ActiveShroudUpgradeName, moduleTag, data, (MODULEINTERFACE_UPDATE), false);

			//BehaviorModule* newMod = (BehaviorModule*)TheModuleFactory->newModule(this, modName, data, MODULETYPE_BEHAVIOR);
			//*curB++ = newMod;
		}
  }
	*/
#endif
}

// What to do after parsing everything else.
// Ex: 'TheControlBar' is unavailable until this point ('ControlBar::init' handles parsing in 'CommandButton.ini', not 'GameEngine::init'
// that does it for thing templates and a host of other things).
// For reference, 'ControlBar::init' is reached by 'initSubsystem(TheGameClient...' in 'GameEngine::init', which is called after thing templates are handled.
void automaticChangesPostINIParsing()
{
#if RTS_ZEROHOUR
	static NameKeyType OCLSpecialPowerNameKey = NAMEKEY("OCLSpecialPower");
	ThingTemplateHashMap* templateHashMap = TheThingFactory->getTemplateHashMap();
	ThingTemplateHashMap::iterator it;
	for (it = templateHashMap->begin(); it != templateHashMap->end(); ++it)
	{
		ThingTemplate* _this = (*it).second;

		//TODO - should overrides be blocked like the '_this->m_reskinnedFrom != nullptr' check elsewhere? Not sure, see if doing this for the base
		// implicitly carries over to overrides for this post-parsing run-through

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
					const ThingTemplate* referenceObjectRef = nullptr;
					if (!_data->m_referenceThingName.isEmpty())
					{
						referenceObjectRef = TheThingFactory->findTemplate( _data->m_referenceThingName );
					}

					if (referenceObjectRef == nullptr)
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
									referenceObjectRef = commandButton->getThingTemplate();
									break;
								}
							}
						}

						if (referenceObjectRef != nullptr)
						{
							// we got em'.
							_data->m_referenceThingName = referenceObjectRef->getName();
						}
					}
				}
			}
		}
	}
#endif
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
					tempMask.set(KINDOF_INFANTRY);
					tempMask.set(KINDOF_VEHICLE);
					tempMask.set(KINDOF_AIRCRAFT);
					tempMask.set(KINDOF_HUGE_VEHICLE);
					if (_this->isAnyKindOf(tempMask))
					{
						// a non-structure unit (not some weird system/inner-detail thing): have a little more health anyway
						return 1.30f;
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
}
#endif // CUSTOM_GAME_DATA_CHANGES


#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
Real moneyScalarAdjustmentFilter(const Player* player)
{
	// The income bonus for AI players can increase over the course of a long game.
	/*
	const UnsignedInt startMin = 8;
	const UnsignedInt endMin = 80;
	const Real startModifier = 1.1f;
	const Real endModifier = 4.0f;
	*/
	const UnsignedInt startMin = 10;
	const UnsignedInt endMin = 80;
	const Real startModifier = 1.0f;
	const Real endModifier = 2.5f;

	Real scalar = 1.0f;

	#if RUN_EXTRA_MONEY_CHEATS
	if (player->getPlayerType() == PLAYER_COMPUTER)
	{
		UnsignedInt frame = TheGameLogic->getFrame();
		if (frame <= (30 * 60) * startMin)
		{
			scalar *= startModifier;
		}
		else if(frame <= (30 * 60) * endMin)
		{
			Real fracto = (Real)(frame - ((30 * 60) * startMin)) / (Real)((30 * 60) * (endMin - startMin));
			Real fracto_inv = 1.0f - fracto;
			
			scalar *= endModifier + (startModifier - endModifier) * fracto_inv;
		}
		else
		{
			scalar *= endModifier;
		}
		return scalar;
	}
	#endif

	#if NOOB_MODE
	if (player->getPlayerType() == PLAYER_HUMAN && player->slotIndex == 1)
	{
		scalar *= (Real)NOOB_INCOME_MONEY_SCALAR;
		return scalar;
	}
	#endif

	return scalar;
}

// See if the player should receive extra money on top of the intended amount
UnsignedInt getCheatAdjustedMoneyAmount(Player* player, UnsignedInt amountToDeposit)
{
	Real scalar = moneyScalarAdjustmentFilter(player);
	if (scalar != 1.0f)
	{
		return (UnsignedInt)(((Real)amountToDeposit) * scalar);
	}

	return amountToDeposit;
}
#endif // RUN_EXTRA_MONEY_CHEATS

#if RUN_BUILD_TIME_CHEATS
Int buildTimeAdjustmentFilter(const Player* player, Int buildTime)
{
	// AI players can build faster over the course of a long game.
	/*
	const UnsignedInt startMin = 8;
	const UnsignedInt endMin = 80;
	const Real startModifier = 1.00f;
	const Real endModifier = 0.76f;
	*/
	const UnsignedInt startMin = 8;
	const UnsignedInt endMin = 80;
	const Real startModifier = 1.00f;
	const Real endModifier = 0.90f;

	Int _buildTime = buildTime;
	if (player->getPlayerType() == PLAYER_COMPUTER)
	{
		UnsignedInt frame = TheGameLogic->getFrame();
		if (frame <= (30 * 60) * startMin)
		{
			_buildTime *= startModifier;
		}
		else if(frame <= (30 * 60) * endMin)
		{
			Real fracto = (Real)(frame - ((30 * 60) * startMin)) / (Real)((30 * 60) * (endMin - startMin));
			Real fracto_inv = 1.0f - fracto;
			
			_buildTime *= endModifier + (startModifier - endModifier) * fracto_inv;
		}
		else
		{
			_buildTime *= endModifier;
		}
		return _buildTime;
	}
	return _buildTime;
}
#endif // RUN_BUILD_TIME_CHEATS

#if RUN_PLAYER_PROMOTION_EXPERIENCE_RATE_CHEATS || NOOB_MODE
Real playerPromotionExperienceRateFilter(const Player* player, Real expRateModifier)
{
	// AI players receive more experience toward promotions (not individual unit veterancy) per kill over the course of a long game.
	const UnsignedInt startMin = 10;
	const UnsignedInt endMin = 50;
	const Real startModifier = 1.00f;
	const Real endModifier = 1.30f;

	Real _expRateModifier = expRateModifier;

	#if RUN_PLAYER_PROMOTION_EXPERIENCE_RATE_CHEATS
	if (player->getPlayerType() == PLAYER_COMPUTER)
	{
		UnsignedInt frame = TheGameLogic->getFrame();
		if (frame <= (30 * 60) * startMin)
		{
			_expRateModifier *= startModifier;
		}
		else if(frame <= (30 * 60) * endMin)
		{
			Real fracto = (Real)(frame - ((30 * 60) * startMin)) / (Real)((30 * 60) * (endMin - startMin));
			Real fracto_inv = 1.0f - fracto;
			
			_expRateModifier *= endModifier + (startModifier - endModifier) * fracto_inv;
		}
		else
		{
			_expRateModifier *= endModifier;
		}
		return _expRateModifier;
	}
	#endif

	#if NOOB_MODE
	if (player->getPlayerType() == PLAYER_HUMAN && player->slotIndex == 1)
	{
		_expRateModifier *= (Real)NOOB_PLAYER_PROMOTION_EXPERIENCE_RATE_SCALAR;
		return _expRateModifier;
	}
	#endif

	return _expRateModifier;
}
#endif // RUN_PLAYER_PROMOTION_EXPERIENCE_RATE_CHEATS


//MODDD - debugging
#include <fstream>
#include <iomanip>
#include "GameLogic/Module/ContainModule.h"

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
void printTimeStamp(std::ofstream& outputStream)
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	outputStream <<
		lt.wYear << "-" <<
		lt.wMonth << "-" <<
		lt.wDay << " " <<
		lt.wHour << ":" <<
		lt.wMinute << ":" <<
		lt.wSecond << "." <<
		std::setw(3) <<
		std::setfill('0') <<
		lt.wMilliseconds <<
		" Frame:" <<
		TheGameLogic->getFrame();
}

// Print the most basic info to tell what this object is at a glance. That is, template name (americaVehicle-whatever)
// with ID to tell if other places are referring to the exact same object.
// Note that this doesn't end in a newline character.
void printObjectIdentifyingInfo(std::ofstream& outputStream, const Object* objToPrint)
{
	outputStream << objToPrint->getTemplate()->getName().str() << ":" << objToPrint->getID();
}

// really, may as well turn that into a utility that gives the string directly
AsciiString getObjectIdentifyingInfoString(const Object* objToPrint)
{
	AsciiString identifier;
	identifier.format("%s:%d", objToPrint->getTemplate()->getName().str(), objToPrint->getID());
	return identifier;
}

void printDeletionCriticalInfo(std::ofstream& outputStream, const Object* objToPrint)
{
	Bool isDestroyed = objToPrint->testStatus(OBJECT_STATUS_DESTROYED);
	Bool isEffectivelyDead = objToPrint->isEffectivelyDead();
	Bool isObjThruInDestroyedList = checkIfObjInDestroyList(objToPrint);

	outputStream << "printDeletionCriticalInfo - ";
	printObjectIdentifyingInfo(outputStream, objToPrint);
	outputStream << " isDestroyed: " << isDestroyed;
	outputStream << " isEffectivelyDead: " << isEffectivelyDead;
	outputStream << " isObjThruInDestroyedList: " << isObjThruInDestroyedList;
	if (objToPrint->getBodyModule())
	{
		outputStream << " health: " << objToPrint->getBodyModule()->getHealth();
	}
	else
	{
		outputStream << " health: N/A (no body module)";
	}
	outputStream << std::endl;
}

void printDeletionCriticalInfo(std::ofstream& outputStream, const Object* objToPrint, const char* extraHeading)
{
	Bool isDestroyed = objToPrint->testStatus(OBJECT_STATUS_DESTROYED);
	Bool isEffectivelyDead = objToPrint->isEffectivelyDead();
	Bool isObjThruInDestroyedList = checkIfObjInDestroyList(objToPrint);

	outputStream << "printDeletionCriticalInfo - ";
	outputStream << extraHeading << ":";
	printObjectIdentifyingInfo(outputStream, objToPrint);
	outputStream << " isDestroyed: " << isDestroyed;
	outputStream << " isEffectivelyDead: " << isEffectivelyDead;
	outputStream << " isObjThruInDestroyedList: " << isObjThruInDestroyedList;
	if (objToPrint->getBodyModule())
	{
		outputStream << " health: " << objToPrint->getBodyModule()->getHealth();
	}
	else
	{
		outputStream << " health: N/A (no body module)";
	}
	outputStream << std::endl;
}

void printItemsInContainedList(std::ofstream& outputStream, const Object* objContainer)
{
	const ContainedItemsList* conList = nullptr;
	ContainModuleInterface* contain = objContainer->getContain();
	if (contain == nullptr)
	{
		outputStream << "printItemsInContainedList - ERROR - 'objContainer' ";
		printObjectIdentifyingInfo(outputStream, objContainer);
		outputStream << " has no 'contain' module" << std::endl;
		return;
	}

	outputStream << "printItemsInContainedList - ";
	printObjectIdentifyingInfo(outputStream, objContainer);
	outputStream << std::endl;
	outputStream << "------------" << std::endl;

	conList = objContainer->getContain()->getContainedItemsList();
	if (conList->empty())
	{
		// just say that?
		outputStream << " (empty)" << std::endl;
	}
	else
	{
		for(ContainedItemsList::const_iterator it = conList->begin(); it != conList->end(); ++it)
		{
			Object* containedObj = *it;
			printDeletionCriticalInfo(outputStream, containedObj);
		}
	}
	outputStream << "------------" << std::endl;
}

void printItemsInContainedList(std::ofstream& outputStream, const Object* objContainer, const Object* objToLookFor)
{
	const ContainedItemsList* conList = nullptr;
	ContainModuleInterface* contain = objContainer->getContain();
	if (contain == nullptr)
	{
		outputStream << "printItemsInContainedList - ERROR - 'objContainer' ";
		printObjectIdentifyingInfo(outputStream, objContainer);
		outputStream << " has no 'contain' module" << std::endl;
		return;
	}
	
	outputStream << "printItemsInContainedList - ";
	printObjectIdentifyingInfo(outputStream, objContainer);
	outputStream << std::endl;
	outputStream << "------------" << std::endl;
	
	Bool objToLookForFound = FALSE;
	conList = objContainer->getContain()->getContainedItemsList();
	if (conList->empty())
	{
		// just say that?
		outputStream << " (empty)" << std::endl;
	}
	else
	{
		for(ContainedItemsList::const_iterator it = conList->begin(); it != conList->end(); ++it)
		{
			Object* containedObj = *it;
			printDeletionCriticalInfo(outputStream, containedObj);
			if (containedObj->getID() == objToLookFor->getID())
			{
				outputStream << "^^^ piicl_OBJTOLOOKFOR ^^^";
				objToLookForFound = TRUE;
			}
		}
	}
	if (!objToLookForFound)
	{
		outputStream << "--- WARNING - 'objToLookFor' ";
		printObjectIdentifyingInfo(outputStream, objToLookFor);
		outputStream << " provided but not found" << std::endl;
	}
	outputStream << "------------" << std::endl;
}

void objectContainedByOnDeleteCheck (Object* currentObject, const char* callSourceLabel)
{
	//MODDD - DEBUG - if anything is referring to this object being deleted... THAT'S BAD!
	for (Object* objThru = TheGameLogic->getFirstObject(); objThru; objThru = objThru->getNextObject())
	{
		if (objThru->getContainedBy() != nullptr && objThru->getContainedBy() == currentObject)
		{
			std::ofstream outputFile;
			outputFile.open("test_crash_containedByBadMemoryBug.txt", std::ios::out | std::ios::app);
			printTimeStamp(outputFile);
			outputFile << " - " << callSourceLabel << std::endl;

			printObjectIdentifyingInfo(outputFile, currentObject);
			outputFile << " is being destroyed but ";
			printObjectIdentifyingInfo(outputFile, objThru);
			outputFile << "->getContainedBy() still refers to it" << std::endl;
				
			if (objThru->getID() == currentObject->getID())
			{
				outputFile << "WARNING - they match?" << std::endl;
			}
				
			printDeletionCriticalInfo(outputFile, currentObject, "obj being deleted");
			printDeletionCriticalInfo(outputFile, objThru, "obj w/ 'getContainedBy' pointing at that");
			printItemsInContainedList(outputFile, currentObject, objThru);
			outputFile.close();

			// nope!  need to undersatnd the issue as it happens right now, don't interfere with possible fixes yet
			// (trying it this time)
			objThru->friend_setContainedBy(nullptr);
			//printf("%d%d", isDestroyed, (int)conList);
		}
	}
}
