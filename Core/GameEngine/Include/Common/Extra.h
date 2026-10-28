//MODDD - new file to separate out logic for automatic adjustments to thing attributes after INI parsing and cheats.
// Example: making units/buildings take longer to build, have more health, extra scalar on most money additions for
// computer players.
// This should reduce the clutter that would otherwise be in ThingTemplate.cpp.
// Also, most of these features are disabled by default in this forked repo's 'main' branch. Enable/adjust as needed,
// but as usual, in multiplayer, anyone you're playing a game with needs to use a build made with the exact same
// changes to this repo (or give them a copy of your built EXE).
// (todo: header stuff)

#pragma once

class Player;
class ThingTemplate;
class WeaponTemplate;
class UpgradeTemplate;
class Object;
class Weapon;
class WeaponBonus;
struct upgradePair;

enum MoneySourceType;

extern int g_dummy;

#if EXTRA_DEBUG_HELP
// Use this to determine the source of a 'destroyObject' call (I don't think an enum is necessary)
extern std::vector<int> g_destroyObjectSource;
#endif

namespace rts_extra
{
//MODDD - bugfix for non-shared abilities on buildings being unusable on RETAIL_COMPATIBLE_CRC=0
void call_objectOnBuildComplete(Object* obj, Bool checkForSpecialPowerModuleCreateCalls = FALSE);

Real getShroudClearingRangeForLookAdjusted(const Object* obj);

Bool isCurrentGameCampaign();

// General utilities I came up with to condense some common script. 'common/GameUtility.h' might also be a good place for this.
Int getUpgradedSupplyBoost(const Object* collectingObject, const std::list<upgradePair>* upgradeBoostList);
void automaticThingTemplateChanges(ThingTemplate* _this);
void automaticChangesPostINIParsing();
void automaticChangesPostINIParsing_things();
void automaticChangesPostINIParsing_thing(ThingTemplate* _this);

#if CUSTOM_ATTRIBUTE_CHANGES
void automaticWeaponTemplateChanges(WeaponTemplate* _this);
void automaticUpgradeTemplateChanges(UpgradeTemplate* _this);
void automaticGlobalDataWeaponBonusChanges();
void addCustomWeaponBonuses(const Weapon* _this, const Object* source, WeaponBonus& bonus);
Real getHealthMulti(const ThingTemplate* _this);
Real healthAdjustmentFilter(Object* obj, Real healthVal);
UnsignedInt specialPowerReloadTimeAdjustmentFilter(const Object* obj, UnsignedInt reloadTime, Bool isSharedNSync);
#endif

#if CUSTOM_GAME_DATA_CHANGES
void automaticGlobalDataChanges();
#endif

#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
Real moneyScalarAdjustmentFilter(const Player* player, Bool runMajorCheats);
UnsignedInt getCheatAdjustedMoneyAmount(Player* player, UnsignedInt amountToDeposit, Bool runMajorCheats, MoneySourceType moneySource);
UnsignedInt getCheatAdjustedMoneyAmount(Player* player, UnsignedInt amountToDeposit, Bool runMajorCheats, Real preScalar, MoneySourceType moneySource);
UnsignedInt getCheatAdjustedMoneyAmountConditional(Player* player, UnsignedInt amountToDeposit, Bool runMajorCheats, MoneySourceType moneySource, Bool canApplyMoney);
UnsignedInt getCheatAdjustedMoneyAmountConditional(Player* player, UnsignedInt amountToDeposit, Bool runMajorCheats, Real preScalar, MoneySourceType moneySource, Bool canApplyMoney);
#endif
#if RUN_BUILD_TIME_CHEATS || NOOB_MODE
Int buildTimeAdjustmentFilter(const Player* player, const ThingTemplate* tt);
#endif
#if RUN_PLAYER_PROMOTION_EXPERIENCE_RATE_CHEATS || NOOB_MODE
Real playerPromotionExperienceRateFilter(const Player* player, Real expRateModifier);
#endif

void extraScoreScreenStatsPrintOut();
void extraScoreScreenStatsPrintOut_player(FILE* outputFile, Player* player);

//MODDD - DEBUG
Bool checkIfObjInDestroyList(const Object* objCheck);
void printTimeStamp(FILE* outputFile);
void printTimeStamp_noFrame(FILE* outputFile);
void printObjectIdentifyingInfo(FILE* outputFile, const Object* objToPrint);
AsciiString getObjectIdentifyingInfoString(Object* objToPrint);
void printDeletionCriticalInfo(FILE* outputFile, const Object* objToPrint);
void printDeletionCriticalInfo(FILE* outputFile, const Object* objToPrint, const char* extraHeading);
void printItemsInContainedList(FILE* outputFile, const Object* objContainer);
void printItemsInContainedList(FILE* outputFile, const Object* objContainer, const Object* objToLookFor);
void objectContainedByOnDeleteCheck(Object* currentObject, int callSource);

}// namespace rts_extra
