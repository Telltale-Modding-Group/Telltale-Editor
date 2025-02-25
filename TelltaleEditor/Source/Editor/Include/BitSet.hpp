#pragma once

#include <Core/Config.hpp>

#include <memory>

// HELPER FROM TELLTALE TOOL LIB
template<U32 N>
struct _BitSet_BaseN {
	static constexpr U32 _N = (N / 32) + (N % 32 == 0 ? 0 : 1);
};

/**
 Static bitset of an enum range. This is used a lot in the Telltale engine and here we use it here.
 Stores bools for each of the enum value range given.
 */
template<typename EnumClass, U32 Num, EnumClass FirstValue>
class BitSet
{
public:
	
	static constexpr U32 NumValues = Num;
	
	static constexpr U32 NumWords = _BitSet_BaseN<Num>::_N;
	
	static constexpr U32 MinValue = (U32)FirstValue;
	
	static constexpr U32 MaxValue = MinValue + NumValues;
	
protected:
	
	U32 _Words[NumWords];
	
public:
	
	BitSet()
	{
		memset(_Words, 0, NumWords << 2);
	}
	
	// Accesses
	inline Bool operator[](EnumClass index) const
	{
		TTE_ASSERT((U32)index >= MinValue && (U32)index < MaxValue, "Invalid index");
		U32 ind = (U32)index - MinValue;
		return (_Words[ind >> 5] >> (ind & 0x1F)) & 1;
	}
	
	// Assigns
	inline void Set(EnumClass index, Bool val)
	{
		TTE_ASSERT((U32)index >= MinValue && (U32)index < MaxValue, "Invalid index");
		U32 ind = (U32)index - MinValue;
		if(val)
			_Words[ind >> 5] |= (1u << (ind & 0x1F));
		else
			_Words[ind >> 5] &= ~(1u << (ind & 0x1F));
	}
	
	// sets all bits in RHS set in this one
	inline void Import(const BitSet& rhs)
	{
		for(U32 i = 0; i < NumWords; i++)
			_Words[i] |= rhs._Words[i];
	}
	
	// Counts number of set bits
	inline U32 CountBits() const
	{
		U32 cnt{};
		for(U32 i = 0; i < NumWords; i++)
			cnt += POP_COUNT(_Words[i]);
		return cnt;
	}
	
	inline EnumClass FindFirstBit(EnumClass startIndex_ = FirstValue) const
	{
		U32 startIndex = (U32)(startIndex_) - MinValue;

		U32 wordIndex = startIndex >> 5;
		U32 bitOffset = startIndex & 0x1F;
		
		for (; wordIndex < NumWords; ++wordIndex)
		{
			U32 word = _Words[wordIndex];
			
			if (bitOffset > 0)
			{
				word &= ~((1U << bitOffset) - 1);
				bitOffset = 0;
			}
			
			if (word != 0)
			{
				return (EnumClass)(((wordIndex * 32) + CLZ_BITS(word)) + MinValue);
			}
		}
		
		return (EnumClass)-1;
	}

};
