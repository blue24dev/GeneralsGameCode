//MODDD - new file for the the existing GenericObjectCreationNugget class definition.
// (most) Implementations remain in ObjectCreationList.cpp
// (todo: header stuff)

#pragma once

#include "GameLogic/ObjectCreationList.h"
//#include "GameLogic/Module/DeliverPayloadAIUpdate.h"

//MODDD - GenericObjectCreationNugget class definition moved from ObjectCreationList.cpp, prototyping innards instead, implementations remain in ObjectCreationList.cpp
class GenericObjectCreationNugget : public ObjectCreationNugget
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(GenericObjectCreationNugget, "GenericObjectCreationNugget")
public:
	//MODDD - moved to here from below
	struct AnimSet
	{
		AsciiString								m_animInitial;
		AsciiString								m_animFlying;
		AsciiString								m_animFinal;
	};

	GenericObjectCreationNugget();
	
	//MODDD - new, override for a new getter
	virtual OCLNuggetType getTypeID() override
	{
		// easy hack since both types re-use this class & consistently set this field with a 1-1 relationship with their type
		return m_nameAreObjects ? OCL_NUGGET_TYPE_CREATE_OBJECT : OCL_NUGGET_TYPE_CREATE_DEBRIS;
	}
	//MODDD - getter for hackery
	std::vector<AsciiString>& getNames() { return m_names; }
	const std::vector<AsciiString>& getNames() const { return m_names; }

	virtual Object* create(Object* primary, const Object* secondary, UnsignedInt lifetimeFrames = 0 ) const override;
	virtual Object* create(Object* primaryObj, const Coord3D *primary, const Coord3D* secondary, Real angle, UnsignedInt lifetimeFrames = 0 ) const override;
	static const FieldParse* getCommonFieldParse();
	static void parseObject(INI *ini, void *instance, void* /*store*/, const void* /*userData*/);
	static void parseDebris(INI *ini, void *instance, void* /*store*/, const void* /*userData*/);
	static void parseAnimSet(INI *ini, void * /*instance*/, void* store, const void* /*userData*/);

protected:

	void doStuffToObj(
		Object* obj,
		const AsciiString& modelName,
		const Coord3D *pos,
		const Matrix3D *mtx,
		Real orientation,
		const Object *sourceObj,
		UnsignedInt lifetimeFrames
	) const;

	Object* reallyCreate(const Coord3D *pos, const Matrix3D *mtx, Real orientation, const Object *sourceObj, UnsignedInt lifetimeFrames ) const;
	static void parseDebrisObjectNames( INI* ini, void *instance, void *store, const void* /*userData*/ );

private:
	//MODDD - moved 'AnimSet' struct definition to the top
	
	std::vector<AsciiString>	m_names;
	AsciiString								m_putInContainer;
	std::vector<AnimSet>			m_animSets;
	const FXList*							m_fxFinal;
	AsciiString								m_particleSysName;
	Int												m_debrisToGenerate;
	Real											m_mass;
	Real											m_extraBounciness;
	Real											m_extraFriction;
	Coord3D										m_offset;
	DebrisDisposition					m_disposition;
	Real											m_dispositionIntensity;
	Real											m_spinRate;
	Real											m_yawRate;
	Real											m_rollRate;
	Real											m_pitchRate;
	Real											m_minMag, m_maxMag;
	Real											m_minPitch, m_maxPitch;
	UnsignedInt								m_minFrames, m_maxFrames;
	ShadowType								m_shadowType;
	StaticGameLODLevel				m_minLODRequired;
	UnsignedInt								m_invulnerableTime;
	Real											m_minHealth;
	Real											m_maxHealth;
	UnsignedInt								m_fadeFrames;
	AsciiString								m_fadeSoundName;
	Real											m_minDistanceAFormation;
	Real											m_minDistanceBFormation;
	Real											m_maxDistanceFormation;
	Int												m_objectCount; // how many objects will there be?
	AudioEventRTS							m_bounceSound;
	Bool											m_requiresLivePlayer;
	Bool											m_containInsideSourceObject; ///< The created stuff will be added to the Contain module of the SourceObject
	Bool											m_preserveLayer;
	Bool											m_nameAreObjects;
	Bool											m_okToChangeModelColor;
	Bool											m_orientInForceDirection;
	Bool											m_spreadFormation;
	Bool											m_fadeIn;
	Bool											m_fadeOut;
	Bool											m_ignorePrimaryObstacle;
	Bool											m_inheritsVeterancy;
  Bool                      m_diesOnBadLand;
	Bool											m_skipIfSignificantlyAirborne;

};
EMPTY_DTOR(GenericObjectCreationNugget)
