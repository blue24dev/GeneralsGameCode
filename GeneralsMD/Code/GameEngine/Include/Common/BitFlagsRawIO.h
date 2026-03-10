//MODDD - new file

#pragma once

#include "Common/BitFlagsRaw.h"
#include "Common/Xfer.h"

template <size_t NUMBITS>
void BitFlagsRaw<NUMBITS>::xfer(Xfer* xfer)
{
	// This is primitive enough I don't think it warrants a version, and entirely new to the as-is state of
	// this codebase anyway.

	// Also, skipping the 'xfer->getXferMode()' check for simplicity's sake, this same thing is fine for all
	// expected modes of XFER_SAVE/LOAD/CRC anyway
	xfer->xferUser( this->m_bits, sizeof( UnsignedByte ) * NUMBITS/8 );

}
