///  
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by ROIC Driver
/// 
#ifndef DRV_ROIC_DEF_H
#define DRV_ROIC_DEF_H 


// #define ROIC_CMD_BASE                   0x2030_0000 
// #define ROIC_CAL_BASE                   0x2030_0080 
// #define ROIC_DATA_BASE                  0x2030_0100 
// #define ROIC_FRAMECFG_ADDR              0x2030_0180 
// #define ROIC_LINECFG_ADDR               0x2030_0184 
// #define ROIC_WORDPERIOD_ADDR            0x2030_0188 
// #define ROIC_DELAYCFG_ADDR              0x2030_018c 
// #define ROIC_ENABLE_ADDR                0x2030_0190 
// #define ROIC_CFGIRQ_ADDR                0x2030_0194 
// #define ROIC_CLRIRQ_ADDR                0x2030_0198 
// #define ROIC_DMATH_ADDR                 0x2030_019c 
// #define ROIC_STATUS_ADDR                0x2030_01a0 
// #define ROIC_DELAY_ADDR                 0x2030_01a4 
// #define ROIC_PREAMBLE_ADDR              0x2030_01a8 
// #define ROIC_TEMPERATURE_ADDR           0x2030_01ac 
// #define ROIC_RF_CTRL_ADDR               0x2030_01d0 


#define GPIO_ROIC_CLK        (52)
#define GPIO_ROIC_CLK_MODE   (3)
#define GPIO_ROIC_CMD        (53) 
#define GPIO_ROIC_CMD_MODE   (3)
#define GPIO_ROIC_CAL        (56) 
#define GPIO_ROIC_CAL_MODE   (3)
#define GPIO_ROIC_D0         (54)
#define GPIO_ROIC_D0_MODE    (3)
#define GPIO_ROIC_D1         (55)
#define GPIO_ROIC_D1_MODE    (3)


//   ROIC_FRAMECFG_ADDR offset 16'h0180
#define DRV_ROIC_FRAME_PERIOD_MASK    (0x3FFFFF)
     


//.   ROIC_LINECFG_ADDR offset 16'h0184
#define DRV_ROIC_LINECFG_NO_LINES          (13)
#define DRV_ROIC_LINECFG_NO_LINES_MASK     (0x3FF<<13)    
#define DRV_ROIC_LINECFG_LINE_PERIOD       (0)
#define DRV_ROIC_LINECFG_LINE_PERIOD_MASK  (0x1FFF<<0)    

//   ROIC_WORDPERIOD_ADDR offset 16'h0188
#define DRV_ROIC_WORDPERIOD_WORDS             (5)    
#define DRV_ROIC_WORDPERIOD_WORDS_MASK        (0x3FF<<5)    
#define DRV_ROIC_WORDPERIOD_WORD_PERIOD       (0)    
#define DRV_ROIC_WORDPERIOD_WORD_PERIOD_MASK  (0x1F<<0)    
        
//   ROIC_DELAYCFG_ADDR   offset 16'h018C
#define DRV_ROIC_DELAYCFG_CAL_START        (5)    
#define DRV_ROIC_DELAYCFG_DELAY_VAL        (0)
#define DRV_ROIC_DELAYCFG_CAL_START_MASK   (0x1FF<<5)    
#define DRV_ROIC_DELAYCFG_DELAY_VAL_MASK   (0x1F<<0)
            
    
//  ROIC_CFGIRQ_ADDR   offset 16'h0194    &&  ROIC_CLRIRQ_ADDR offset 16'h0198
#define DRV_ROIC_IRQ_CAL_FIFO_EMPTY      (1<<0)    // CAL fifo below level aka empty
#define DRV_ROIC_IRQ_DATA_FIFO_FULL      (1<<1)    // DATA fifo above level aka full
#define DRV_ROIC_IRQ_FRAME_START         (1<<2)    // frame started
#define DRV_ROIC_IRQ_LINE_START          (1<<3)    // line started
#define DRV_ROIC_IRQ_CAL_FIFO_UNDER      (1<<4)    // calibration under run
#define DRV_ROIC_IRQ_DATA_FIFO_UNDER     (1<<5)    // data memory underflow
#define DRV_ROIC_IRQ_PREAMBLE2_ERR       (1<<6)    // Preamble2 error - sticky
#define DRV_ROIC_IRQ_PREAMBLE1_ERR       (1<<7)    //  Preamble1 error  - sticky
#define DRV_ROIC_IRQ_EOF                 (1<<8)    // end of frame - sticky

// raw status irq only for ROIC_CFGIRQ_ADDR
#define DRV_ROIC_CFGIRQ_RAW_CAL_FIFO_EMPTY  (1<<16)    // RAW CAL fifo below level aka empty
#define DRV_ROIC_CFGIRQ_RAW_DATA_FIFO_FULL  (1<<17)    // RAW DATA fifo above level aka full
#define DRV_ROIC_CFGIRQ_RAW_FRAME_START     (1<<18)    // RAW frame started
#define DRV_ROIC_CFGIRQ_RAW_LINE_START      (1<<19)    // RAW line started
#define DRV_ROIC_CFGIRQ_RAW_CAL_FIFO_UNDER  (1<<20)    // calibration under run
#define DRV_ROIC_CFGIRQ_RAW_DATA_FIFO_UNDER (1<<21)    // data memory underflow
#define DRV_ROIC_CFGIRQ_RAW_PREAMBLE2_ERR   (1<<22)    // Preamble2 error - sticky
#define DRV_ROIC_CFGIRQ_RAW_PREAMBLE1_ERR   (1<<23)    //  Preamble1 error  - sticky
#define DRV_ROIC_CFGIRQ_RAW_EOF             (1<<24)    // end of frame - sticky



    
// ROIC_DMATH_ADDR  offset 16'h019C
#define DRV_ROIC_DMATH_DATA_TRESHOLD         (5)
#define DRV_ROIC_DMATH_DATA_TRESHOLD_MASK    (0x1F<<5) 
#define DRV_ROIC_DMATH_CAL_TRESHOLD          (0) 
#define DRV_ROIC_DMATH_CAL_TRESHOLD_MASK     (0x1F<<0)

//ROIC_STATUS_ADDR    offset 16'h01a0
#define DRV_ROIC_STAT_DATA_LEVEL         (8)
#define DRV_ROIC_STAT_DATA_LEVEL_MASK    (0x3F<<8)
#define DRV_ROIC_STAT_CAL_LEVEL          (2)
#define DRV_ROIC_STAT_CAL_LEVEL_MASK     (0x3F<<2)
#define DRV_ROIC_STAT_DATA_FIFO_OVER     (1<<1)
#define DRV_ROIC_STAT_BUSY               (1<<0)

  
// ROIC_DELAY_ADDR  offset 16'h01a4
#define DRV_ROIC_DELAY_D1_PH_SHIFT          (0)
#define DRV_ROIC_DELAY_D1_PH_SHIFT_MASK     (0x07<<0)
#define DRV_ROIC_DELAY_D1_INV               (1)
#define DRV_ROIC_DELAY_D1_INV_MASK          (1<<1)
#define DRV_ROIC_DELAY_D2_PH_SHIFT          (4)
#define DRV_ROIC_DELAY_D2_PH_SHIFT_MASK     (0x07<<4)
#define DRV_ROIC_DELAY_D2_INV               (7)
#define DRV_ROIC_DELAY_D2_INV_MASK          (1<<7)

//   ROIC_ENABLE_ADDR    offset h0190
#define DRV_ROIC_ENABLE_EN       (1<<0)          
#define DRV_ROIC_ENABLE_DAT2_EN  (1<<1)                    
            
            
// function specific defines            
#define DRV_ROIC_1_DATA_LINE (1)
#define DRV_ROIC_2_DATA_LINE (2)

            

#endif //DRV_ROIC_DEF_H