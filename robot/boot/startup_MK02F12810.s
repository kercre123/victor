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


                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   0x1700
__initial_sp

; Vector Table Mapped to Address 0 at Reset
                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size
                EXPORT __initial_sp

__Vectors       DCD     __initial_sp                        ; Top of Stack
                DCD     Reset_Handler                       ; Reset Handler
                DCD     NMI_Handler                         ;NMI Handler
                DCD     HardFault_Handler                   ;Hard Fault Handler
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

                ; NOTE: We deleted a bunch of vectors because IRQs are disabled during boot
__Vectors_End
__Vectors_Size 	EQU     __Vectors_End - __Vectors

                EXPORT HW_VERSION
HW_VERSION      DCD   0x01050000    ; THIS MUST BE SET TO 0xFFFFFFFF FOR 1.0 HEADS

_NVIC_ICER0     EQU   0xE000E180
_NVIC_ICPR0     EQU   0xE000E280

                AREA    |.text|, CODE, READONLY

; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  SystemInit
                IMPORT  __main
                IMPORT  FlashConfig

                IF      :LNOT::DEF:RAM_TARGET
                LDR R0, =FlashConfig    ; dummy read, workaround for flashConfig
                ENDIF

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
                LDR     R0, =SystemInit
                BLX     R0
                CPSIE   i               ; Unmask interrupts
                LDR     R0, =__main
                BX      R0
                ENDP

; Dummy Exception Handlers (infinite loops which can be modified)
NMI_Handler\
                PROC
                EXPORT  NMI_Handler         [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
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
                IMPORT   OTA_EntryPoint

                CPSID   I                       ; Mask interrupts
                MOV     R0, #0
                LDR     SP, [R0]        ; Setup our initial SP
                
                LDR     R0, =OTA_EntryPoint
                BX      R0
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

DefaultISR\
                PROC
                B      DefaultISR
                ENDP
                ALIGN


                END
