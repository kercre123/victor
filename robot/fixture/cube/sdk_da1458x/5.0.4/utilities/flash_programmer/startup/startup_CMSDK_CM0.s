;/*
; *-----------------------------------------------------------------------------
; * The confidential and proprietary information contained in this file may
; * only be used by a person authorised under and to the extent permitted
; * by a subsisting licensing agreement from ARM Limited.
; *
; *            (C) COPYRIGHT 2010-2011 ARM Limited.
; *                ALL RIGHTS RESERVED
; *
; * This entire notice must be reproduced on all copies of this file
; * and copies of this file may only be made by a person if such person is
; * permitted to do so under the terms of a subsisting license agreement
; * from ARM Limited.
; *
; *      SVN Information
; *
; *      Checked In          : $Date: 2011-03-17 11:25:01 +0000 (Thu, 17 Mar 2011) $
; *
; *      Revision            : $Revision: 164919 $
; *
; *      Release Information : BP200-r0p0-00rel0
; *-----------------------------------------------------------------------------
; */

;/*****************************************************************************/
;/* Startup.s: Startup file for ARM Cortex-M0 Device Family                   */
;/*****************************************************************************/


;// <h> Stack Configuration
;//   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
;// </h> 
Stack_Size      EQU     0x100
 

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


; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
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
                DCD     WUPCT_Handler	
                DCD     BLE_RADIOCNTL_Handler	
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
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
                B       .
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
                EXPORT WUPCT_Handler           [WEAK]
                EXPORT BLE_RADIOCNTL_Handler   [WEAK]
				EXPORT BLE_CRYPT_Handler	   [WEAK]
                EXPORT UART_Handler		       [WEAK]
                EXPORT UART2_Handler           [WEAK]
                EXPORT I2C_Handler             [WEAK]
                EXPORT SPI_Handler             [WEAK]
                EXPORT ADC_Handler             [WEAK]
                EXPORT KEYBRD_Handler           [WEAK]
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
WUPCT_Handler	
BLE_RADIOCNTL_Handler	
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
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR

                ALIGN

                ENDIF


                END

