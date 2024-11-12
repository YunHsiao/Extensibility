// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>

template<typename T>
class TRelaxedAtomicCounter
{
public:
	TRelaxedAtomicCounter() = default;

	TRelaxedAtomicCounter(T InValue)
		: Value(InValue)
	{
		static_assert(TIsArithmetic<T>::Value || TIsPointer<T>::Value, "Only arithmetic or pointer types are supported");
	}

	TRelaxedAtomicCounter(const TRelaxedAtomicCounter<T>& InValue)
		: Value(InValue.Load())
	{}

	FORCEINLINE T operator->() const
	{
		return Load();
	}

	FORCEINLINE T Get() const
	{
		return Load();
	}

	FORCEINLINE T operator+(T InValue) const
	{
		return InValue + Load();
	}

	FORCEINLINE T operator-(T InValue) const
	{
		return InValue - Load();
	}

	FORCEINLINE T operator*(T InValue) const
	{
		return InValue * Load();
	}

	FORCEINLINE T operator&(T InValue) const
	{
		return InValue & Load();
	}

	FORCEINLINE T operator|(T InValue) const
	{
		return InValue | Load();
	}

	FORCEINLINE T operator^(T InValue) const
	{
		return InValue ^ Load();
	}

	FORCEINLINE bool operator==(T InValue) const
	{
		return InValue == Load();
	}

	FORCEINLINE bool operator!=(T InValue) const
	{
		return InValue != Load();
	}

	template<typename TargetType>
	FORCEINLINE operator TargetType() const
	{
		return (TargetType)Load();
	}

	FORCEINLINE T operator=(T InValue)
	{
		return Store(InValue);
	}

	FORCEINLINE TRelaxedAtomicCounter<T>& operator=(const TRelaxedAtomicCounter<T>& InValue)
	{
		Store(InValue.Load());
		return *this;
	}

	FORCEINLINE T operator++(int)
	{
		return FetchAdd(1);
	}

	FORCEINLINE T operator--(int)
	{
		return FetchSub(1);
	}

	FORCEINLINE T FetchAdd(T InValue)
	{
		return Value.fetch_add(InValue, std::memory_order_relaxed);
	}

	FORCEINLINE T FetchSub(T InValue)
	{
		return Value.fetch_sub(InValue, std::memory_order_relaxed);
	}

	FORCEINLINE T FetchAnd(T InValue)
	{
		return Value.fetch_and(InValue, std::memory_order_relaxed);
	}

	FORCEINLINE T FetchOr(T InValue)
	{
		return Value.fetch_or(InValue, std::memory_order_relaxed);
	}

	FORCEINLINE T FetchXor(T InValue)
	{
		return Value.fetch_xor(InValue, std::memory_order_relaxed);
	}

	FORCEINLINE T Exchange(T InValue)
	{
		return Value.exchange(InValue, std::memory_order_relaxed);
	}

	FORCEINLINE bool CompareAndSwap(T& Expected, T Desired)
	{
		return Value.compare_exchange_strong(Expected, Desired, std::memory_order_relaxed, std::memory_order_relaxed);
	}

	// Use this if a loop is needed
	FORCEINLINE bool CompareAndSwapWeak(T& Expected, T Desired)
	{
		return Value.compare_exchange_weak(Expected, Desired, std::memory_order_relaxed, std::memory_order_relaxed);
	}

	// For special cases where more strict memory order is required
	FORCEINLINE std::atomic<T>& GetAtomic()
	{
		return Value;
	}

private:
	FORCEINLINE T Load() const
	{
		return Value.load(std::memory_order_relaxed);
	}

	FORCEINLINE T Store(T InValue)
	{
		Value.store(InValue, std::memory_order_relaxed);
		return InValue;
	}

	std::atomic<T> Value;
};
