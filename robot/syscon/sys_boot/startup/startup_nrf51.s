; Copyright (c) 2015, Nordic Semiconductor ASA
; All rights reserved.
; 
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 
; * Redistributions of source code must retain the above copyright notice, this
;   list of conditions and the following disclaimer.
; 
; * Redistributions in binary form must reproduce the above copyright notice,
;   this list of conditions and the following disclaimer in the documentation
;   and/or other materials provided with the distribution.
; 
; * Neither the name of Nordic Semiconductor ASA nor the names of its
;   contributors may be used to endorse or promote products derived from
;   this software without specific prior written permission.
; 
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
; CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
; NOTE: Template files (including this one) are application specific and therefore 
; expected to be copied into the application project folder prior to its use!

; Description message

                IF :DEF: __STACK_SIZE
Stack_Size      EQU     __STACK_SIZE
                ELSE
Stack_Size      EQU     2048
                ENDIF

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


                PRESERVE8
                THUMB

; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size
                  
                IMPORT  SVC_Handler

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler
                DCD     NMI_Handler
                DCD     HardFault_Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     PendSV_Handler
                DCD     SysTick_Handler

                ; External Interrupts
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     0                         ; Reserved
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     Trampoline
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved

__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

; Reset Handler

NRF_POWER_RAMON_ADDRESS              EQU   0x40000524  ; NRF_POWER->RAMON address
NRF_POWER_RAMONB_ADDRESS             EQU   0x40000554  ; NRF_POWER->RAMONB address
NRF_POWER_RAMONx_RAMxON_ONMODE_Msk   EQU   0x3         ; All RAM blocks on in onmode bit mask

                EXPORT  Reset_Handler
                IMPORT  SystemInit
                IMPORT  __main

Reset_Handler   PROC
                
                CPSID   I
                MOVS    R1, #NRF_POWER_RAMONx_RAMxON_ONMODE_Msk
                
                LDR     R0, =NRF_POWER_RAMON_ADDRESS
                LDR     R2, [R0]
                ORRS    R2, R2, R1
                STR     R2, [R0]
                
                LDR     R0, =NRF_POWER_RAMONB_ADDRESS
                LDR     R2, [R0]
                ORRS    R2, R2, R1
                STR     R2, [R0]
                
                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP

; Dummy Exception Handlers (infinite loops which can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
                B       .
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler            [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler           [WEAK]
                B       .
                ENDP

Trampoline      PROC
                LDR  R1, =(0x1028)  ; Vector table offset
                MRS  R0, IPSR       ; Get exception number
                LSLS R0, #26        ; Clear all bits save bottom 6
                LSRS R0, #24        ; Multiply by 4
                LDR  R0, [R1,R0]    ; Load vector address (base+exception*4)
                BX   R0             ; Jump to it
                ENDP

                ALIGN


; User Initial Stack & Heap

                IF      :DEF:__MICROLIB
                
                EXPORT  __initial_sp

                ELSE

                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap

                LDR     R0, = Heap_Mem
                LDR     R1, = (Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem + Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR

                ALIGN

                ENDIF

                END
