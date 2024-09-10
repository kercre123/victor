#pragma once

/*
==============================================================================

KrotosVectorOperations.h
Created: 06 June 2018 12:36:11pm
Author:  Dave Stevenson

==============================================================================
*/

#include <memory>

class FloatVectorOperations
{
public:
	static void fill(float* dest, float valueToFill, int num) noexcept
	{
		for (int i = 0; i < num; ++i)
		{
			dest[i] = valueToFill;
		}
	}

	static void copy(float* dest, const float* src, int num) noexcept
	{
		memcpy(dest, src, (size_t)num * sizeof(float));
	}

	static void copyWithMultiply(float* dest, const float* src, float multiplier, int num) noexcept
	{
		for (int i = 0; i < num; ++i)
		{
			dest[i] = src[i] * multiplier;
		}
	}

	static void add(float* dest, const float* src, int num) noexcept
	{
		for (int i = 0; i < num; ++i)
		{
			dest[i] += src[i];
		}
	}

	static void addWithMultiply(float* dest, const float* src, float multiplier, int num) noexcept
	{
		for (int i = 0; i < num; ++i)
		{
			dest[i] += src[i] * multiplier;
		}
	}

	static void clear(float* dest, int num) noexcept
	{
		memset(dest, 0, (size_t)num * sizeof(float));
	}

	static void multiply(float* dest, float multiplier, int num)
	{
		for (int i = 0; i < num; ++i)
		{
			dest[i] *= multiplier;
		}
	}
};
