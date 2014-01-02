///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     H264 Encoder API defines.
///



#ifndef H264_ENCODER_API_DEFINES_H_
#define H264_ENCODER_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------

/// @brief   error codes
#define H264_ENCODE_OK         0x00
#define H264_ENCODE_INIT       0x01
#define H264_ENCODE_BUSY       0x02
#define H264_ENCODE_INVALID    0xFF

/// @brief   interrupt config
#ifndef H264_ENCODE_INTERRUPT_LEVEL
#define H264_ENCODE_INTERRUPT_LEVEL  0x05
#endif


#define H264_ENCODE_MAX_NALU_SIZE      0x40000

/// @brief   control cmdCfg field bitmasks
#define H264_ENCODE_MODE_CAVLC_BASE    (0 << 24)
#define H264_ENCODE_MODE_CAVLC_MAIN    (1 << 24)
#define H264_ENCODE_MODE_CABAC_MAIN    (2 << 24)
#define H264_ENCODE_MODE_CABAC_HIGH    (3 << 24)
#define H264_ENCODE_MODE_RATECONTROL   (1 << 28)
#define H264_ENCODE_MODE_ADAPTIVEQUANT (1 << 29)
#define H264_ENCODE_MODE_HEADER3D      (1 << 31)

#define H264_ENCODE_SVE_0_ENTROPY      (0 << 16)
#define H264_ENCODE_SVE_1_ENTROPY      (1 << 16)
#define H264_ENCODE_SVE_2_ENTROPY      (2 << 16)
#define H264_ENCODE_SVE_3_ENTROPY      (3 << 16)
#define H264_ENCODE_SVE_4_ENTROPY      (4 << 16)
#define H264_ENCODE_SVE_5_ENTROPY      (5 << 16)
#define H264_ENCODE_SVE_6_ENTROPY      (6 << 16)
#define H264_ENCODE_SVE_7_ENTROPY      (7 << 16)

#define H264_ENCODE_SVE_0_ENCODER      (1 << 0)
#define H264_ENCODE_SVE_1_ENCODER      (1 << 1)
#define H264_ENCODE_SVE_2_ENCODER      (1 << 2)
#define H264_ENCODE_SVE_3_ENCODER      (1 << 3)
#define H264_ENCODE_SVE_4_ENCODER      (1 << 4)
#define H264_ENCODE_SVE_5_ENCODER      (1 << 5)
#define H264_ENCODE_SVE_6_ENCODER      (1 << 6)
#define H264_ENCODE_SVE_7_ENCODER      (1 << 7)


// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// handle set, get of streaming
/// @brief   parameter data structure
typedef struct
{
    unsigned int address;
    unsigned int nbytes;
}h264_encode_param_t;

// this structure has to mirror the structure defined in shave config.h
/// @brief   control data structure
typedef struct
{
    u32 cmdId;
    u32 cmdCfg;
    u32 width;
    u32 height;
    u32 frameNum;
    u32 frameQp;
    u32 fps;
    u32 tgtRate;
    u32 avgRate;
    u32 frameGOP;
    u32 naluBuffer;
    u32 naluNext;
    u32 origFrame;
    u32 nextFrame;
    u32 currFrame;
    u32 refFrame;
} h264_encoder_t;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif /* H264_ENCODER_API_DEFINES_H_ */
