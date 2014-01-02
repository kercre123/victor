///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by GPIO Driver
/// 
#ifndef DRV_GPIO_DEFINES_H_
#define DRV_GPIO_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------

#define ACTION_UPDATE_LEVEL        (1 << 0)
#define ACTION_UPDATE_PIN          (1 << 1)
#define ACTION_UPDATE_PAD          (1 << 2)
#define ACTION_TERMINATE_ARRAY     (1 << 3) ///< When this is set we treat this as last element of array

#define ACTION_UPDATE_ALL          (ACTION_UPDATE_LEVEL  | \
                                    ACTION_UPDATE_PIN    | \
                                    ACTION_UPDATE_PAD        )

#define PIN_LEVEL_LOW              (0x0) ///< We want to set this pin to Low 
#define PIN_LEVEL_HIGH             (0x1) ///< We want to set this pin to High
#define PIN_LEVEL_NA               (0x3) ///< Pin Level Not Applicable (e.g. Input )

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef struct
{
    unsigned char  startGpio;
    unsigned char  endGpio;
    unsigned char  action;
    unsigned char  pinLevel;
    unsigned int   pinConfig;
    unsigned int   padConfig;
} drvGpioConfigRangeType;

typedef drvGpioConfigRangeType drvGpioInitArrayType[];

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
//                                  GPIO PIN Definitions 
// ---------------------------------------------------------------------------------------------

// Bits 16,17 define the IRQ source which the GPIO can be optionally routed to
#define D_GPIO_IRQ_SRC_1            (1 << 17)
#define D_GPIO_IRQ_SRC_0            (1 << 16)
#define D_GPIO_IRQ_SRC_NONE         (0)

// bit 15                           
#define D_GPIO_WAKEUP_ON            (0)
#define D_GPIO_WAKEUP_OFF           (1 << 15) 

//bit 14 (OE Pin Inversion )                           
#define D_GPIO_OE_INVERT_ON         (1 << 14)   
#define D_GPIO_OE_INVERT_OFF        (0)   

//bit 12 (Data Pin Inversion)                           
#define D_GPIO_DATA_INV_ON          (1 << 12) 
#define D_GPIO_DATA_INV_OFF         (0) 

//bit 11                            
#define D_GPIO_BIT_SHIFT_DIR_IN     (11)
#define D_GPIO_DIR_IN               (0x00800)
#define D_GPIO_DIR_OUT              (0x00000)

//bit 10 - 3  usused
//bit 2:0                           
#define D_GPIO_MODE_0               (0x00000)
#define D_GPIO_MODE_1               (0x00001)
#define D_GPIO_MODE_2               (0x00002)
#define D_GPIO_MODE_3               (0x00003)
#define D_GPIO_MODE_4               (0x00004)
#define D_GPIO_MODE_5               (0x00005)
#define D_GPIO_MODE_6               (0x00006)
#define D_GPIO_MODE_7               (0x00007)

// ---------------------------------------------------------------------------------------------
//                                  GPIO PAD Definitions 
// ---------------------------------------------------------------------------------------------
//bit 0:1
#define D_GPIO_PAD_NO_PULL      (0x0000)
#define D_GPIO_PAD_PULL_UP      (0x0001)
#define D_GPIO_PAD_PULL_DOWN    (0x0002)
#define D_GPIO_PAD_BUS_KEEPER   (0x0003)
//bit 2:3
#define D_GPIO_PAD_DRIVE_2mA    (0x0000)
#define D_GPIO_PAD_DRIVE_4mA    (0x0004)
#define D_GPIO_PAD_DRIVE_6mA    (0x0008)
#define D_GPIO_PAD_DRIVE_8mA    (0x000C)
//bit 4                         
#define D_GPIO_PAD_VOLT_2V5     (0) 
#define D_GPIO_PAD_VOLT_1V8     (1 << 4) 
// bit5
#define D_GPIO_PAD_SLEW_FAST    (1 << 5)  // Approx 100Mhz
#define D_GPIO_PAD_SLEW_SLOW    (0)       // Approx 50Mhz
//bit 6
#define D_GPIO_PAD_SCHMITT_ON   (1 << 6) 
#define D_GPIO_PAD_SCHMITT_OFF  (0) 
//bit 7
#define D_GPIO_PAD_RECEIVER_ON  (1 << 7)  // Receive buffer in IO Pad enabled (default)
#define D_GPIO_PAD_RECEIVER_OFF (0)       // Receive buffer in IO Pad disabled to save power (output only cfg)
//bit 8
#define D_GPIO_PAD_BIAS_3V3     (1 << 8)  // Can optionally be used to increase output switching strength to improve compatibility
#define D_GPIO_PAD_BIAS_2V5     (0)       // with 3.3V logic. However the rail still only switches to 2.5V max

// bits 9-11 can be used when the main power island is disabled to ensure pin stays in desired configuration
#define D_GPIO_PAD_LOCALCTRL_ON  (1 << 9) // When this bit is set, gpio controled from scan chain via LocalData/Oen
#define D_GPIO_PAD_LOCALCTRL_OFF (0     ) 

#define D_GPIO_PAD_LOCALDATA_HI  (1 << 10) // When LOCAL_CTRL is ON; this bit selects the output value of the pin
#define D_GPIO_PAD_LOCALDATA_LO  (0      ) 

#define D_GPIO_PAD_LOCAL_PIN_IN  (1 << 10) // When LOCAL_CTRL is ON; this setting makes the pin an input
#define D_GPIO_PAD_LOCAL_PIN_OUT (0      ) // When LOCAL_CTRL is ON; this setting makes the pin an output 


// Some default configurations
#define D_GPIO_PAD_DEFAULTS  (D_GPIO_PAD_NO_PULL       |       \
                              D_GPIO_PAD_DRIVE_2mA     |       \
                              D_GPIO_PAD_VOLT_2V5      |       \
                              D_GPIO_PAD_SLEW_SLOW     |       \
                              D_GPIO_PAD_SCHMITT_OFF   |       \
                              D_GPIO_PAD_RECEIVER_ON   |       \
                              D_GPIO_PAD_BIAS_2V5      |       \
                              D_GPIO_PAD_LOCALCTRL_OFF |       \
                              D_GPIO_PAD_LOCALDATA_LO  |       \
                              D_GPIO_PAD_LOCAL_PIN_OUT      )  

#endif // DRV_GPIO_DEF_H_

