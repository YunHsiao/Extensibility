// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Containers/BitArray.h"
#include "ExtensibilityCoreMinimal.h"

#if UE_VERSION_OLDER_THAN(5, 0, 0)
#define WordIndex DWORDIndex
#endif

/** An iterator which only iterates over set bits from the given memory range.
 * Can be copied & default constructed which just creates an empty instance. */
template<bool bConst>
class TSetBitIterator : public FRelativeBitReference
{
public:
	template<typename T>
	using TQualify = std::conditional_t<bConst, const T, T>;
	using FDataType = TQualify<uint32>*;

	TSetBitIterator() : TSetBitIterator(nullptr, 0, 0) {}

	template<typename Allocator = FDefaultBitArrayAllocator>
	explicit TSetBitIterator(TQualify<TBitArray<Allocator>>& Array, bool bReverseBits = false)
		: TSetBitIterator(FDataSource(Array.GetData(), 0, Array.Num()), bReverseBits)
	{}

	TSetBitIterator(FDataType InData, int32 StartIndex, int32 Length, bool bReverseBits = false)
		: TSetBitIterator(FDataSource(InData, StartIndex, StartIndex + Length), bReverseBits)
	{}

	/** Forwards iteration operator. */
	FORCEINLINE TSetBitIterator& operator++()
	{
		// Mark the current bit as visited.
		UnvisitedBitMask &= ~Mask;

		// Find the first set bit that hasn't been visited yet.
		FindFirstSetBit();

		return *this;
	}

	/** Forward to the first set bit after skipping specified number of bits. */
	FORCEINLINE void SkipBits(int32 Count)
	{
		CurrentBitIndex += Count;

		WordIndex = CurrentBitIndex >> NumBitsPerDWORDLogTwo;
		Mask = 1 << (CurrentBitIndex & (NumBitsPerDWORD - 1));

		UnvisitedBitMask = (~0U) << (CurrentBitIndex & (NumBitsPerDWORD - 1));
		BaseBitIndex = CurrentBitIndex & ~(NumBitsPerDWORD - 1);

		FindFirstSetBit();
	}

	FORCEINLINE friend bool operator==(const TSetBitIterator& Lhs, const TSetBitIterator& Rhs) 
	{
		// We only need to compare the bit index and the array... all the rest of the state is unobservable.
		return Lhs.CurrentBitIndex == Rhs.CurrentBitIndex && &Lhs.ArrayData == &Rhs.ArrayData;
	}

	FORCEINLINE friend bool operator!=(const TSetBitIterator& Lhs, const TSetBitIterator& Rhs)
	{ 
		return !(Lhs == Rhs);
	}

	/** conversion to "bool" returning true if the iterator is valid. */
	FORCEINLINE explicit operator bool() const
	{ 
		return CurrentBitIndex < ArrayNum; 
	}
	/** inverse of the "bool" operator */
	FORCEINLINE bool operator !() const 
	{
		return !(bool)*this;
	}

	/** Index accessor. */
	FORCEINLINE int32 GetIndex() const
	{
		return CurrentBitIndex - StartOffset;
	}

	FORCEINLINE int32 TotalBits() const
	{
		return ArrayNum - StartOffset;
	}

	FORCEINLINE bool IsValid() const
	{
		return ArrayData != nullptr;
	}

protected:

	struct FDataSource
	{
		FDataType Data;
		int32 StartIndex;
		int32 DataSize;

		FDataSource(FDataType InData, int32 InStartIndex, int32 InDataSize)
		{
			int32 DataOffset = InStartIndex / NumBitsPerDWORD;
			int32 IndexOffset = DataOffset * NumBitsPerDWORD;
			Data = InData + DataOffset;
			DataSize = InDataSize - IndexOffset;
			StartIndex = InStartIndex - IndexOffset;
		}
	};

	explicit TSetBitIterator(const FDataSource& Source, bool bInReverseBits)
		: FRelativeBitReference(Source.StartIndex)
		, ArrayData            (Source.Data)
		, bReverseBits         (bInReverseBits)
		, StartOffset          (Source.StartIndex)
		, ArrayNum             (Source.DataSize)
		, UnvisitedBitMask     ((~0U) << (Source.StartIndex & (NumBitsPerDWORD - 1)))
		, CurrentBitIndex      (Source.StartIndex)
		, BaseBitIndex         (Source.StartIndex & ~(NumBitsPerDWORD - 1))
	{
		check(CurrentBitIndex < NumBitsPerDWORD);
		check(ArrayNum < (1 << (NumBitsPerDWORD - NumBitsPerDWORDLogTwo - 1)));

		check(CurrentBitIndex <= ArrayNum);
		if (CurrentBitIndex != ArrayNum)
		{
			FindFirstSetBit();
		}
	}

	FDataType ArrayData;

	uint32 bReverseBits : 1;
	uint32 StartOffset : NumBitsPerDWORDLogTwo;
	uint32 ArrayNum : NumBitsPerDWORD - NumBitsPerDWORDLogTwo - 1;

	uint32 UnvisitedBitMask;
	uint32 CurrentBitIndex;
	uint32 BaseBitIndex;

	/** Find the first set bit starting with the current bit, inclusive. */
	void FindFirstSetBit()
	{
		const int32 LastDWORDIndex = (ArrayNum - 1) / NumBitsPerDWORD;

		// Advance to the next non-zero uint32.
		uint32 RemainingBitMask = (bReverseBits ? ~ArrayData[WordIndex] : ArrayData[WordIndex]) & UnvisitedBitMask;
		while (!RemainingBitMask)
		{
			++WordIndex;
			BaseBitIndex += NumBitsPerDWORD;
			if (WordIndex > LastDWORDIndex)
			{
				// We've advanced past the end of the array.
				CurrentBitIndex = ArrayNum;
				return;
			}

			RemainingBitMask = bReverseBits ? ~ArrayData[WordIndex] : ArrayData[WordIndex];
			UnvisitedBitMask = ~0;
		}

		// This operation has the effect of unsetting the lowest set bit of BitMask
		const uint32 NewRemainingBitMask = RemainingBitMask & (RemainingBitMask - 1);

		// This operation XORs the above mask with the original mask, which has the effect
		// of returning only the bits which differ; specifically, the lowest bit
		Mask = NewRemainingBitMask ^ RemainingBitMask;

		// If the Nth bit was the lowest set bit of BitMask, then this gives us N
		CurrentBitIndex = BaseBitIndex + NumBitsPerDWORD - 1 - FMath::CountLeadingZeros(Mask);

		// If we've accidentally iterated off the end of an array but still within the same DWORD
		// then set the index to the last index of the array
		if (CurrentBitIndex > ArrayNum)
		{
			CurrentBitIndex = ArrayNum;
		}
	}
};

using FConstSetBitIterator = TSetBitIterator<true>;

class FSetBitIterator : public TSetBitIterator<false>
{
public:
	using TSetBitIterator::TSetBitIterator;

	FORCEINLINE void UnsetCurrentBit() const
	{
		if (bReverseBits) ArrayData[WordIndex] |= Mask;
		else ArrayData[WordIndex] &= ~Mask;
	}
};

#undef WordIndex
