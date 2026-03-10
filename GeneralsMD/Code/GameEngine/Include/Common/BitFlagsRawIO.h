
//MODDD - new file

#pragma once

#include "Common/BitFlagsRaw.h"
//#include "Common/INI.h"
#include "Common/Xfer.h"

template <size_t NUMBITS>
void BitFlagsRaw<NUMBITS>::xfer(Xfer* xfer)
{
	// This is primitive enough I don't think it warrants a version, and entirely new to the as-is state of
	// this codebase anyway.
	if( xfer->getXferMode() == XFER_SAVE )
	{
		xfer->xferUser( this->m_bits, sizeof( this->m_bits ) );
	}
	else if( xfer->getXferMode() == XFER_LOAD )
	{
		xfer->xferUser( this->m_bits, sizeof( this->m_bits ) );
	}
	else if( xfer->getXferMode() == XFER_CRC )
	{
		xfer->xferUser( this->m_bits, sizeof( this->m_bits ) );
	}
	else
	{

		DEBUG_CRASH(( "BitFlagsXfer - Unknown xfer mode '%d'", xfer->getXferMode() ));
		throw XFER_MODE_UNKNOWN;

	}

}
