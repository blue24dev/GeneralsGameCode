//MODDD - new file for the the existing DeliverPayloadNugget class definition.
// (most) Implementations remain in ObjectCreationList.cpp
// (todo: header stuff)

#pragma once

#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Module/DeliverPayloadAIUpdate.h"

//MODDD - DeliverPayloadNugget class definition moved from ObjectCreationList.cpp, prototyping innards instead, implementations remain in ObjectCreationList.cpp
// ------------------------------------------------------------------------------------------------
class DeliverPayloadNugget : public ObjectCreationNugget
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(DeliverPayloadNugget, "DeliverPayloadNugget")
public:

	DeliverPayloadNugget();

	//MODDD - new, override for a new getter
	virtual OCLNuggetType getTypeID() override
	{
		return OCL_NUGGET_TYPE_DELIVER_PAYLOAD;
	}

	//MODDD - disarming mines gives experience. Removed 'const' on 'primaryObj'
	virtual Object* create(Object *primaryObj, const Coord3D *primary, const Coord3D *secondary, Real angle, UnsignedInt lifetimeFrames = 0 ) const override;

	//MODDD - disarming mines gives experience. Removed 'const' on 'primaryObj'
	virtual Object* create(Object* primaryObj, const Coord3D *primary, const Coord3D* secondary, Bool createOwner, UnsignedInt lifetimeFrames = 0 ) const override;

	//MODDD - new getter
	const AsciiString& getTransportName() const { return m_transportName; }

	static void parsePayload( INI* ini, void *instance, void *store, const void* /*userData*/ );

	static void parse(INI *ini, void *instance, void* /*store*/, const void* /*userData*/);

private:

	struct Payload
	{
		AsciiString m_payloadName;
		Int m_payloadCount;
	};

	//Specific data needed to create the transport(s), internal payload, and initial physics.
  AsciiString           m_transportName;
	AsciiString						m_putInContainerName;
	std::vector<Payload>	m_payload;
	Real									m_formationSpacing;
	Real									m_convergenceFactor;
	Real									m_errorRadius;
	UnsignedInt						m_delayDeliveryFramesMax;
	UnsignedInt						m_formationSize;
	Bool									m_startAtPreferredHeight;
	Bool									m_startAtMaxSpeed;

	//AI specific data passed over to DeliverPayloadAIUpdate::deliver()
	DeliverPayloadData		m_data;
};
EMPTY_DTOR(DeliverPayloadNugget)
// ------------------------------------------------------------------------------------------------
