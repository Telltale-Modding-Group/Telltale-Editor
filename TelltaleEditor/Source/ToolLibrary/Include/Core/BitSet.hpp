#pragma once

#include <Core/Config.hpp>

#include <memory>
#include <array>

// HELPER FROM TELLTALE TOOL LIB (IMPROVED)
template<U32 N>
struct _BitSet_BaseN
{
    static constexpr U32 _N = (N + 31) >> 5;
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
    
    std::array<U32, NumWords> _Words;
    
public:
    
    constexpr inline BitSet() : _Words() {}

    constexpr inline BitSet(const BitSet& rhs) : _Words(rhs._Words) {}
    
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

    // keep bits in this bitset that are set in the rhs
    inline void Mask(const BitSet& rhs)
    {
        for (U32 i = 0; i < NumWords; i++)
            _Words[i] &= rhs._Words[i];
    }

    // Counts number of set bits in range
    inline U32 CountBits(EnumClass first, U32 num) const
    {
        U32 N = 0;
        for(U32 i = 0; i < num; i++)
        {
            if(operator[]((EnumClass)((U32)first + i)))
                N++;
        }
        return N;
    }
    
    // Counts number of set bits
    inline U32 CountBits() const
    {
        U32 cnt{};
        for(U32 i = 0; i < NumWords; i++)
            cnt += POP_COUNT(_Words[i]);
        return cnt;
    }
    
    // Get words
    inline U32* Words()
    {
        return _Words.data();
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
    
    // for iterating over set enum values
    class Iterator
    {
        
        const BitSet& _Bitset;
        U32 _CurrentIndex;
        
    public:
        
        inline Iterator(const BitSet& bitset, U32 startIndex)
        : _Bitset(bitset), _CurrentIndex(startIndex) {}
        
        inline Bool operator!=(const Iterator& other) const
        {
            return _CurrentIndex != other._CurrentIndex;
        }
        
        inline Iterator& operator++(int)
        {
            return ++(*this);
        }
        
        inline Iterator& operator++()
        {
            // Find the next set bit
            for (++_CurrentIndex; _CurrentIndex < NumValues; ++_CurrentIndex)
            {
                if (_Bitset[(EnumClass)_CurrentIndex])
                    break;
            }
            return *this;
        }
        
        inline EnumClass operator*() const
        {
            return (EnumClass)_CurrentIndex;
        }
        
    };
    
    inline Iterator begin() const
    {
        U32 startIndex = MinValue;
        for (; startIndex < MaxValue; ++startIndex)
        {
            if ((*this)[(EnumClass)startIndex])
                break;
        }
        return Iterator(*this, startIndex);
    }
    
    inline Iterator end() const
    {
        return Iterator(*this, MaxValue);
    }

    inline void Clear(Bool bValue)
    {
        if(bValue)
        {
            U32 nWords = NumValues >> 5;
            U32 nRemBits = NumValues & 31;
            for(U32 i = 0; i < nWords; i++)
            {
                _Words[i] = UINT32_MAX;
            }
            if(nRemBits)
            {
                _Words[nWords] = (1 << nRemBits) - 1;
            }
        }
        else
        {
            memset(_Words.data(), 0, (size_t)NumWords << 2);
        }
    }

    // Add all compile time bits
    template<EnumClass... Values>
    constexpr inline void AddEnums()
    {
        (_SetCE(Values), ...);
    }

    // Make with compile bits
    template<EnumClass... Values>
    static constexpr inline BitSet MakeWith()
    {
        BitSet _Bits{};
        _Bits.AddEnums<Values...>();
        return _Bits;
    }

private:

    static constexpr U32 _ToBitHelper(EnumClass e)
    {
        return static_cast<U32>(e);
    }

    constexpr inline void _SetCE(EnumClass e)
    {
        U32 index = _ToBitHelper(e);
        TTE_ASSERT(index >= MinValue && index < MaxValue, "Invalid enum value: outside of bit set range");
        U32 ind = index - MinValue;
        _Words[ind >> 5] |= (1u << (ind & 0x1F));
    }
    
};

template<typename EnumClass, EnumClass FirstValue, EnumClass EndValue>
using BitSetRanged = BitSet<EnumClass, (U32)EndValue - (U32)FirstValue, FirstValue>;

/**
 Simple type to help with flags.
 */
class Flags
{
    
    U32 _Value = 0;
    
public:
    
    Flags() = default;
    
    inline Flags(U32 val) : _Value(val) {}
    
    inline operator U32() const
    {
        return _Value;
    }
    
    template<typename IntOrEnum>
    inline void Add(IntOrEnum fl)
    {
        _Value |= (U32)fl;
    }
    
    template<typename IntOrEnum>
    inline void Set(IntOrEnum fl, Bool bOnOff)
    {
        if(bOnOff)
            Add(fl);
        else
            Remove(fl);
    };
    
    template<typename IntOrEnum>
    inline void Toggle(IntOrEnum fl)
    {
        Set(fl, !Test(fl));
    }
    
    template<typename IntOrEnum>
    inline void Remove(IntOrEnum fl)
    {
        _Value &= ~((U32)fl);
    }
    
    template<typename IntOrEnum>
    inline Bool Test(IntOrEnum fl) const
    {
        return (_Value & (U32)fl) != 0;
    }
    
    inline void operator+=(U32 fl)
    {
        Add(fl);
    }
    
    inline void operator-=(U32 fl)
    {
        Remove(fl);
    }
    
};
