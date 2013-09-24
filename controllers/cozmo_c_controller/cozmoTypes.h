#ifndef COZMO_TYPES_H
#define COZMO_TYPES_H


#include "anki/embeddedCommon.h"
//#include "anki/common.h"  //Shouldn't need to include non-embedded coretech here!
#include "anki/math/radians.h"

// Some of this should be moved to a shared file to be used by basestation as well

/*
// We specify types according to their sign and bits. We should use these in
// our code instead of the normal 'int','short', etc. because different
// compilers on different architectures treat these differently.
typedef unsigned char       u8;
typedef signed char         s8;
typedef unsigned short      u16;
typedef signed short        s16;
typedef unsigned long       u32;
typedef signed long         s32;
typedef unsigned long long  u64;
typedef signed long long    s64;
*/

// Maximum values
#define u8_MAX ((u8)0xFF)
#define u16_MAX ((u16)0xFFFF)
#define u32_MAX ((u32)0xFFFFFFFF)
#define s8_MAX ((s8)0x7F)
#define s16_MAX ((s16)0x7FFF)
#define s32_MAX ((s32)0x7FFFFFFF)
#define s8_MIN ((s8)0x80)
#define s16_MIN ((s16)0x8000)
#define s32_MIN ((s32)0x80000000)

// Memory map for our core (STM32F051)
#define CORE_CLOCK          48000000
#define CORE_CLOCK_LOW      (CORE_CLOCK / 6)
#define ROM_SIZE            65536
#define RAM_SIZE            8192
#define ROM_START           0x08000000
#define FACTORY_BLOCK       0x08001400
#define USER_BLOCK_A        0x08001800
#define USER_BLOCK_B        0x08001C00    // Must follow block A
#define APP_START           0x08002000
#define APP_END             0x08010000
#define RAM_START           0x20000000
#define CRASH_REPORT_START  0x20001fc0    // Final 64 bytes of RAM
#define VECTOR_TABLE_ENTRIES 48

// So we can explicitly specify when something is interpreted as a boolean value
typedef u8 BOOL;

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX
  #define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
  #define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifndef CLIP
  #define CLIP(n, min, max) ( MIN(MAX(min, n), max ) )
#endif

#ifndef ABS
  #define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

#ifndef SIGN
  #define SIGN(a) ((a) >= 0)
#endif

#ifndef NULL
  #define NULL (0)
#endif






///// Misc. vars that may or may not belong here

#define INVALID_IDEAL_FOLLOW_LINE_IDX s16_MAX


/// This should be in CoreTech-Common
namespace Anki{
namespace Embedded {
  
  typedef Point<float> Point2f;

  class Pose2d
  {
  public:
    // Constructors:
    Pose2d() : coord(0,0), angle(0) {}
    Pose2d(const float x, const float y, const Radians angle) ;
    Pose2d(const Pose2d &other) {
      *this = other;
    }
    
    // Accessors:
    float   get_x()     const {return coord.x;}
    float   get_y()     const {return coord.y;}
    Point2f get_xy()    const {return coord;}
    Radians get_angle() const {return angle;}

    float& x() {return coord.x;}
    float& y() {return coord.y;}
    
    void operator=(const Pose2d &other) {
      this->coord = other.coord;
      this->angle = other.angle;
    }


    Point2f coord;
    Radians angle;
    
  }; // class Pose2d

}
}

//// This is from general.h, should go in coretech_common

//////////////////////////////////////////////////////////////////////////////
// COMPARISON MACROS
//////////////////////////////////////////////////////////////////////////////


// Tolerance for which two floating point numbers are considered equal (to deal
// with imprecision in floating point representation).
const double FLOATING_POINT_COMPARISON_TOLERANCE = 1e-5;

// TRUE if x is near y by the amount epsilon, else FALSE
#define FLT_NEAR(x,y) ((x) == (y) || (((x) > (y)-(FLOATING_POINT_COMPARISON_TOLERANCE)) && ((x) < (y)+(FLOATING_POINT_COMPARISON_TOLERANCE)))) 
#define NEAR(x,y,epsilon) ((x) == (y) || (((x) > (y)-(epsilon)) && ((x) < (y)+(epsilon)))) 

// TRUE if x is within FLOATING_POINT_COMPARISON_TOLERANCE of 0.0
#define NEAR_ZERO(x) (NEAR(x, 0.0, FLOATING_POINT_COMPARISON_TOLERANCE))

// TRUE if greater than the negative of the tolerance
#define FLT_GTR_ZERO(x) ((x) >= -FLOATING_POINT_COMPARISON_TOLERANCE)

// TRUE if x >= y - tolerance
#define FLT_GE(x,y) ((x) >= (y) || (((x) >= (y)-(FLOATING_POINT_COMPARISON_TOLERANCE))))

// TRUE if x - tolerance <= y
#define FLT_LE(x,y) ((x) >= (y) || (((x)-(FLOATING_POINT_COMPARISON_TOLERANCE) <= (y))))
  
// TRUE if val is within the range [minVal, maxVal], else FALSE
#define IN_RANGE(val,minVal,maxVal) ((val) >= (minVal) && (val) <= (maxVal))

// Square of a number
#define SQUARE(x) ((x) * (x))

// Convert between millimeters and meters
#define M_TO_MM(x) ( ((double)(x)) * 1000.0 )
#define MM_TO_M(x) ( ((double)(x)) / 1000.0 )




#endif // COZMO_TYPES_H