;/**************************************************************************//**
; * @file     startup_ARMCM0.s
; * @brief    CMSIS Core Device Startup File for
; *           ARMCM0 Device Series
; * @version  V1.08
; * @date     23. November 2012
; *
; * @note
; *
; ******************************************************************************/
;/* Copyright (c) 2011 - 2012 ARM LIMITED
;
;   All rights reserved.
;   Redistribution and use in source and binary forms, with or without
;   modification, are permitted provided that the following conditions are met:
;   - Redistributions of source code must retain the above copyright
;     notice, this list of conditions and the following disclaimer.
;   - Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimer in the
;     documentation and/or other materials provided with the distribution.
;   - Neither the name of ARM nor the names of its contributors may be used
;     to endorse or promote products derived from this software without
;     specific prior written permission.
;   *
;   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
;   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
;   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
;   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
;   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
;   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
;   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
;   POSSIBILITY OF SUCH DAMAGE.
;   ---------------------------------------------------------------------------*/
;/*
;//-------- <<< Use Configuration Wizard in Context Menu >>> ------------------
;*/

;// <h> Stack Configuration
;//   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
;// </h>

Stack_Size      EQU     0x00000600

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


;// <h> Heap Configuration
;//   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
;// </h>

Heap_Size       EQU     0x00000100

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit


                PRESERVE8
                THUMB

;remap  uncomment below expression to have the application remap SYSRAM to 0
;__REMAP_SYSRAM EQU 1

; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size
                ;ENTRY
__Vectors       DCD     __initial_sp              ; Top of Stack
                IF      :DEF:__REMAP_SYSRAM
                DCD     Reset_Handler+0x20000000             ; Reset Handler
                ELSE
                DCD     Reset_Handler             ; Reset Handler
                ENDIF
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; PendSV Handler
                DCD     SysTick_Handler           ; SysTick Handler

                DCD     BLE_WAKEUP_LP_Handler
                DCD     BLE_FINETGTIM_Handler
                DCD     BLE_GROSSTGTIM_Handler
                DCD     BLE_CSCNT_Handler
                DCD     BLE_SLP_Handler
                DCD     BLE_ERROR_Handler
                DCD     BLE_RX_Handler
                DCD     BLE_EVENT_Handler
                DCD     SWTIM_Handler
                DCD     WKUP_QUADEC_Handler
                DCD     BLE_RF_DIAG_Handler
                DCD     BLE_CRYPT_Handler
                DCD     UART_Handler
                DCD     UART2_Handler    
                DCD     I2C_Handler    
                DCD     SPI_Handler    
                DCD     ADC_Handler    
                DCD     KEYBRD_Handler    
                DCD     RFCAL_Handler    
                DCD     GPIO0_Handler
                DCD     GPIO1_Handler
                DCD     GPIO2_Handler
                DCD     GPIO3_Handler
                DCD     GPIO4_Handler
__Vectors_End

__Vectors_Size         EQU     __Vectors_End - __Vectors
                AREA    |.text|, CODE, READONLY


; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  __main
                IMPORT  SystemInit

;remap 
                IF      :DEF:__REMAP_SYSRAM
                LDR     R0, =0x0
                LDR     R1, [R0]
                LDR     R0, =0x20000000
                LDR     R2, [R0]
                CMP     R2, R1
                BEQ     remap_done
                LDR     R0, =0x50000012
                LDRH    R1, [R0]
                LSRS    R2, R1, #2
                LSLS    R1, R2, #2
                MOVS    R2, #0x2
                ADDS    R1, R1, R2            ;remap SYSRAM to 0
                LSLS    R2, R2, #14
                ADDS    R1, R1, R2         ;SW RESET
                STRH    R1, [R0]
remap_done                
                ENDIF
;remap

                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP


; Dummy Exception Handlers (infinite loops which can be modified)
                IMPORT NMI_HandlerC
NMI_Handler\
                PROC
                movs r0, #4
                mov r1, lr
                tst r0, r1
                beq NMI_stacking_used_MSP
                mrs r0, psp
                ldr r1,=NMI_HandlerC
                bx r1
NMI_stacking_used_MSP
                mrs r0, msp
                ldr r1,=NMI_HandlerC
                bx r1
                ENDP
                
                IMPORT HardFault_HandlerC
HardFault_Handler\
                PROC
                movs r0, #4
                mov r1, lr
                tst r0, r1
                beq HardFault_stacking_used_MSP
                mrs r0, psp
                ldr r1,=HardFault_HandlerC
                bx r1
HardFault_stacking_used_MSP
                mrs r0, msp
                ldr r1,=HardFault_HandlerC
                bx r1
                ENDP

				
				IMPORT SVC_Handler_c
SVC_Handler     PROC
                movs r0, #4
                mov r1, lr
                tst r0, r1
                beq SVC_stacking_used_MSP
                mrs r0, psp
                ldr r1,=SVC_Handler_c
                bx r1
SVC_stacking_used_MSP
                mrs r0, msp
                ldr r1,=SVC_Handler_c
                bx r1
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler            [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler            [WEAK]
                B       .
                ENDP
Default_Handler PROC
                EXPORT BLE_WAKEUP_LP_Handler   [WEAK]
                EXPORT BLE_FINETGTIM_Handler   [WEAK]
                EXPORT BLE_GROSSTGTIM_Handler  [WEAK]
                EXPORT BLE_CSCNT_Handler       [WEAK]
                EXPORT BLE_SLP_Handler         [WEAK]
                EXPORT BLE_ERROR_Handler       [WEAK]
                EXPORT BLE_RX_Handler          [WEAK]
                EXPORT BLE_EVENT_Handler	   [WEAK]
                EXPORT SWTIM_Handler           [WEAK]
                EXPORT WKUP_QUADEC_Handler     [WEAK]
                EXPORT BLE_RF_DIAG_Handler     [WEAK]
				EXPORT BLE_CRYPT_Handler	   [WEAK]
                EXPORT UART_Handler		       [WEAK]
                EXPORT UART2_Handler           [WEAK]
                EXPORT I2C_Handler             [WEAK]
                EXPORT SPI_Handler             [WEAK]
                EXPORT ADC_Handler             [WEAK]
                EXPORT KEYBRD_Handler          [WEAK]
                EXPORT RFCAL_Handler           [WEAK]
                EXPORT GPIO0_Handler           [WEAK]
                EXPORT GPIO1_Handler           [WEAK]
                EXPORT GPIO2_Handler           [WEAK]
                EXPORT GPIO3_Handler           [WEAK]
                EXPORT GPIO4_Handler           [WEAK]
               
				
BLE_WAKEUP_LP_Handler
BLE_FINETGTIM_Handler     
BLE_GROSSTGTIM_Handler    
BLE_CSCNT_Handler          
BLE_SLP_Handler 
BLE_ERROR_Handler 
BLE_RX_Handler
BLE_EVENT_Handler	
SWTIM_Handler
WKUP_QUADEC_Handler 	
BLE_RF_DIAG_Handler 
BLE_CRYPT_Handler	
UART_Handler		
UART2_Handler    
I2C_Handler    
SPI_Handler    
ADC_Handler    
KEYBRD_Handler    
RFCAL_Handler    
GPIO0_Handler
GPIO1_Handler
GPIO2_Handler
GPIO3_Handler
GPIO4_Handler
               B       .
               ENDP

                ALIGN


; User Initial Stack & Heap

                IF      :DEF:__MICROLIB

                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit

                ELSE

                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap

                LDR     R0, =  Heap_Mem
                LDR     R1, = (Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, =  Stack_Mem
                BX      LR

                ALIGN

                ENDIF


                END

