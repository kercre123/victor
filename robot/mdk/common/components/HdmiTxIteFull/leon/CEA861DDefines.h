
/******************************************************************************

 @File         : CEA861DDefines.h
 @Author       : Alexandru Dan
 @Brief        : All Vic Codes according to Hdmi Specification EIA-CEA-861-D (July 2006)
 Date          : 28 - Oct - 2011
 E-mail        : alexandru.dan@movidius.com
 Copyright     :  Movidius Srl 2011,  Movidius Ltd 2011

 Description :
    Contains the definitions for all Vic Codes according to Hdmi Specification
    EIA-CEA-861-D (July 2006)

******************************************************************************/

#ifndef _CEA_861_D_DEFINES_H_
#define _CEA_861_D_DEFINES_H_

/******************************************************************************
 1: Exported typedefs and Constant definitions
******************************************************************************/
typedef enum StandardVicCodesDefinitions
{
    MODE_FORMAT_0,
    MODE_640x480p60_4_3,
    MODE_480p60_4_3,
    MODE_480p60_16_9,
    MODE_720p60_16_9,
    MODE_1080i60_16_9,
    MODE_480i60_4_3,
    MODE_480i60_16_9,
    MODE_720x240p60_4_3,
    MODE_720x240p60_16_9,
    MODE_2880x480i60_4_3,
    MODE_2880x480i60_16_9,
    MODE_2880x240p60_4_3,
    MODE_2880x240p60_16_9,
    MODE_1440x480p60_4_3,
    MODE_1440x480p60_16_9,
    MODE_1080p60_16_9,
    MODE_576p50_4_3,
    MODE_576p50_16_9,
    MODE_720p50_16_9,
    MODE_1080i50_16_9,
    MODE_720x576i50_4_3,
    MODE_1440x576i50_16_9,
    MODE_720x288p50_4_3,
    MODE_1440x288p50_16_9,
    MODE_2880x576i50_4_3,
    MODE_2880x576i50_16_9,
    MODE_2880x288p50_4_3,
    MODE_2880x288p50_16_9,
    MODE_1440x576p50_4_3,
    MODE_1440x576p50_16_9,
    MODE_1080p50_16_9,
    MODE_1080p24_16_9,
    MODE_1080p25_16_9,
    MODE_1080p30_16_9,
    MODE_2880x480p60_4_3,
    MODE_2880x480p60_16_9,
    MODE_2880x576p50_4_3,
    MODE_2880x576p50_16_9,
    MODE_1920x1080i50_16_9,
    MODE_1080i100_16_9,
    MODE_720p100_16_9,
    MODE_576p100_4_3,
    MODE_576p100_16_9,
    MODE_576i100_4_3,
    MODE_576i100_16_9,
    MODE_1080i120_16_9,
    MODE_720p120_16_9,
    MODE_480p120_4_3,
    MODE_480p120_16_9,
    MODE_480i120_4_3,
    MODE_480i120_16_9,
    MODE_576p200_4_3,
    MODE_576p200_16_9,
    MODE_576i200_4_3,
    MODE_576i200_16_9,
    MODE_480p240_4_3,
    MODE_480p240_16_9,
    MODE_480i240_4_3,
    MODE_480i240_16_9,
    MODE_LastCEAMode = 59,
    MODE_FP_720p_24Hz,
    MODE_FP_720p_25Hz,
    MODE_FP_720p_30Hz,
    MODE_FP_720p_50Hz,
    MODE_FP_720p_60Hz,
    MODE_FP_1080i_50Hz,
    MODE_FP_1080i_60Hz,
    MODE_FP_1080p_24Hz,
    MODE_FP_1080p_25Hz,
    MODE_FP_1080p_30Hz
} VicCodeType;

#endif //_CEA_861_D_DEFINES_H_
