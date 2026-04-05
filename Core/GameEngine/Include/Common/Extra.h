//MODDD - new file to separate out logic for automatic adjustments to thing attributes after INI parsing and cheats.
// Example: making units/buildings take longer to build, have more health, extra scalar on most money additions for
// computer players.
// This should reduce the clutter that would otherwise be in ThingTemplate.cpp.
// Also, most of these features are disabled by default in this forked repo's 'main' branch. Enable/adjust as needed,
// but as usual, in multiplayer, anyone you're playing a game with needs to use a build made with the exact same
// changes to this repo (or give them a copy of your built EXE).
// (todo: header stuff)

#pragma once

// This is one cool include!
#include "Common/STLTypedefs.h"

class Player;
class ThingTemplate;
class WeaponTemplate;
class UpgradeTemplate;
class Object;
class Weapon;
class WeaponBonus;

#if CUSTOM_ATTRIBUTE_CHANGES
void automaticThingTemplateChanges(ThingTemplate* _this);
void automaticWeaponTemplateChanges(WeaponTemplate* _this);
void automaticUpgradeTemplateChanges(UpgradeTemplate* _this);
void automaticGlobalDataWeaponBonusChanges();
void addCustomWeaponBonuses(const Weapon* _this, const Object* source, WeaponBonus& bonus);
Real getHealthMulti(const ThingTemplate* _this);
Real healthAdjustmentFilter(Object* obj, Real healthVal);
#endif

#if CUSTOM_GAME_DATA_CHANGES
void automaticGlobalDataChanges();
#endif

#if RUN_EXTRA_MONEY_CHEATS || NOOB_MODE
Real moneyScalarAdjustmentFilter(const Player* player);
UnsignedInt getCheatAdjustedMoneyAmount(Player* player, UnsignedInt amountToDeposit);
#endif
#if RUN_BUILD_TIME_CHEATS
Int buildTimeAdjustmentFilter(const Player* player, Int buildTime);
#endif
#if RUN_PLAYER_PROMOTION_EXPERIENCE_RATE_CHEATS || NOOB_MODE
Real playerPromotionExperienceRateFilter(const Player* player, Real expRateModifier);
#endif

//MODDD - DEBUG
Bool checkIfObjInDestroyList(const Object* objCheck);
void printTimeStamp(std::ofstream& outputStream);
void printObjectIdentifyingInfo(std::ofstream& outputStream, const Object* objToPrint);
AsciiString getObjectIdentifyingInfoString(Object* objToPrint);
void printDeletionCriticalInfo(std::ofstream& outputStream, const Object* objToPrint);
void printDeletionCriticalInfo(std::ofstream& outputStream, const Object* objToPrint, const char* extraHeading);
void printItemsInContainedList(std::ofstream& outputStream, const Object* objContainer);
void printItemsInContainedList(std::ofstream& outputStream, const Object* objContainer, const Object* objToLookFor);
