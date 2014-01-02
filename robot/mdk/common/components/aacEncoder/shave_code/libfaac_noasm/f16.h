#ifndef F16_H
#define F16_H



//typedef float		   Float;
//typedef unsigned short Float16;
typedef half Float16;
//typedef half Float16;


//#########################################################
static inline float f16_to_f32(half x)
{
  //half h = *((half*)&x);
  return (float)x;
}

//#############################################
static inline half f32_to_f16(float x)
{
  //half h = (half)x;
  //return *((half*)&h);
  return (half)x;
}

#endif
