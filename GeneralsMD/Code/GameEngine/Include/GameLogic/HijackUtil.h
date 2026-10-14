//MODDD - new file, to reduce some of the clutter from an excessive amount of static helpers similar to
// 'EjectPilotDie::ejectPilot' used for hijack logic

#pragma once

// forward declarations
class Object;
class ObjectCreationList;

namespace HijackUtil
{
	void deleteExistingHijacker(ObjectID hijackerID);
	void ejectHijacker(Object* hijacker, Object* dyingObject, const ObjectCreationList* oclReference);
	Bool checkForTargetEjectSupport(Object* hijacker, Object* other);
	void clearHijackerInObject(Object* obj);
}
