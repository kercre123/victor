
#ifndef DRV_UART_SYN_DEF_H
#define DRV_UART_SYN_DEF_H 

// 1: Defines
// ----------------------------------------------------------------------------

// IER 0x04 - int enable register 
#define D_UART_IER_PTIME   (1<<7)   // Programmable THRE Interrupt Mode Enable
#define D_UART_IER_EDSSI  (1<<3)    //Enable Modem Status Interrupt
#define D_UART_IER_ELSI   (1<<2)   //Enable Receiver Line Status Interrupt
#define D_UART_IER_ETBEI   (1<<1)  //Enable Transmit Holding Register Empty Interrupt
#define D_UART_IER_ERBFI    (1<<0) // Enable Received Data Available Interrupt

// IRR 0x08  Interrupt Identity Register - Read Only
#define D_UART_IIR_FIFOSE              (1<<6)    // 2 bits,  FIFOs Enabled
#define D_UART_IIR_IID_MODEM_STAT      (0<<0)    // 4 bits   Interrupt ID.
#define D_UART_IIR_IID_NO_INT          (1<<0)    // 
#define D_UART_IIR_IID_THR_EMPTY       (2<<0)    //
#define D_UART_IIR_IID_RX_DATA         (4<<0)    // 
#define D_UART_IIR_IID_RX_LINE_ST      (6<<0)    // 
#define D_UART_IIR_IID_BUSY            (7<<0)    // 
#define D_UART_IIR_IID_CH_TIMEOUT      (1<<0)    //



// FCR 0x08 FIFO Control Register - Write Only
#define D_UART_FCR_RCVR_1CH          (0<<6)// 2 bits, RCVR Trigger
#define D_UART_FCR_RCVR_QUARTER      (1<<6)// 2 bits, RCVR Trigger
#define D_UART_FCR_RCVR_HALF         (2<<6)// 2 bits, RCVR Trigger
#define D_UART_FCR_RCVR_2LESS        (3<<6)// 2 bits, RCVR Trigger
#define D_UART_FCR_TET_EMPTY         (0<<4)// 2 bits, TX Empty Trigger
#define D_UART_FCR_TET_2CH           (1<<4)// 2 bits, TX Empty Trigger
#define D_UART_FCR_TET_QUARTER       (2<<4)// 2 bits, TX Empty Trigger
#define D_UART_FCR_TET_HALF          (3<<4)// 2 bits, TX Empty Trigger
#define D_UART_FCR_DMAM              (1<<3)// DMA Mode
#define D_UART_FCR_XFIFOR            (1<<2)// XMIT FIFO Reset
#define D_UART_FCR_RFIFOR            (1<<1)// RCVR FIFO Reset
#define D_UART_FCR_FIFOE             (1<<0) // FIFO Enable

// LCR 0x0C Line Control Register
#define D_UART_LCR_DLAB    (1<<7) //Divisor Latch Access Bit. Gives access to DLL and DLH/LPDLL and LPDLH
#define D_UART_LCR_BC      (1<<6) //Break Control Bit
#define D_UART_LCR_SP      (1<<5) //Stick Parity
#define D_UART_LCR_EPS     (1<<4) //Even Parity Select
#define D_UART_LCR_PEN     (1<<3) // Parity Enable
#define D_UART_LCR_STOP1   (0<<2) // Number of stop bits
#define D_UART_LCR_STOP2   (1<<2) // Number of stop bits 1.5 if dls is 0, otherwise is 2
#define D_UART_LCR_DLS5    (0<<0) // 2 bits, Data Length Select
#define D_UART_LCR_DLS6    (1<<0) // 2 bits, Data Length Select
#define D_UART_LCR_DLS7    (2<<0) // 2 bits, Data Length Select
#define D_UART_LCR_DLS8    (3<<0) // 2 bits, Data Length Select

// MCR 0x10 Modem Control Register
#define D_UART_MCR_SIRE (1<<6) // SIR Mode Enable
#define D_UART_MCR_AFCE (1<<5) // Auto Flow Control Enable
#define D_UART_MCR_LB   (1<<4) // LoopBack Bit
#define D_UART_MCR_OUT2 (1<<3)
#define D_UART_MCR_OUT1 (1<<2)
#define D_UART_MCR_RTS  (1<<1) // Request to Send
#define D_UART_MCR_DTR  (1<<0) // Data Terminal Ready

// LSR 0x14 Line Status Register 
#define D_UART_LSR_RFE   (1<<7)  //Receiver FIFO Error bit.
#define D_UART_LSR_TEMT  (1<<6)  //Transmitter Empty bit
#define D_UART_LSR_THRE  (1<<5)  //Transmit Holding Register Empty bit
#define D_UART_LSR_BI    (1<<4)  //Break Interrupt bit
#define D_UART_LSR_FE    (1<<3)  //Framing Error bit
#define D_UART_LSR_PE    (1<<2)  // Parity Error bit
#define D_UART_LSR_OE    (1<<1)  // Overrun error bit.
#define D_UART_LSR_DR    (1<<0)  // Data Ready bit.

// MSR  0x18 - Modem Status Register
#define D_UART_MSR_DCD    (1<<7)   // Data Carrier Detect
#define D_UART_MSR_RI     (1<<6)   // Ring Indicator
#define D_UART_MSR_DSR    (1<<5)   //Data Set Ready
#define D_UART_MSR_CTS    (1<<4)   // Clear to Send
#define D_UART_MSR_DDCD   (1<<3)   //Delta Data Carrier Detect
#define D_UART_MSR_TERI   (1<<2)   // Trailing Edge of Ring Indicator
#define D_UART_MSR_DDSR   (1<<1)   // Delta Data Set Ready 
#define D_UART_MSR_DCTS   (1<<0)   // Delta Clear to Send

// FAR 0x70 FIFO Access Register
#define D_UART_FAR (1<<0)   // FIFO Access Register

// RFW 0x78 Receive FIFO Write
#define D_UART_RFW_RFFE (1<<9)   // Receive FIFO Framing Error
#define D_UART_RFW_RFPE  (1<<8)  // Receive FIFO Parity Error
#define D_UART_RFW_RFWD  (1<<0)  // Receive FIFO Write Data

// USR 0x7C  UART Status Register , read only 
#define D_UART_USR_RFF   (1<<4) // Receive FIFO Full
#define D_UART_USR_RFNE  (1<<3) // Receive FIFO Not Empty
#define D_UART_USR_TFE   (1<<2) // Transmit FIFO Empty
#define D_UART_USR_TFNF  (1<<1) // Transmit FIFO Not Full
#define D_UART_USR_BUSY  (1<<0) // UART Busy

// SRR 0x88 Software Reset Register, write only
#define D_UART_SRR_XFR (1<<2) // XMIT FIFO Reset
#define D_UART_SRR_RFR (1<<1) // RCVR FIFO Reset
#define D_UART_SRR_UR  (1<<0) // UART Reset

// SRTS 0x8C Shadow Request to Send
#define D_UART_SRTS (1<<0) // Shadow Request to Send

// SBCR 0x90  Shadow Break Control Register
#define D_UART_SBCR (1<<0)  //Shadow Break Control Bit

// SDMAM  0x94 Shadow DMA Mode
#define D_UART_SDMAM (1<<0)  // Shadow DMA Mode

// SFE 0x 98 Shadow FIFO Enable
#define D_UART_SFE (1<<0) //  Shadow FIFO Enable

// SRT 0x9C  Shadow RCVR Trigger
#define D_UART_SRT  (1<<0) // 2 bits 

// STET 0xA0  Shadow TX Empty Trigger
#define D_UART_STET (1<<0) // 2 bits 

// HTX 0xA4 Halt TX
#define D_UART_HTX (1<<0)

// DMASA  0xA8 DMA Software Acknowledge
#define D_UART_DMASA (1<<0)

// CPR 0xf4 Component Parameter Register, read-only
#define D_UART_CPR_FIFO_MODE          (1<<16)   // 8 bits. fifo size
#define D_UART_CPR_DMA_EXTRA          (1<<13) 
#define D_UART_CPR_UART_ADD_ENCODED_PARAMS  (1<<12)
#define D_UART_CPR_SHADOW             (1<<11)
#define D_UART_CPR_FIFO_STAT          (1<<10)
#define D_UART_CPR_FIFO_ACCESS        (1<<9)
#define D_UART_CPR_NEW_FEAT           (1<<8)
#define D_UART_CPR_SIR_LP_MODE        (1<<7)
#define D_UART_CPR_SIR_MODE           (1<<6)  
#define D_UART_CPR_THRE_MODE          (1<<5)
#define D_UART_CPR_AFCE_MODE          (1<<4)
#define D_UART_CPR_APB_DATA_WIDTH     (1<<0) // 2 bits 



#endif //DRV_UART_SYN_H

