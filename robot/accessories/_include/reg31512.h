/*--------------------------------------------------------------------------
 * reg3151x.h
 *
 * Keil C51 header file for the Nordic Semiconductor nRF31512, nRF31504 and nRF31502
 * 2.4GHz RF transceiver with embedded 8051 compatible microcontroller.
 *
 *
 *------------------------------------------------------------------------*/
#ifndef REG3151X_H__
#define REG3151X_H__

//-----------------------------------------------------------------------------
// Byte Registers
//-----------------------------------------------------------------------------

sfr   P0           = 0x80;
sfr   SP           = 0x81;
sfr   DPL          = 0x82;
sfr   DPH          = 0x83;
sfr   DPL1         = 0x84;
sfr   DPH1         = 0x85;
sfr   PCON         = 0x87;
sfr   TCON         = 0x88;
sfr   TMOD         = 0x89;
sfr   TL0          = 0x8A;
sfr   TL1          = 0x8B;
sfr   TH0          = 0x8C;
sfr   TH1          = 0x8D;
sfr   P1           = 0x90;
sfr   DPS          = 0x92;
sfr   P0DIR        = 0x93;
sfr   P1DIR        = 0x94;
// sfr   P2CON        = 0x97;   // XXX Undocumented in nRF31512
sfr   P0CON        = 0x9E;
sfr   P1CON        = 0x9F;
// sfr   P2           = 0xA0;   // XXX Undocumented in nRF31512
sfr   CLKCTRL      = 0xA3;
sfr   PWRDWN       = 0xA4;
sfr   WUCON        = 0xA5;
sfr   INTEXP       = 0xA6;
sfr   MEMCON       = 0xA7;
sfr   IEN0         = 0xA8;
sfr   IP0          = 0xA9;
sfr   RTC2CPT01    = 0xAB;
sfr   RTC2CPT10    = 0xAC;
sfr   CLKLFCTRL    = 0xAD;
sfr   OPMCON       = 0xAE;
sfr   WDSV         = 0xAF;
sfr   RSTREAS      = 0xB1;
sfr   RTC2CON      = 0xB3;
sfr   RTC2CMP0     = 0xB4;
sfr   RTC2CMP1     = 0xB5;
sfr   RTC2CPT00    = 0xB6;
sfr   IEN1         = 0xB8;
sfr   IP1          = 0xB9;
sfr   IRCON        = 0xC0;
sfr   CCEN         = 0xC1;
sfr   CCL1         = 0xC2;
sfr   CCH1         = 0xC3;
sfr   CCL2         = 0xC4;
sfr   CCH2         = 0xC5;
sfr   CCL3         = 0xC6;
sfr   CCH3         = 0xC7;
sfr   T2CON        = 0xC8;
sfr   MPAGE        = 0xC9;
sfr   CRCL         = 0xCA;
sfr   CRCH         = 0xCB;
sfr   TL2          = 0xCC;
sfr   TH2          = 0xCD;
sfr   WUOPC1       = 0xCE;
sfr   WUOPC0       = 0xCF;
sfr   PSW          = 0xD0;
sfr   ADCCON3      = 0xD1;
// sfr   ADCCON2      = 0xD2; // XXX: Undocumented in nRF31512
sfr   ADCCON1      = 0xD3;
sfr   ADCDATH      = 0xD4;
sfr   ADCDATL      = 0xD5;
sfr   POFCON       = 0xDC;
sfr   CCPDATIA     = 0xDD;
sfr   CCPDATIB     = 0xDE;
sfr   CCPDATO      = 0xDF;
sfr   ACC          = 0xE0;
sfr   RTC2PCO      = 0xE3;
sfr   SPIRCON0     = 0xE4;
sfr   SPIRCON1     = 0xE5;
sfr   SPIRSTAT     = 0xE6;
sfr   SPIRDAT      = 0xE7;
sfr   RFCON        = 0xE8;
sfr   B            = 0xF0;
sfr   FSR          = 0xF8;
sfr   SPIMCON0     = 0xFC;
sfr   SPIMCON1     = 0xFD;
sfr   SPIMSTAT     = 0xFE;
sfr   SPIMDAT      = 0xFF;

sfr16 CC1          = 0xC2;
sfr16 CC2          = 0xC4;
sfr16 CC3          = 0xC6;
sfr16 CRC          = 0xCA;
sfr16 T2           = 0xCC;

/*  FSR  */
sbit  MCDIS        = FSR^7;
sbit  STP          = FSR^6;
sbit  WEN          = FSR^5;
sbit  RDYN         = FSR^4;
sbit  INFEN        = FSR^3;
sbit  RDIS         = FSR^2;
sbit  RDEND        = FSR^1;
sbit  WPEN         = FSR^0;

/*  PSW   */
sbit  CY           = PSW^7;
sbit  AC           = PSW^6;
sbit  F0           = PSW^5;
sbit  RS1          = PSW^4;
sbit  RS0          = PSW^3;
sbit  OV           = PSW^2;
sbit  P            = PSW^0;

/*  TCON  */
sbit  TF1          = TCON^7;
sbit  TR1          = TCON^6;
sbit  TF0          = TCON^5;
sbit  TR0          = TCON^4;
sbit  IE1          = TCON^3;
sbit  IT1          = TCON^2;
sbit  IE0          = TCON^1;
sbit  IT0          = TCON^0;

/*  T2CON  */
sbit  T2PS         = T2CON^7;
sbit  I3FR         = T2CON^6;
sbit  I2FR         = T2CON^5;
sbit  T2R1         = T2CON^4;
sbit  T2R0         = T2CON^3;
sbit  T2CM         = T2CON^2;
sbit  T2I1         = T2CON^1;
sbit  T2I0         = T2CON^0;

/*  IEN0  */
sbit  EA           = IEN0^7;
sbit  ET2          = IEN0^5;
sbit  ES0          = IEN0^4;
sbit  ET1          = IEN0^3;
sbit  EX1          = IEN0^2;
sbit  ET0          = IEN0^1;
sbit  EX0          = IEN0^0;

/* IEN1  */
sbit  EXEN2        = IEN1^7;
sbit  WUIRQ        = IEN1^5;
sbit  MISC         = IEN1^4;
sbit  WUPIN        = IEN1^3;
sbit  SPI          = IEN1^2;
sbit  RF           = IEN1^1;
sbit  RFSPI        = IEN1^0;

/* IRCON */
sbit  EXF2         = IRCON^7;
sbit  TF2          = IRCON^6;
sbit  WUF          = IRCON^5;
sbit  MISCF        = IRCON^4;
sbit  WUPINF       = IRCON^3;
sbit  SPIF         = IRCON^2;
sbit  RFF          = IRCON^1;
sbit  RFSPIF       = IRCON^0;

/* PORT0 */
sbit  P00          = P0^0;
sbit  P01          = P0^1;
sbit  P02          = P0^2;
sbit  P03          = P0^3;
sbit  P04          = P0^4;
sbit  P05          = P0^5;
sbit  P06          = P0^6;
sbit  P07          = P0^7;

/* PORT1 */
sbit  P10          = P1^0;
sbit  P11          = P1^1;

/* RFCON */
sbit  RFCE         = RFCON^0;
sbit  RFCSN        = RFCON^1;
sbit  RFCKEN       = RFCON^2;

//-----------------------------------------------------------------------------
// Register bit definitions
//-----------------------------------------------------------------------------
// SPIMSTAT/SPIRSTAT
#define RXREADY   (1 << 2)
#define TXEMPTY   (1 << 1)
#define TXREADY   (1 << 0)
// SPIMCON
#define SPIEN     (1 << 0)
#define SPIPHASE  (1 << 1)
#define SPIPOL    (1 << 2)
#define SPI4M     (1 << 4)
#define SPI2M     (2 << 4)
#define SPI1M     (3 << 4)
// WUCON
#define WAKE_ON_XOSC  (2 << 0)
#define WAKE_ON_TICK  (2 << 4)
#define WAKE_ON_RADIO (2 << 6)
// RTC2CON
#define RTC_CAPTURE         (1 << 4)
#define RTC_COMPARE         (2 << 1)
#define RTC_COMPARE_RESET   (3 << 1)
#define RTC_ENABLE          (1 << 0)
// PWRDWN
#define STANDBY       (7 << 0)
#define RETENTION     (4 << 0)
// PxCON
#define HIGH_DRIVE    (3 << 5)
#define NO_PULL       ((0 << 5) | (1 << 4))
#define PULL_DOWN     ((1 << 5) | (1 << 4))
#define PULL_UP       ((2 << 5) | (1 << 4))
#define INPUT_OFF     ((3 << 5) | (1 << 4))
// PCON
#define PCON_PMW      (1 << 4)
// FSR
#define FSR_WEN       (1 << 5)
// CLKLFCTRL
#define CLKLF_RC      (1 << 0)
#define CLKLF_XOSC    (2 << 0)
// CLKCTRL
#define X16IRQ        (1 << 3)
#define XOSC_ON       (1 << 7)
// OPMCON
#define OPM_LATCH     (1 << 1)
// T2CON
#define T2_24         (1 << 7)
#define T2_RUN        (1 << 0)
#define T2_STOP       (0 << 0)
// ADC
#define ADC_START(chan) ((1 << 7) | (chan << 2))
#define ADC_BUSY        (1 << 6)
#define ADC_8BIT        (1 << 6)
#define ADC_10BIT       (2 << 6)

//-----------------------------------------------------------------------------
// Interrupt Vector Definitions
//-----------------------------------------------------------------------------

#define INTERRUPT_IFP          0   // Interrupt from pin
#define INTERRUPT_T0           1   // Timer0 Overflow
#define INTERRUPT_POFIRQ       2   // Power failure interrupt
#define INTERRUPT_T1           3   // Timer1 Overflow
#define INTERRUPT_T2           5   // Timer2 Overflow / external reload
#define INTERRUPT_RFRDY        8   // RF SPI ready interrupt
#define INTERRUPT_RFIRQ        9   // RF interrupt
#define INTERRUPT_SERIAL       10  // SPI
#define INTERRUPT_WUOPIRQ      11  // Wakeup on pin interrupt
#define INTERRUPT_MISCIRQ      12  // Misc interrupt
#define INTERRUPT_TICK         13  // Internal Wakeup

#define EXT_INT0_ISR()  void ext_int0_isr(void) interrupt INTERRUPT_IPF     // External Interrupt0 (P0.3) (0x03)
#define T0_ISR()        void t0_isr(void)       interrupt INTERRUPT_T0      // Timer0 Overflow (0x0b)
#define POFIRQ_ISR()    void pofirq_isr(void)   interrupt INTERRUPT_POFIRQ  // Power failure interrupt (0x13)
#define T1_ISR()        void t1_isr(void)       interrupt INTERRUPT_T1      // Timer1 Overflow (0x1b)
#define T2_ISR()        void t2_isr(void)       interrupt INTERRUPT_T2      // Timer2 Overflow (0x2b)
#define RF_RDY_ISR()    void rf_rdy_isr(void)   interrupt INTERRUPT_RFRDY   // RF SPI ready interrupt (0x43)
#define NRF_ISR()       void nrf_isr(void)      interrupt INTERRUPT_RFIRQ   // RF interrupt (0x4b)
#define SER_ISR()       void serial_isr(void)   interrupt INTERRUPT_SERIAL  // SERIAL / SPI interrupt (0x53)
#define WUOP_ISR()      void wuop_isr(void)     interrupt INTERRUPT_WUOPIRQ // Wake on pin interrupt (0x5b)
#define MISC_ISR()      void misc_isr(void)     interrupt INTERRUPT_MISCIRQ // MISC interrupt (0x63)
#define ADC_ISR()       void adc_isr(void)      interrupt INTERRUPT_MISCIRQ // ADC interrupt (0x63)
#define TICK_ISR()      void tick_isr(void)     interrupt INTERRUPT_TICK    // Internal wakeup interrupt (0x6b)

#define RF_START_US     130

#endif
