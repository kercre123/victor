/**
 * Clock settings - derived from Freescale example code
 */
#include <stdint.h>
#include "MK02F12810.h"

#define DISABLE_WDOG                 1
#define CLOCK_SETUP                  1

extern "C" void SystemInit (void);

/* MCG mode constants */

#define MCG_MODE_FEI                   0U
#define MCG_MODE_FBI                   1U
#define MCG_MODE_BLPI                  2U
#define MCG_MODE_FEE                   3U
#define MCG_MODE_FBE                   4U
#define MCG_MODE_BLPE                  5U

/* Define clock source values */
#define CPU_XTAL_CLK_HZ                10000000u           /* Value of the external crystal or oscillator clock frequency in Hz */
#define CPU_XTAL32k_CLK_HZ             0u                  /* No RTC clock available */
#define CPU_INT_SLOW_CLK_HZ            32768u              /* Value of the slow internal oscillator clock frequency in Hz  */

/* ----------------------------------------------------------------------------
   -- Core clock settings
   ---------------------------------------------------------------------------- */

/* Low power mode enable */
/* SMC_PMPROT: AHSRUN=1,AVLP=1,ALLS=1,AVLLS=1 */
#define SYSTEM_SMC_PMPROT_VALUE        0xAAU               /* SMC_PMPROT */

#define MCG_MODE                     MCG_MODE_FEI /* Clock generator mode */
/* MCG_C1: CLKS=0,FRDIV=3,IREFS=1,IRCLKEN=1,IREFSTEN=0 - Divide by 256 */
#define SYSTEM_MCG_C1_VALUE          0x1EU               /* MCG_C1 */
/* MCG_C2: LOCRE0=0,FCFTRIM=0,RANGE=2,HGO=0,EREFS=1,LP=0,IRCS=0 */
#define SYSTEM_MCG_C2_VALUE          0x24U               /* MCG_C2 */
/* MCG_C4: DMX32=0,DRST_DRS=3,FCTRIM=0,SCFTRIM=0 */
#define SYSTEM_MCG_C4_VALUE          0x60U               /* MCG_C4 */
/* MCG_SC: ATME=0,ATMS=0,ATMF=0,FLTPRSRV=0,FCRDIV=0,LOCS0=0 */
#define SYSTEM_MCG_SC_VALUE          0x00U               /* MCG_SC */
/* MCG_C5:  */
#define SYSTEM_MCG_C5_VALUE          0x00U               /* MCG_C5 */
/* MCG_C6: CME=0 */
#define SYSTEM_MCG_C6_VALUE          0x00U               /* MCG_C6 */
/* MCG_C7: OSCSEL=0 */
#define SYSTEM_MCG_C7_VALUE          0x00U               /* MCG_C7 */
/* OSC_CR: ERCLKEN=1,EREFSTEN=0,SC2P=0,SC4P=0,SC8P=0,SC16P=0 */
#define SYSTEM_OSC_CR_VALUE          0x80U               /* OSC_CR */
/* SMC_PMCTRL: RUNM=3,STOPA=0,STOPM=0 */
#define SYSTEM_SMC_PMCTRL_VALUE      0x60U               /* SMC_PMCTRL */
/* SIM_CLKDIV1: OUTDIV1=0,OUTDIV2=1,OUTDIV4=3 */
#define SYSTEM_SIM_CLKDIV1_VALUE     0x01030000U         /* SIM_CLKDIV1 */
/* SIM_SOPT1: OSC32KSEL=3,OSC32KOUT=0,RAMSIZE=0 */
#define SYSTEM_SIM_SOPT1_VALUE       0x000C0000U         /* SIM_SOPT1 */
/* SIM_SOPT2: PLLFLLSEL=0,TRACECLKSEL=0,CLKOUTSEL=0 */
#define SYSTEM_SIM_SOPT2_VALUE       0x00U               /* SIM_SOPT2 */

/* ----------------------------------------------------------------------------
   -- SystemInit() sets up clocks to run on internal oscillator
   ---------------------------------------------------------------------------- */
void SystemInit (void)
{
  // Enable FPU
  SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));    /* set CP10, CP11 Full Access */

#if (DISABLE_WDOG)
  /* WDOG->UNLOCK: WDOGUNLOCK=0xC520 */
  WDOG->UNLOCK = WDOG_UNLOCK_WDOGUNLOCK(0xC520); /* Key 1 */
  /* WDOG->UNLOCK: WDOGUNLOCK=0xD928 */
  WDOG->UNLOCK = WDOG_UNLOCK_WDOGUNLOCK(0xD928); /* Key 2 */
  /* WDOG->STCTRLH: ?=0,DISTESTWDOG=0,BYTESEL=0,TESTSEL=0,TESTWDOG=0,?=0,?=1,WAITEN=1,STOPEN=1,DBGEN=0,ALLOWUPDATE=1,WINEN=0,IRQRSTEN=0,CLKSRC=1,WDOGEN=0 */
  WDOG->STCTRLH = WDOG_STCTRLH_BYTESEL(0x00) |
                 WDOG_STCTRLH_WAITEN_MASK |
                 WDOG_STCTRLH_STOPEN_MASK |
                 WDOG_STCTRLH_ALLOWUPDATE_MASK |
                 WDOG_STCTRLH_CLKSRC_MASK |
                 0x0100U;
#endif /* (DISABLE_WDOG) */

  // Release hold when recovering from VLLSx
  if((RCM->SRS0 & RCM_SRS0_WAKEUP_MASK) != 0x00U)
    if((PMC->REGSC & PMC_REGSC_ACKISO_MASK) != 0x00U)
       PMC->REGSC |= PMC_REGSC_ACKISO_MASK;

  /* Power mode protection initialization */
  SMC->PMPROT = SYSTEM_SMC_PMPROT_VALUE;

  /* High speed run mode enable */
#if (((SYSTEM_SMC_PMCTRL_VALUE) & SMC_PMCTRL_RUNM_MASK) == (0x03U << SMC_PMCTRL_RUNM_SHIFT))
  SMC->PMCTRL = (uint8_t)((SYSTEM_SMC_PMCTRL_VALUE) & (SMC_PMCTRL_RUNM_MASK)); /* Enable HSRUN mode */
  while(SMC->PMSTAT != 0x80U) {      /* Wait until the system is in HSRUN mode */
  } 
#endif

  /* Set system prescalers and clock sources */
  SIM->CLKDIV1 = SYSTEM_SIM_CLKDIV1_VALUE; /* Set system prescalers */
  SIM->SOPT1 = ((SIM->SOPT1) & (uint32_t)(~(SIM_SOPT1_OSC32KSEL_MASK))) | ((SYSTEM_SIM_SOPT1_VALUE) & (SIM_SOPT1_OSC32KSEL_MASK)); /* Set 32 kHz clock source (ERCLK32K) */
  SIM->SOPT2 = ((SIM->SOPT2) & (uint32_t)(~(SIM_SOPT2_PLLFLLSEL_MASK))) | ((SYSTEM_SIM_SOPT2_VALUE) & (SIM_SOPT2_PLLFLLSEL_MASK)); /* Selects the high frequency clock for various peripheral clocking options. */

/*** MCG_MODE is FEI */
  
  /* Set MCG and OSC */
#if  (((SYSTEM_OSC_CR_VALUE) & OSC_CR_ERCLKEN_MASK) != 0x00U)
  /* SIM_SCGC5: PORTA=1 */
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
  /* PORTA_PCR18: ISF=0,MUX=0 */
  PORTA_PCR18 &= (uint32_t)~(uint32_t)((PORT_PCR_ISF_MASK | PORT_PCR_MUX(0x07)));
  if (((SYSTEM_MCG_C2_VALUE) & MCG_C2_EREFS_MASK) != 0x00U) {
  /* PORTA_PCR19: ISF=0,MUX=0 */
  // PORTA_PCR19 &= (uint32_t)~(uint32_t)((PORT_PCR_ISF_MASK | PORT_PCR_MUX(0x07)));
  }
#endif
  
  MCG->SC = SYSTEM_MCG_SC_VALUE;       /* Set SC (fast clock internal reference divider) */
  MCG->C1 = SYSTEM_MCG_C1_VALUE;       /* Set C1 (clock source selection, FLL ext. reference divider, int. reference enable etc.) */

  /* Check that the source of the FLL reference clock is the requested one. */
  if (((SYSTEM_MCG_C1_VALUE) & MCG_C1_IREFS_MASK) != 0x00U) {
    while((MCG->S & MCG_S_IREFST_MASK) == 0x00U) {
    }
  } else {
    while((MCG->S & MCG_S_IREFST_MASK) != 0x00U) {
    }
  }
  
  // Set up dividers
  MCG->C2 = (MCG->C2 & (uint8_t)(~(MCG_C2_FCFTRIM_MASK))) | (SYSTEM_MCG_C2_VALUE & (uint8_t)(~(MCG_C2_LP_MASK))); /* Set C2 (freq. range, ext. and int. reference selection etc. excluding trim bits; low power bit is set later) */
  MCG->C4 = ((SYSTEM_MCG_C4_VALUE) & (uint8_t)(~(MCG_C4_FCTRIM_MASK | MCG_C4_SCFTRIM_MASK))) | (MCG->C4 & (MCG_C4_FCTRIM_MASK | MCG_C4_SCFTRIM_MASK)); /* Set C4 (FLL output; trim values not changed) */
  OSC->CR = SYSTEM_OSC_CR_VALUE;       /* Set OSC_CR (OSCERCLK enable, oscillator capacitor load) */
  MCG->C7 = SYSTEM_MCG_C7_VALUE;       /* Set C7 (OSC Clock Select) */
  MCG->C6 = SYSTEM_MCG_C6_VALUE;

  /* Wait until output of the FLL is selected */
  while((MCG->S & MCG_S_CLKST_MASK) != 0x00U) {
  }
  
  /* Use LPTMR to wait for 1ms for FLL clock stabilization */
  SIM_SCGC5 |= SIM_SCGC5_LPTMR_MASK;   /* Alow software control of LPMTR */
  LPTMR0->CMR = LPTMR_CMR_COMPARE(0);  /* Default 1 LPO tick */
  LPTMR0->CSR = (LPTMR_CSR_TCF_MASK | LPTMR_CSR_TPS(0x00));
  LPTMR0->PSR = (LPTMR_PSR_PCS(0x01) | LPTMR_PSR_PBYP_MASK); /* Clock source: LPO, Prescaler bypass enable */
  LPTMR0->CSR = LPTMR_CSR_TEN_MASK;    /* LPMTR enable */
  while((LPTMR0_CSR & LPTMR_CSR_TCF_MASK) == 0u) {
  }
  LPTMR0_CSR = 0x00;                   /* Disable LPTMR */
  SIM_SCGC5 &= (uint32_t)~(uint32_t)SIM_SCGC5_LPTMR_MASK;
  
#if (((SYSTEM_SMC_PMCTRL_VALUE) & SMC_PMCTRL_RUNM_MASK) == (0x02U << SMC_PMCTRL_RUNM_SHIFT))
  SMC->PMCTRL = (uint8_t)((SYSTEM_SMC_PMCTRL_VALUE) & (SMC_PMCTRL_RUNM_MASK)); /* Enable VLPR mode */
  while(SMC->PMSTAT != 0x04U) {      /* Wait until the system is in VLPR mode */
  }
#endif
}
