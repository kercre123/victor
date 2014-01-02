// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : FP16 implementation
//
// -----------------------------------------------------------------------------

#ifndef __SIPP_FP16__
#define __SIPP_FP16__

#include "moviFloat32.h"

#define FP16_MAX           65504.0f

#define FP16_SIGN          (1<<(sizeof(fp16)*8-1))
#define HANDLE_NEGZERO(x)  if(EXTRACT_F16_EXP(x.getPackedValue()) == 0 && EXTRACT_F16_FRAC(x.getPackedValue()) == 0) \
                             x.setPackedValue(x.getPackedValue() ^ (x.getPackedValue() & FP16_SIGN));      

// Conversion macros
#define U8F_TO_FP16(x)     fp16((x)*(1.0f/255))
#define FP16_TO_U8F(x)     (F16_IS_NAN((x).getPackedValue()) ? (EXTRACT_F16_SIGN((x).getPackedValue()) ? 0x0 : 0xFF) : uint8_t(floor(((x)*(255)) + 0.5f)))
#define U12F_TO_FP16(x)    fp16((x)*(1.0f/4095))
#define FP16_TO_U12F(x)    uint16_t(floor(((x)*(4095)) + 0.5f))
#define U8F_TO_U12F(x)     (x << 4) | (x >> 4)
#define U12F_TO_U8F(x)     (x >> 4)
#define U16F_TO_FP16(x)    fp16((x)/65535.f)
#define FP16_TO_U16F(x)    uint16_t(floor(((x)*(65535.)) + 0.5))

class fp16{
private:
	unsigned short _h;
public:
	/* Constructors */
	fp16();
	fp16(float v);
	fp16(double v);

	/* Destructor */
	~fp16();

	/* Auxiliary swap function for assignments */
	friend void swap(fp16&, fp16&);
	
	/* General purpose set and get functions */
	void  setUnpackedValue(float);
	void  setPackedValue(unsigned short);
	float getUnpackedValue(void);
	unsigned short getPackedValue(void);
	
	/* Binary Comparison Operators */
	bool operator == (const fp16);
	bool operator != (const fp16);
	bool operator >  (const fp16);
	bool operator <  (const fp16);
	bool operator >= (const fp16);
	bool operator <= (const fp16);
	bool operator == (const float);
	bool operator != (const float);
	bool operator >  (const float);
	bool operator <  (const float);
	bool operator >= (const float);
	bool operator <= (const float);

	/* Assignment Operators return a reference to a fp16 object in order to allow chained assignments, such as: f1 = f2 = f3 (supported with primitive types) */
	fp16 & operator = (fp16);
	fp16 & operator = (float);
	//fp16 & operator = (double);

	/* Compound Assignment Operators */
	fp16 & operator += (const  fp16);
	fp16 & operator -= (const  fp16);
	fp16 & operator *= (const  fp16);
	fp16 & operator /= (const  fp16);
	fp16 & operator += (const float);
	fp16 & operator -= (const float);
	fp16 & operator *= (const float);
	fp16 & operator /= (const float);

	/* Binary Arithmetic Operators 
	 * -There's quite a major pitfall if these operators are overloaded; namely, the built-in 
	 * operator for each of these arithmetic operations is overloaded and the compiler will
	 * constrain the end-user to use a trailing f if the second operand is an immediate value,
	 * so that it is explicitly specified it as being a float value, otherwise it will fail to 
	 * find the proper overloaded operator and will generate an error (unless the operator
	 * is overloaded with every single primitive type). Leaving it like that just makes the
	 * compiler use the built-in operator+(float, float) which will get the right parameters
	 * due to the conversion operator.
	 */
	//fp16 operator + (const  fp16);
	//fp16 operator - (const  fp16);
	//fp16 operator * (const  fp16);
	//fp16 operator / (const  fp16);
	//fp16 operator + (const float);
	//fp16 operator - (const float);
	//fp16 operator * (const float);
	//fp16 operator / (const float);

	/* Conversion Operator */
	operator float ();

	/* Unary Minus Operator */
	fp16 operator - () const;

	/* Absolute Value Operator 
	 * -This is redundant since fabs can be called directly with a fp16 instance (let me know if I should remove it)
	 */
	friend float fp16abs(fp16);
};

#endif // __SIPP_FP16__
