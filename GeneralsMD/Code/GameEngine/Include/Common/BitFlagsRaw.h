//MODDD - new file, based off Common/BitFlags.h but with a custom implementation (avoiding std::bitset)
// to avoid the need for string names per bit and offer access to the raw byte data for easier save/loads.
//MODDD - TODO
// Could argue the BitFlags class should also store raw bytes instead of relying on std::bitset - I don't
// see the benefit of it's 'xfer' method (see BitFlagsIO.h) saving the string name of each bit vs. being
// able to save the bits to as many bytes as needed.
// See xfer.cpp - Xfer::xferUpgradeMask. Flexibility with things being reordered kindof makes sense, but
// I don't think that benefit is all that important anyway. Doing a raw data save w/o string name checks
// there & similar (other bitflags subclasses/types) could be a new macro setting to be safe.

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/STLTypedefs.h"

class Xfer;

// Note: 'NUMBITS' should always be divisible by 8 (a byte)
template <size_t NUMBITS>
class BitFlagsRaw
{
private:
  UnsignedByte m_bits[NUMBITS/8];

public:
	void xfer(Xfer* xfer);

	void set(Int i, Int val = 1)
	{
		if (val == 1)
		{
		  // true
			m_bits[i/8] |= (1 << (i%8));
		}
		else
		{
			// false
			m_bits[i/8] &= ~(1 << (i%8));
		}
	}

	Bool test(Int i) const
	{
		return (m_bits[i/8] & (1 << (i%8))) != 0;
	}

	Int size() const
	{
		return NUMBITS;
	}

	Int count() const
	{
		int _count = 0;
		for (int i_byte = 0; i_byte < NUMBITS/8; ++i_byte)
		{
			UnsignedByte byteCopy = m_bits[i_byte];
			// Brian Kernighan's Algorithm - nice n' lean
			while (byteCopy != 0)
			{
				byteCopy &= (byteCopy - 1);
				++_count;
			}
		}
		return _count;
	}

	Bool any() const
	{
		for (int i_byte = 0; i_byte < NUMBITS/8; ++i_byte)
		{
			if (m_bits[i_byte] != 0)
			{
				return true;
			}
		}
		return false;
	}

	void flip()
	{
		for (int i_byte = 0; i_byte < NUMBITS/8; ++i_byte)
		{
			m_bits[i_byte] = ~m_bits[i_byte];
		}
	}

	// AKA reset
	void clear()
	{
		memset(&m_bits[0], 0, NUMBITS/8);
	}

	Bool anyIntersectionWith(const BitFlagsRaw& that) const
	{
		for (int i_byte = 0; i_byte < NUMBITS/8; ++i_byte)
		{
			if ((that.m_bits[i_byte] & this->m_bits[i_byte]) != 0)
			{
				return true;
			}
		}
		return false;
	}

	void clearAndSet(const BitFlagsRaw& clr, const BitFlagsRaw& set)
	{
		for (int i_byte = 0; i_byte < NUMBITS/8; ++i_byte)
		{
			this->m_bits[i_byte] &= ~clr.m_bits[i_byte];
			this->m_bits[i_byte] |= set.m_bits[i_byte];
		}
	}

	Bool testSetAndClear(const BitFlagsRaw& mustBeSet, const BitFlagsRaw& mustBeClear) const
	{
		for (int i_byte = 0; i_byte < NUMBITS/8; ++i_byte)
		{
		  if ((this->m_bits[i_byte] & mustBeClear.m_bits[i_byte]) != 0)
			{
				return false;
			}
		  if ((this->m_bits[i_byte] & ~mustBeSet.m_bits[i_byte]) != 0)
			{
				return false;
			}
		}
		return true;
	}
};
