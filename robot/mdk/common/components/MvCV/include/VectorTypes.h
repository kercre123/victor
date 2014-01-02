#ifndef __MV_VECTOR_TYPES__
#define __MV_VECTOR_TYPES__

#ifdef __MOVICOMPILE__

#ifdef __OLD_MOVICOMPILE__
// This was defined only in mvision project to keep compatibility with pre-vectorization era
// moviCompile 50.20
#include <moviVectorUtils.h>
#else
#include <moviVectorUtils.h>
#endif

#else // PC vectorization mapping of supported Shave operations

#include <mv_types.h>

// Class for vectors with 4 elements
template<typename T, unsigned int L> 
class Vec
{
private:
	T s[L];

public:
	// Constructors ---------------------------------------------------

	// Subscripting operators -----------------------------------------
	inline T& operator [] (const int i)
	{
		assert(i >= 0 && i < L);

		return s[i];
	}

	inline T operator [] (const int i) const
	{
		assert(i >= 0 && i < L);

		return s[i];
	}

	// Binary vector operators ------------------------------------------
	inline Vec operator + (const Vec& v) const
	{
		Vec res;

		for (u32 i = 0; i < L; i++)
			res[i] = s[i] + v[i];

		return res;
	}

	inline Vec operator - (const Vec& v) const
	{
		Vec res;

		for (u32 i = 0; i < L; i++)
			res[i] = s[i] - v[i];

		return res;
	}

	inline Vec operator * (const Vec& v) const
	{
		Vec res;

		for (u32 i = 0; i < L; i++)
			res[i] = s[i] * v[i];

		return res;
	}

	inline Vec operator / (const Vec& v) const
	{
		Vec res;

		for (u32 i = 0; i < L; i++)
			res[i] = s[i] / v[i];

		return res;
	}

	// Binary scalar operators ------------------------------------------
	inline Vec operator + (const T scalar) const
	{
		Vec res;

		for (u32 i = 0; i < L; i++)
			res[i] = s[i] + scalar;

		return res;
	}

	inline Vec operator - (const T scalar) const
	{
		Vec res;

		for (u32 i = 0; i < L; i++)
			res[i] = s[i] - scalar;

		return res;
	}

	inline Vec operator * (const T scalar) const
	{
		Vec res;

		for (u32 i = 0; i < L; i++)
			res[i] = s[i] * scalar;

		return res;
	}

	inline Vec operator / (const T scalar) const
	{
		Vec res;

		for (u32 i = 0; i < L; i++)
			res[i] = s[i] / scalar;

		return res;
	}
};

template<typename T> 
class Mat4x4
{
private:
	T data[4][4];

public:
	T* rows[4];

	Mat4x4()
	{
		rows[0] = data[0];
		rows[1] = data[1];
		rows[2] = data[2];
		rows[3] = data[3];
	}
};

// Native vector data types - 128bit vectors
typedef Vec<s32, 4>		int4;
typedef Vec<u32, 4>		uint4;
typedef Vec<s32, 4>		long4;
typedef Vec<u32, 4>		ulong4;
typedef Vec<fp32, 4>	float4;
typedef Vec<s16, 8>		short8;
typedef Vec<u16, 8>		ushort8;
typedef Vec<fp16, 8>	half8;
typedef Vec<s8, 16>		char16;
typedef Vec<u8, 16>		uchar16;

// Additional vector types - less than 128bit vectors
typedef Vec<fp32, 2>	float2;
typedef Vec<s32, 2>		int2;
typedef Vec<s16, 2>		short2;
typedef Vec<u8, 4>		uchar4;

// Native matrix data types - 128bit vectors as rows
typedef Mat4x4<fp32>	float4x4;
typedef Mat4x4<s32>		int4x4;

#endif

#endif