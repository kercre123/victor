#ifndef DRV_ICB_H
#define DRV_ICB_H

/**
 * @file DrvIcb.h
 *                MOVIDIA Trap Managing Drivers\n
 *                Copyright 2008, 2009 MOVIDIA Ltd.\n
 *                Header file with description to all functions
 * @brief Public ICB Driver Functions
 * @author Bogdan Manciu
*/

/*===========================================================================*/
/*                                     Include Headers                       */
/*===========================================================================*/
#include "mv_types.h"
#include "registersMyriad.h"
#include "DrvIcbDefines.h"
#include "swcLeonUtils.h"

/*===========================================================================*/
/*                            Type Declarations                              */
/*===========================================================================*/
/**
 * @typedef *irq_handler
 *        Function type\n
 *        When treating an interrupt one will declare such a function\n
 *        and use DrvIcbSetIrqHandler( u32 source, irq_handler function )\n
 *        to assign it to a specific source
 */

typedef void (*irq_handler)( u32 source ); // function in the rom

/*===========================================================================*/
/*                                 Functions                                 */
/*===========================================================================*/


/// Setup an IRQ handler
/// 
/// Performs the steps necessary to enable a Myriad IRQ
/// @param[in] irqSrc (see DrvIcbDefines.h) e.g. IRQ_UART
/// @param[in] priority priority in the range of 1-14 (higher has greater priority)
/// @param[in] type of either POS_EDGE_INT, NEG_EDGE_INT, POS_LEVEL_INT, NEG_LEVEL_INT 
/// @param[in] function pointer to irq hanlder (type void funct(u32 src); )
/// @return    void  (asserts on error)
void DrvIcbSetupIrq(u32 irqSrc,u32 priority, u32 type,irq_handler function);

/**
 * @fn void DrvIcbConfigureIrq( u32 source, u32 priority, u32 type )
 *     Configures the way an irq is generated( eg. edge/level positive/negative )
 * @param source - trap number in the docs Table1\n
 *                 defines for each source are available in DrvIcbDefines.h
 * @param priority - irq priority from 0 to 15( 15 = NMI )
 * @param type     - level or edge see DrvIcbDefines.h
 * @return  - void
 */
void DrvIcbConfigureIrq( u32 source, u32 priority, u32 type );

/**
 * @fn void DrvIcbSetIrqHandler( u32 source, irq_handler function );
 *     Assigns a handler function to a specific hardware irq source
 * @param source - trap number  ( see DrvIcbDefines.h defines )
 * @param function - user declared function following\n
 *                   the irq_handler function pattern\n
 *                   see tc_icb_0 for an example funtion\n
 * Note. User must clear PIL and Interrupt_pending before returning from trap handler.
 */
 void DrvIcbSetIrqHandler( u32 source, irq_handler function );

/**
 * @{
 * @name Enable/Disable Traps
 *  Enable/Disable Traps for a specific Source
 * @param source - trap number ( see DrvIcbDefines.h defines )
 */
void DrvIcbEnableIrq(  u32 source );
void DrvIcbDisableIrq( u32 source );
/**@}
 * @fn void DrvIcbIrqClear( u32 source )
 *     Clears the Pending Flag for the Source IRQ\n
 *     This should be done at the end of every *irq_handler function\n
 *     But only after calling drvIcbClrPsrPil
 * @param source - irq source to get cleared
 */
void DrvIcbIrqClear( u32 source );

/**
 * @fn void DrvIcbIrqTrigger( u32 source )
 *     Forces an interrupt to trigger for the Source IRQ\n
 * @param source - irq source to be trigger
 */
void DrvIcbIrqTrigger( u32 source );

/**
 * @fn int DrvIcbIrqPending( u32 source );
 *     Gives the Pending Flag for the Source IRQ\n
 * @param source - irq source
 * @return 1 if the source irq is pending\n
 *         and 0 if it's not
 */
int DrvIcbIrqPending( u32 source );

/**
 * @fn u32 DrvIcbIrqSrc( void );
 *     Gives the Current Interupt Source
 * @return A SEVEN bit value which is the IRQ Source\n
 */
u32 DrvIcbIrqSrc( void );

/**
 * @fn static inline void drvIcbClrPsrPil( void );
 *
 * DO NOT USE: deprecated: use swcLeonSetPIL(0) instead!
 *
 * Clears PIL from PSR register allowing for any trap\n
 * to occur on any level
 */
static inline void drvIcbClrPsrPil( void ) __attribute__((deprecated));
static inline void drvIcbClrPsrPil( void ) {
    swcLeonSetPIL(0);
}

/**
 * @{
 * @name Enable/Disable Traps
 *
 * DO NOT USE: deprecated: use swcLeonDisableTraps() instead!
 *                          or swcLeonEnableTraps() instead!
 *
 *     Disables CPU Trap Handling by clearing Leon's ET flag in PSR
 */
static inline void drvIcbDisableTraps() __attribute__((deprecated));
static inline void drvIcbDisableTraps() {
    swcLeonDisableTraps();
}

static inline void drvIcbEnableTraps() __attribute__((deprecated));
static inline void drvIcbEnableTraps() {
    swcLeonEnableTraps();
}
///@}

/**
 * @fn static inline void drvIcbSetPil( u32 pil );
 *
 * DO NOT USE: deprecated: use swcLeonSetPIL(pil) instead!
 *
 * Set IRQ Level in the Leon's PSR
 */
static inline void drvIcbSetPil( u32 pil ) __attribute__((deprecated));
static inline void drvIcbSetPil( u32 pil ) {
    swcLeonSetPIL(pil);
}

/**
 * @name Get IRQ Level
 *     Returns PIL field from the Leon's PSR.
 *
 *    DO NOT USE: deprecated: use swcLeonGetPIL() instead!
 */
static inline u32 drvIcbGetPil() __attribute__((deprecated));
static inline u32 drvIcbGetPil() {
    return swcLeonGetPIL();
}

/**
 * @fn u32 drvIcbTrapsStatus();
 *      Use to read the ET flag from Leon's PSR
 * @return A 32 bit value representative of ET value. If !=0 then Traps Are ENABLED
 *
 *  DO NOT USE: deprecated: use swcLeonAreTrapsEnabled() instead!
 */
static inline u32 drvIcbTrapsStatus() __attribute__((deprecated));
static inline u32 drvIcbTrapsStatus() {
    return swcLeonAreTrapsEnabled();
}

/**
 * Reat the leon's PSR register
 * DO NOT USE: deprecated: use swcLeonGetPSR() instead!
 */
static inline u32 drvIcbGetPsr() __attribute__((deprecated));
static inline u32 drvIcbGetPsr() {
    return swcLeonGetPSR();
}

#endif // DRV_ICB_H
