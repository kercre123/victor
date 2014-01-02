/**
 * @file DrvIcb.c
 *                Contains the driver funtions to manage interupts
 *                see headerfile DrvIcb.h for detailed descriptions
 * @brief ICB Driver Implementation
 * @author Bogdan Manciu
 */

/*===========================================================================*/
/*                            Include Headers                                */
/*===========================================================================*/
#include "isaac_registers.h"
#include "mv_types.h"
#include "DrvIcb.h"
#include "DrvIcbDefines.h"
#include "swcLeonUtilsDefines.h"
#include "assert.h"

//irq vector use driver functions to modify it's content
extern irq_handler __irq_table[__N_IRQS__];

/*===========================================================================*/
/*                                 Functions                                 */
/*===========================================================================*/
void DrvIcbSetupIrq(u32 irqSrc,u32 priority, u32 type,irq_handler function)
{
    // Disable this irq source while reconfiguring the IRQ
	DrvIcbDisableIrq(irqSrc);

    // Clear any potentially pending irqs on this source
    DrvIcbIrqClear(irqSrc);

    // setup a hanlder to be called when this irq is triggered
    DrvIcbSetIrqHandler(irqSrc, function);

    // Configure interrupt priority and type
    DrvIcbConfigureIrq(irqSrc,priority,type);

    // enable the source
    DrvIcbEnableIrq(irqSrc);

    // Ensure traps are enabled
    swcLeonEnableTraps();

    return;
}
void DrvIcbConfigureIrq( u32 source, u32 priority, u32 type ) {

    assert((source < __N_IRQS__) && (priority <= 14) && (priority > 0) && (type <=3) );
    // config trap trigger
    SET_REG_WORD( ICB_BASE_ADR +( source << 2 ), ( priority << 2 )| type );

  return;
}

void DrvIcbEnableIrq( u32 source ) {
  u32 t =( source & 0x20 )? 4 : 0;
  SET_REG_WORD( ICB_ENABLE_0_ADR + t, GET_REG_WORD_VAL( ICB_ENABLE_0_ADR + t ) |( 1 << (source - t*8) ));
}

void DrvIcbDisableIrq(u32 source) {
  u32 t =( source & 0x20 )? 4 : 0;
  SET_REG_WORD( ICB_ENABLE_0_ADR + t, GET_REG_WORD_VAL( ICB_ENABLE_0_ADR + t ) & ~( 1 << (source - t*8) ));
}

void DrvIcbSetIrqHandler( u32 source, irq_handler function ) {
  if( source < __N_IRQS__ ) {
    __irq_table[ source ] = function;
  }
}

int DrvIcbIrqPending( u32 source ) {
  u32 t =( source & 0x20 )? 4 : 0;
  return GET_REG_WORD_VAL( ICB_PEND_0_ADR + t )&( 1 << (source - t*8) );
}

void DrvIcbIrqClear( u32 source ) {
  u32 t =( source & 0x20 )? 4 : 0;
  SET_REG_WORD( ICB_CLEAR_0_ADR + t, 1 << (source - t*8) );
}

void DrvIcbIrqTrigger( u32 source ) {
  u32 t =( source & 0x20 )? 4 : 0;
  SET_REG_WORD( ICB_STATUS_0_ADR + t, 1 << (source - t*8) );
}

u32 DrvIcbIrqSrc( void ) {
	// irq level reg is 2x32
  return GET_REG_WORD_VAL( ICB_SRC_ADR ); //0x7F is an invalid source
}
