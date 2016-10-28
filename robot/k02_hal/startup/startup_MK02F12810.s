; * ---------------------------------------------------------------------------------------
; *  @file:    startup_MK02F12810.s
; *  @purpose: CMSIS Cortex-M4 Core Device Startup File
; *            MK02F12810
; *  @version: 0.4
; *  @date:    2014-10-14
; *  @build:   b141016
; * ---------------------------------------------------------------------------------------
; *
; * Copyright (c) 1997 - 2014 , Freescale Semiconductor, Inc.
; * All rights reserved.
; *
; * Redistribution and use in source and binary forms, with or without modification,
; * are permitted provided that the following conditions are met:
; *
; * o Redistributions of source code must retain the above copyright notice, this list
; *   of conditions and the following disclaimer.
; *
; * o Redistributions in binary form must reproduce the above copyright notice, this
; *   list of conditions and the following disclaimer in the documentation and/or
; *   other materials provided with the distribution.
; *
; * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
; *   contributors may be used to endorse or promote products derived from this
; *   software without specific prior written permission.
; *
; * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
; * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
; * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
; * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
; *
; *------- <<< Use Configuration Wizard in Context Menu >>> ------------------
; *
; *****************************************************************************/


                PRESERVE8
                THUMB


                AREA    HEADER, DATA, READONLY
__Signature     DCD     Reset_Handler
                DCB     'C','Z','M','0'
                DCD     __Vectors
                DCD     0xDEADFACE
                DCD     0xDEADFACE
                DCD     0xDEADFACE
                DCD     0x00000000

; Vector Table Mapped, aligned to a a 1kbyte boundary

                AREA    VECTORS, DATA, READWRITE, ALIGN=8
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp   ; Top of Stack
                DCD     Reset_Handler  ; Reset Handler
                DCD     NMI_Handler                         ;NMI Handler
                DCD     HardFault_PreHandler                ;Hard Fault Handler
                DCD     MemManage_Handler                   ;MPU Fault Handler
                DCD     BusFault_Handler                    ;Bus Fault Handler
                DCD     UsageFault_Handler                  ;Usage Fault Handler
                DCD     0                                   ;Reserved
                DCD     0                                   ;Reserved
                DCD     0                                   ;Reserved
                DCD     0                                   ;Reserved
                DCD     SVC_Handler                         ;SVCall Handler
                DCD     DebugMon_Handler                    ;Debug Monitor Handler
                DCD     0                                   ;Reserved
                DCD     PendSV_Handler                      ;PendSV Handler
                DCD     SysTick_Handler                     ;SysTick Handler

                                                            ;External Interrupts
                DCD     DMA0_IRQHandler                     ;DMA Channel 0 Transfer Complete
                DCD     DMA1_IRQHandler                     ;DMA Channel 1 Transfer Complete
                DCD     DMA2_IRQHandler                     ;DMA Channel 2 Transfer Complete
                DCD     DMA3_IRQHandler                     ;DMA Channel 3 Transfer Complete
                DCD     Reserved20_IRQHandler               ;Reserved interrupt 20
                DCD     Reserved21_IRQHandler               ;Reserved interrupt 21
                DCD     Reserved22_IRQHandler               ;Reserved interrupt 22
                DCD     Reserved23_IRQHandler               ;Reserved interrupt 23
                DCD     Reserved24_IRQHandler               ;Reserved interrupt 24
                DCD     Reserved25_IRQHandler               ;Reserved interrupt 25
                DCD     Reserved26_IRQHandler               ;Reserved interrupt 26
                DCD     Reserved27_IRQHandler               ;Reserved interrupt 27
                DCD     Reserved28_IRQHandler               ;Reserved interrupt 28
                DCD     Reserved29_IRQHandler               ;Reserved interrupt 29
                DCD     Reserved30_IRQHandler               ;Reserved interrupt 30
                DCD     Reserved31_IRQHandler               ;Reserved interrupt 31
                DCD     DMA_Error_IRQHandler                ;DMA Error Interrupt
                DCD     MCM_IRQHandler                      ;Normal Interrupt
                DCD     FTF_IRQHandler                      ;FTFA Command complete interrupt
                DCD     Read_Collision_IRQHandler           ;Read Collision Interrupt
                DCD     LVD_LVW_IRQHandler                  ;Low Voltage Detect, Low Voltage Warning
                DCD     LLW_IRQHandler                      ;Low Leakage Wakeup
                DCD     WDOG_EWM_IRQHandler                 ;WDOG Interrupt
                DCD     Reserved39_IRQHandler               ;Reserved Interrupt 39
                DCD     I2C0_IRQHandler                     ;I2C0 interrupt
                DCD     Reserved41_IRQHandler               ;Reserved Interrupt 41
                DCD     SPI0_IRQHandler                     ;SPI0 Interrupt
                DCD     Reserved43_IRQHandler               ;Reserved Interrupt 43
                DCD     Reserved44_IRQHandler               ;Reserved Interrupt 44
                DCD     Reserved45_IRQHandler               ;Reserved interrupt 45
                DCD     Reserved46_IRQHandler               ;Reserved interrupt 46
                DCD     UART0_RX_TX_IRQHandler              ;UART0 Receive/Transmit interrupt
                DCD     UART0_ERR_IRQHandler                ;UART0 Error interrupt
                DCD     UART1_RX_TX_IRQHandler              ;UART1 Receive/Transmit interrupt
                DCD     UART1_ERR_IRQHandler                ;UART1 Error interrupt
                DCD     Reserved51_IRQHandler               ;Reserved interrupt 51
                DCD     Reserved52_IRQHandler               ;Reserved interrupt 52
                DCD     Reserved53_IRQHandler               ;Reserved interrupt 53
                DCD     Reserved54_IRQHandler               ;Reserved interrupt 54
                DCD     ADC0_IRQHandler                     ;ADC0 interrupt
                DCD     CMP0_IRQHandler                     ;CMP0 interrupt
                DCD     CMP1_IRQHandler                     ;CMP1 interrupt
                DCD     FTM0_IRQHandler                     ;FTM0 fault, overflow and channels interrupt
                DCD     FTM1_IRQHandler                     ;FTM1 fault, overflow and channels interrupt
                DCD     FTM2_IRQHandler                     ;FTM2 fault, overflow and channels interrupt
                DCD     Reserved61_IRQHandler               ;Reserved interrupt 61
                DCD     Reserved62_IRQHandler               ;Reserved interrupt 62
                DCD     Reserved63_IRQHandler               ;Reserved interrupt 63
                ; DCD     PIT0_IRQHandler                     ;PIT timer channel 0 interrupt
                ; DCD     PIT1_IRQHandler                     ;PIT timer channel 1 interrupt
                ; DCD     PIT2_IRQHandler                     ;PIT timer channel 2 interrupt
                ; DCD     PIT3_IRQHandler                     ;PIT timer channel 3 interrupt
                ; DCD     PDB0_IRQHandler                     ;PDB0 Interrupt
                ; DCD     Reserved69_IRQHandler               ;Reserved interrupt 69
                ; DCD     Reserved70_IRQHandler               ;Reserved interrupt 70
                ; DCD     Reserved71_IRQHandler               ;Reserved interrupt 71
                ; DCD     DAC0_IRQHandler                     ;DAC0 interrupt
                ; DCD     MCG_IRQHandler                      ;MCG Interrupt
                ; DCD     LPTMR0_IRQHandler                   ;LPTimer interrupt
                ; DCD     PORTA_IRQHandler                    ;Port A interrupt
                ; DCD     PORTB_IRQHandler                    ;Port B interrupt
                ; DCD     PORTC_IRQHandler                    ;Port C interrupt
                ; DCD     PORTD_IRQHandler                    ;Port D interrupt
                ; DCD     PORTE_IRQHandler                    ;Port E interrupt
                ; DCD     SWI_IRQHandler                      ;Software interrupt
                ; DCD     Reserved81_IRQHandler               ;Reserved interrupt 81
                ; DCD     Reserved82_IRQHandler               ;Reserved interrupt 82
                ; DCD     Reserved83_IRQHandler               ;Reserved interrupt 83
                ; DCD     Reserved84_IRQHandler               ;Reserved interrupt 84
                ; DCD     Reserved85_IRQHandler               ;Reserved interrupt 85
                ; DCD     Reserved86_IRQHandler               ;Reserved interrupt 86
                ; DCD     Reserved87_IRQHandler               ;Reserved interrupt 87
                ; DCD     Reserved88_IRQHandler               ;Reserved interrupt 88
                ; DCD     Reserved89_IRQHandler               ;Reserved interrupt 89
__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors

_NVIC_ICER0     EQU   0xE000E180
_NVIC_ICPR0     EQU   0xE000E280

__initial_sp    EQU   0x1FFFE600        ; 1.5KB of stack at BOTTOM of RAM
                EXPORT __initial_sp

                AREA    |.text|, CODE, READONLY

; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                ;IMPORT  SystemInit
                IMPORT  __main

                CPSID   I               ; Mask interrupts
                LDR R0, =_NVIC_ICER0    ; Disable interrupts and clear pending flags
                LDR R1, =_NVIC_ICPR0
                LDR R2, =0xFFFFFFFF
                MOV R3, #8
_irq_clear
                CBZ R3, _irq_clear_end
                STR R2, [R0], #4        ; NVIC_ICERx - clear enable IRQ register
                STR R2, [R1], #4        ; NVIC_ICPRx - clear pending IRQ register
                SUB R3, R3, #1
                B _irq_clear
_irq_clear_end
                CPSIE   i               ; Unmask interrupts
                LDR     R0, =__main
                BX      R0
                ENDP

FORCE_HARDFAULT PROC
                EXPORT FORCE_HARDFAULT

                SUB     sp, sp, #16
                PUSH    {r0-r3}

                STR     r12, [sp, #16]
                STR     lr, [sp, #20]
                LDR     r0, =.
                STR     r0, [sp, #24]
                MRS     r0, PSR
                STR     r0, [sp, #28]

                B       HardFault_PreHandler
                ENDP

HardFault_PreHandler\
                PROC
                IMPORT  HardFault_Handler
                IMPORT  CRASHLOG_POINTER
                B .
                PUSH    {r4-r7}
                MOV     r4, r8
                MOV     r5, r9
                MOV     r6, r10
                MOV     r7, r11
                PUSH    {r4-r7}
                
                ; Store crashlog to global pointer
                LDR     r4, =CRASHLOG_POINTER
                MOV     r5, sp
                STR     r5, [r4, #0]
                B       HardFault_Handler
                ENDP

; Dummy Exception Handlers (infinite loops which can be modified)
NMI_Handler\
                PROC
                EXPORT  NMI_Handler         [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler         [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler         [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler         [WEAK]
                B       .
                ENDP
SVC_Handler\
                PROC
                EXPORT  SVC_Handler         [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler         [WEAK]
                B       .
                ENDP
PendSV_Handler\
                PROC
                EXPORT  PendSV_Handler         [WEAK]
                B       .
                ENDP
SysTick_Handler\
                PROC
                EXPORT  SysTick_Handler         [WEAK]
                B       .
                ENDP
Default_Handler\
                PROC
                EXPORT  DMA0_IRQHandler         [WEAK]
                EXPORT  DMA1_IRQHandler         [WEAK]
                EXPORT  DMA2_IRQHandler         [WEAK]
                EXPORT  DMA3_IRQHandler         [WEAK]
                EXPORT  Reserved20_IRQHandler         [WEAK]
                EXPORT  Reserved21_IRQHandler         [WEAK]
                EXPORT  Reserved22_IRQHandler         [WEAK]
                EXPORT  Reserved23_IRQHandler         [WEAK]
                EXPORT  Reserved24_IRQHandler         [WEAK]
                EXPORT  Reserved25_IRQHandler         [WEAK]
                EXPORT  Reserved26_IRQHandler         [WEAK]
                EXPORT  Reserved27_IRQHandler         [WEAK]
                EXPORT  Reserved28_IRQHandler         [WEAK]
                EXPORT  Reserved29_IRQHandler         [WEAK]
                EXPORT  Reserved30_IRQHandler         [WEAK]
                EXPORT  Reserved31_IRQHandler         [WEAK]
                EXPORT  DMA_Error_IRQHandler         [WEAK]
                EXPORT  MCM_IRQHandler         [WEAK]
                EXPORT  FTF_IRQHandler         [WEAK]
                EXPORT  Read_Collision_IRQHandler         [WEAK]
                EXPORT  LVD_LVW_IRQHandler         [WEAK]
                EXPORT  LLW_IRQHandler         [WEAK]
                EXPORT  WDOG_EWM_IRQHandler         [WEAK]
                EXPORT  Reserved39_IRQHandler         [WEAK]
                EXPORT  I2C0_IRQHandler         [WEAK]
                EXPORT  Reserved41_IRQHandler         [WEAK]
                EXPORT  SPI0_IRQHandler         [WEAK]
                EXPORT  Reserved43_IRQHandler         [WEAK]
                EXPORT  Reserved44_IRQHandler         [WEAK]
                EXPORT  Reserved45_IRQHandler         [WEAK]
                EXPORT  Reserved46_IRQHandler         [WEAK]
                EXPORT  UART0_RX_TX_IRQHandler         [WEAK]
                EXPORT  UART0_ERR_IRQHandler         [WEAK]
                EXPORT  UART1_RX_TX_IRQHandler         [WEAK]
                EXPORT  UART1_ERR_IRQHandler         [WEAK]
                EXPORT  Reserved51_IRQHandler         [WEAK]
                EXPORT  Reserved52_IRQHandler         [WEAK]
                EXPORT  Reserved53_IRQHandler         [WEAK]
                EXPORT  Reserved54_IRQHandler         [WEAK]
                EXPORT  ADC0_IRQHandler         [WEAK]
                EXPORT  CMP0_IRQHandler         [WEAK]
                EXPORT  CMP1_IRQHandler         [WEAK]
                EXPORT  FTM0_IRQHandler         [WEAK]
                EXPORT  FTM1_IRQHandler         [WEAK]
                EXPORT  FTM2_IRQHandler         [WEAK]
                EXPORT  Reserved61_IRQHandler         [WEAK]
                EXPORT  Reserved62_IRQHandler         [WEAK]
                EXPORT  Reserved63_IRQHandler         [WEAK]
                ;EXPORT  PIT0_IRQHandler         [WEAK]
                ;EXPORT  PIT1_IRQHandler         [WEAK]
                ;EXPORT  PIT2_IRQHandler         [WEAK]
                ;EXPORT  PIT3_IRQHandler         [WEAK]
                ;EXPORT  PDB0_IRQHandler         [WEAK]
                ;EXPORT  Reserved69_IRQHandler         [WEAK]
                ;EXPORT  Reserved70_IRQHandler         [WEAK]
                ;EXPORT  Reserved71_IRQHandler         [WEAK]
                ;EXPORT  DAC0_IRQHandler         [WEAK]
                ;EXPORT  MCG_IRQHandler         [WEAK]
                ;EXPORT  LPTMR0_IRQHandler         [WEAK]
                ;EXPORT  PORTA_IRQHandler         [WEAK]
                ;EXPORT  PORTB_IRQHandler         [WEAK]
                ;EXPORT  PORTC_IRQHandler         [WEAK]
                ;EXPORT  PORTD_IRQHandler         [WEAK]
                ;EXPORT  PORTE_IRQHandler         [WEAK]
                ;EXPORT  SWI_IRQHandler         [WEAK]
                ;EXPORT  Reserved81_IRQHandler         [WEAK]
                ;EXPORT  Reserved82_IRQHandler         [WEAK]
                ;EXPORT  Reserved83_IRQHandler         [WEAK]
                ;EXPORT  Reserved84_IRQHandler         [WEAK]
                ;EXPORT  Reserved85_IRQHandler         [WEAK]
                ;EXPORT  Reserved86_IRQHandler         [WEAK]
                ;EXPORT  Reserved87_IRQHandler         [WEAK]
                ;EXPORT  Reserved88_IRQHandler         [WEAK]
                ;EXPORT  Reserved89_IRQHandler         [WEAK]
                EXPORT  DefaultISR         [WEAK]
DMA0_IRQHandler
DMA1_IRQHandler
DMA2_IRQHandler
DMA3_IRQHandler
Reserved20_IRQHandler
Reserved21_IRQHandler
Reserved22_IRQHandler
Reserved23_IRQHandler
Reserved24_IRQHandler
Reserved25_IRQHandler
Reserved26_IRQHandler
Reserved27_IRQHandler
Reserved28_IRQHandler
Reserved29_IRQHandler
Reserved30_IRQHandler
Reserved31_IRQHandler
DMA_Error_IRQHandler
MCM_IRQHandler
FTF_IRQHandler
Read_Collision_IRQHandler
LVD_LVW_IRQHandler
LLW_IRQHandler
WDOG_EWM_IRQHandler
Reserved39_IRQHandler
I2C0_IRQHandler
Reserved41_IRQHandler
SPI0_IRQHandler
Reserved43_IRQHandler
Reserved44_IRQHandler
Reserved45_IRQHandler
Reserved46_IRQHandler
UART0_RX_TX_IRQHandler
UART0_ERR_IRQHandler
UART1_RX_TX_IRQHandler
UART1_ERR_IRQHandler
Reserved51_IRQHandler
Reserved52_IRQHandler
Reserved53_IRQHandler
Reserved54_IRQHandler
ADC0_IRQHandler
CMP0_IRQHandler
CMP1_IRQHandler
FTM0_IRQHandler
FTM1_IRQHandler
FTM2_IRQHandler
Reserved61_IRQHandler
Reserved62_IRQHandler
Reserved63_IRQHandler
;PIT0_IRQHandler
;PIT1_IRQHandler
;PIT2_IRQHandler
;PIT3_IRQHandler
;PDB0_IRQHandler
;Reserved69_IRQHandler
;Reserved70_IRQHandler
;Reserved71_IRQHandler
;DAC0_IRQHandler
;MCG_IRQHandler
;LPTMR0_IRQHandler
;PORTA_IRQHandler
;PORTB_IRQHandler
;PORTC_IRQHandler
;PORTD_IRQHandler
;PORTE_IRQHandler
;SWI_IRQHandler
;Reserved81_IRQHandler
;Reserved82_IRQHandler
;Reserved83_IRQHandler
;Reserved84_IRQHandler
;Reserved85_IRQHandler
;Reserved86_IRQHandler
;Reserved87_IRQHandler
;Reserved88_IRQHandler
;Reserved89_IRQHandler
DefaultISR
                B      DefaultISR
                ENDP
                  ALIGN


                END
