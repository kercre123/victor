#ifndef __HARDWARE_H
#define __HARDWARE_H

#include <stdint.h>
#include "common.h"
//#include "stm32f0xx.h"

static const uint32_t SYSTEM_CLOCK = 48000000;
static const uint32_t CONTACT_BAUDRATE = 57600;
static const uint32_t COMMS_BAUDRATE = 3000000;

// H-Bridge
namespace LP1 GPIO_DEFINE(F, 0);
namespace LN1 GPIO_DEFINE(A, 10);
namespace LN2 GPIO_DEFINE(A, 11);

namespace HP1 GPIO_DEFINE(F, 1);
namespace HN1 GPIO_DEFINE(B, 5);
namespace HN2 GPIO_DEFINE(B, 1);

namespace RTP1 GPIO_DEFINE(B, 12);
namespace RTN1 GPIO_DEFINE(A, 8);
namespace RTN2 GPIO_DEFINE(A, 7);

namespace LTP1 GPIO_DEFINE(A, 15);
namespace LTN1 GPIO_DEFINE(A, 9);
namespace LTN2 GPIO_DEFINE(B, 0);

// Encoders
namespace HENCA GPIO_DEFINE(A, 1);
namespace HENCB GPIO_DEFINE(A, 0);
namespace LENCA GPIO_DEFINE(B, 3);
namespace LENCB GPIO_DEFINE(B, 2);
namespace RTENC GPIO_DEFINE(C, 14);
namespace LTENC GPIO_DEFINE(C, 15);

//-----------Hacky P3 (XXX: replace with syscon after master merge)- -------------------

namespace BAT_EN          GPIO_DEFINE(A, 3);
namespace nVEXT_EN        GPIO_DEFINE(B, 9);
namespace POWER_EN        GPIO_DEFINE(A, 6); //PWR_EN, POWER_B
namespace nVDDs_EN        GPIO_DEFINE(C, 13);

namespace VEXT_SENSE      GPIO_DEFINE(A, 2); //VEXT_SENSE_TX
namespace VBAT_SENSE      GPIO_DEFINE(A, 4);

namespace CHRG1           GPIO_DEFINE(F, 1); //HP1
namespace CHRG2           GPIO_DEFINE(F, 0); //LP1
//namespace nCHG_EN         GPIO_DEFINE(A, 15); //LTP1
namespace CHG_EN          GPIO_DEFINE(A, 15); //LTP1 -- Note: polarity inverted for DVT1-A??
namespace nCHG_HC         GPIO_DEFINE(B, 12); //RTP1

//------------?P2?--------------------------------
/*
// Power
namespace POWER_EN GPIO_DEFINE(A, 6); //POWER_B
namespace nVDDs_EN GPIO_DEFINE(C, 13);
namespace VEXT_SENSE GPIO_DEFINE(A, 2);
namespace VBAT_SENSE GPIO_DEFINE(A, 4);

// Charging
//namespace nCHG_EN GPIO_DEFINE(A, 12);
//namespace nCHG_HC GPIO_DEFINE(B, 12);
namespace CHRG1 GPIO_DEFINE(F, 1);
namespace CHRG2 GPIO_DEFINE(F, 0);

// Microphones
namespace MIC_LR      GPIO_DEFINE(B, 15);
namespace MIC2_MISO   GPIO_DEFINE(B, 4);
namespace MIC1_MISO   GPIO_DEFINE(B, 14);
namespace MIC2_SCK    GPIO_DEFINE(A, 5);
namespace MIC1_SCK    GPIO_DEFINE(B, 13);

// Cap Sense
//namespace CAPI GPIO_DEFINE(B, 8);
//namespace CAPO GPIO_DEFINE(A, 14);

// Communication
namespace VEXT_TX GPIO_DEFINE(A, 2);
//namespace BODY_TX GPIO_DEFINE(B, 6);
//namespace BODY_RX GPIO_DEFINE(B, 7);
//namespace SCL1 GPIO_DEFINE(B, 10);
//namespace SDA1 GPIO_DEFINE(B, 11);
//namespace SCL2 GPIO_DEFINE(F, 6);
//namespace SDA2 GPIO_DEFINE(F, 7);

// Lights
//namespace LED_DAT GPIO_DEFINE(A, 13);
//namespace LED_CLK GPIO_DEFINE(A, 14);
*/

#endif
