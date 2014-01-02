///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Type casts header
///

#include <moviVectorUtils.h>

/// Cast char8 to short8
///
/// This function casts char8 to short8
///
/// @param vec    char8 vector to cast
/// @return short8 vector result
#ifdef __cplusplus
extern "C"
#endif
short8 char8CastToshort8(char8 vec);

/// Cast uchar8 to ushort8
///
/// This function casts uchar8 to ushort8
///
/// @param vec    uchar8 vector to cast
/// @return ushort8 vector result
#ifdef __cplusplus
extern "C"
#endif
ushort8 uchar8CastToushort8(uchar8 vec);

/// Cast short4 to int4
///
/// This function casts short4 to int4
///
/// @param vec    short4 vector to cast
/// @return int4 vector result
#ifdef __cplusplus
extern "C"
#endif
int4 short4CastToint4(short4 vec);

/// Cast ushort4 to uint4
///
/// This function casts short4 to int4
///
/// @param vec    short4 vector to cast
/// @return int4 vector result
#ifdef __cplusplus
extern "C"
#endif
uint4 ushort4CastTouint4(ushort4 vec);

/// Cast char4 to int4
///
/// This function casts char4 to int4
///
/// @param vec    char4 vector to cast
/// @return int4 vector result
#ifdef __cplusplus
extern "C"
#endif
int4 char4CastToint4(char4 vec);

/// Cast int4 to short4
///
/// This function casts int4 to short4
///
/// @param vec    char4 vector to cast
/// @return short4 vector result
#ifdef __cplusplus
extern "C"
#endif
short4 int4CastToshort4(int4 vec);

/// Cast short8 to char8
///
/// This function casts short8 to char8
///
/// @param vec    short8 vector to cast
/// @return char8 vector result
#ifdef __cplusplus
extern "C"
#endif
char8 short8CastTochar8(short8 vec);

/// Cast int4 to char4
///
/// This function casts int4 to char4
///
/// @param vec    int4 vector to cast
/// @return char8 vector result
#ifdef __cplusplus
extern "C"
#endif
char4 int4CastTochar4(int4 vec);

/// Cast uint4 to ushort4
///
/// This function casts uint4 to ushort4
///
/// @param vec    uint4 vector to cast
/// @return ushort4 vector result
#ifdef __cplusplus
extern "C"
#endif
ushort4 uint4CastToushort4(uint4 vec);

/// Cast ushort8 to uchar8
///
/// This function casts ushort8 to uchar8
///
/// @param vec    ushort8 vector to cast
/// @return uchar8 vector result
#ifdef __cplusplus
extern "C"
#endif
uchar8 ushort8CastTouchar8(ushort8 vec);

/// Cast uint4 to uchar4
///
/// This function casts uint4 to uchar4
///
/// @param vec    uint4 vector to cast
/// @return uchar4 vector result
#ifdef __cplusplus
extern "C"
#endif
uchar4 uint4CastTouchar4(uint4 vec);
