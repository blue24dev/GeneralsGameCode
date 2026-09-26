//MODDD - new file for the the existing DeliverPayloadNugget class definition.
// (most) Implementations remain in ObjectCreationList.cpp
// (todo: header stuff)

#pragma once

#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Module/DeliverPayloadAIUpdate.h"

//MODDD - DeliverPayloadNugget class definition moved from ObjectCreationList.cpp, prototyping innards instead, implementations remain in ObjectCreationList.cpp
class DeliverPayloadNugget : public ObjectCreationNugget
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(DeliverPayloadNugget, "DeliverPayloadNugget")
public:

	//MODDD - moved to here from below
	struct Payload
	{
		AsciiString m_payloadName;
		Int m_payloadCount;
	};

	DeliverPayloadNugget();

	//MODDD - new, override for a new getter
	virtual OCLNuggetType getTypeID() override
	{
		return OCL_NUGGET_TYPE_DELIVER_PAYLOAD;
	}
	
	//MODDD - new getters
	AsciiString& getTransportName() { return m_transportName; }
	const AsciiString& getTransportName() const { return m_transportName; }
	std::vector<Payload>& getPayload() { return m_payload; }
	const std::vector<Payload>& getPayload() const { return m_payload; }
	DeliverPayloadData& getDeliverPayloadData() { return m_data; }
	const DeliverPayloadData& getDeliverPayloadData() const { return m_data; }

	//MODDD - disarming mines gives experience. Removed 'const' on 'primaryObj'
	virtual Object* create(Object *primaryObj, const Coord3D *primary, const Coord3D *secondary, Real angle, UnsignedInt lifetimeFrames = 0 ) const override;

	//MODDD - disarming mines gives experience. Removed 'const' on 'primaryObj'
	virtual Object* create(Object* primaryObj, const Coord3D *primary, const Coord3D* secondary, Bool createOwner, UnsignedInt lifetimeFrames = 0 ) const override;

	static void parsePayload( INI* ini, void *instance, void *store, const void* /*userData*/ );

	static void parse(INI *ini, void *instance, void* /*store*/, const void* /*userData*/);

private:
	//MODDD - moved 'Payload' struct definition to the top

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
