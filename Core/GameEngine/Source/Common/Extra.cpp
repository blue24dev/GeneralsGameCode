//MODDD - new file

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Extra.h"
#include "Common/Player.h"
#include "Common/KindOf.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"
#include "GameLogic/Object.h"
#include "GameNetwork/GameInfo.h"
#include "GameLogic/GameLogic.h"

// It would make sense to consider the majority of this file "MODDD - for me only".


#if CUSTOM_ATTRIBUTE_CHANGES

// includes for hackish edits further below
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
		if (_this->m_visionRange < 180.0f) {
			_this->m_visionRange = 180.0f;
		}
		_this->m_visionRange *= 1.3f;
	}

	// EXTRA SLOW-DOWN FOR EVERYTHING
	_this->m_buildTime *= 1.4f;

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

	static NameKeyType ActiveBodyNameKey = NAMEKEY("ActiveBody");
	static NameKeyType StructureBodyNameKey = NAMEKEY("StructureBody");
#if RTS_ZEROHOUR
	static NameKeyType UndeadBodyNameKey = NAMEKEY("UndeadBody");
#endif
	static NameKeyType RebuildHoleExposeDieNameKey = NAMEKEY("RebuildHoleExposeDie");
	static NameKeyType MaxHealthUpgradeNameKey = NAMEKEY("MaxHealthUpgrade");
	static NameKeyType StealthDetectorUpdateNameKey = NAMEKEY("StealthDetectorUpdate");
	static NameKeyType SpecialAbilityUpdateNameKey = NAMEKEY("SpecialAbilityUpdate");
	static NameKeyType ActiveShroudUpgradeNameKey = NAMEKEY("ActiveShroudUpgrade");
	
	Bool foundStealthDetectorUpdate = false;
	Bool foundActiveShroudUpgrade = false;
	StealthDetectorUpdateModuleData* stealthDetectorData = nullptr;

	// Automatic adjustments for the max health based on kind
	Int modIdx;
	const ModuleInfo& mi = _this->getBehaviorModuleInfo();
	for (modIdx = 0; modIdx < mi.getCount(); ++modIdx)
	{
		AsciiString modName = mi.getNthName(modIdx);
		const ModuleData* data = mi.getNthData(modIdx);
		if (modName.isEmpty())
			continue;
		NameKeyType modNameKey = NAMEKEY(modName);

		// Checking for 'ActiveBody', 'StructureBody', and 'UndeadBody' should be enough.
		// Other body subclasses are 'ImmortalBody' and 'HighlanderBody', which don't look to be for normal commandable/attackable things.
		if( modNameKey == ActiveBodyNameKey )
		{
			ActiveBodyModuleData* _data = (ActiveBodyModuleData*)data;
			// Assume if a health value is so absurdly high it shouldn't be touched
			if (_data->m_maxHealth >= 1000000)continue;
			Real healthMulti = getHealthMulti(_this);
			if (healthMulti != 1.0f)
			{
				_data->m_initialHealth *= healthMulti;
				_data->m_maxHealth *= healthMulti;
			}
		}
		else if( modNameKey == StructureBodyNameKey )
		{
			StructureBodyModuleData* _data = (StructureBodyModuleData*)data;
			// Assume if a health value is so absurdly high it shouldn't be touched
			if (_data->m_maxHealth >= 1000000.0f)continue;
			Real healthMulti = getHealthMulti(_this);
			if (healthMulti != 1.0f)
			{
				_data->m_initialHealth *= healthMulti;
				_data->m_maxHealth *= healthMulti;
			}
		}
#if RTS_ZEROHOUR
		else if( modNameKey == UndeadBodyNameKey )
		{
			UndeadBodyModuleData* _data = (UndeadBodyModuleData*)data;
			// Assume if a health value is so absurdly high it shouldn't be touched
			if (_data->m_maxHealth >= 1000000.0f)continue;
			Real healthMulti = getHealthMulti(_this);
			if (healthMulti != 1.0f)
			{
				_data->m_initialHealth *= healthMulti;
				_data->m_maxHealth *= healthMulti;
				_data->m_secondLifeMaxHealth *= healthMulti;
			}
		}
#endif
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
			if (_data->m_detectionRange > 0)
			{
				  _data->m_detectionRange *= 1.2f;
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
	}

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
}

void automaticUpgradeTemplateChanges(UpgradeTemplate* _this)
{
	_this->m_buildTime *= 1.1f * 1.4f;
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
			return 1.85f;
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
#endif // CUSTOM_ATTRIBUTE_CHANGES


#if RUN_EXTRA_MONEY_CHEATS
Real moneyScalarAdjustmentFilter(const Player* player)
{
	// The income bonus for AI players can increase over the course of a long game.
	const UnsignedInt startMin = 15;
	const UnsignedInt endMin = 60;
	const Real startModifier = 1.5f;
	const Real endModifier = 3.3f;

	Real scalar = 1.0f;
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
	const UnsignedInt startMin = 15;
	const UnsignedInt endMin = 60;
	const float startModifier = 1.00f;
	const float endModifier = 0.78f;

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
	const UnsignedInt startMin = 15;
	const UnsignedInt endMin = 60;
	const Real startModifier = 1.10f;
	const Real endModifier = 1.50f;

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
#endif
