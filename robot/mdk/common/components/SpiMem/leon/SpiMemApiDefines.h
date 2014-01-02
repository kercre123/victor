///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
///
///

#ifndef SPIMEMAPIDEFINES_H_
#define SPIMEMAPIDEFINES_H_

typedef struct {
    u32     base;
    u32     size;

    u32     sectorSize;
    u32     sectorCount;
    u32     sectorPerBlock;
    u32     pageSize;

} tySpiMemState;

#endif /* SPIMEMAPIDEFINES_H_ */
