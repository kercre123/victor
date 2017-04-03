; Syscon bootloader header for Cozmo

                IF :DEF: __STACK_SIZE
Stack_Size      EQU     __STACK_SIZE
                ELSE
Stack_Size      EQU     2048
                ENDIF

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

                IF :DEF: __HEAP_SIZE
Heap_Size       EQU     __HEAP_SIZE
                ELSE
Heap_Size       EQU     2048
                ENDIF

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
                IMPORT  EnterRecovery

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler
                DCD     NMI_Handler
                DCD     HardFault_Handler
                DCD     5                         ; Syscon version - 0 = pre-Pilot, 1 = Pilot, 3 = "First 1000" Prod, 4 = Full Prod, 5 = 1.5 EP2
                DCD     0                         ; Body ESN
                DCD     0xFFFFFFFF                ; Current Color[0]
                DCD     0xFFFFFFFF                ; Current Color[1]
                DCD     0xFFFFFFFF                ; Current Color[2]
                DCD     0xFFFFFFFF                ; Current Color[3]
                DCD     0                         ; Reserved
                DCD     SVC_Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     PendSV_Handler
                DCD     SysTick_Handler

                DCD     EnterRecovery

__Vectors_End
__Vectors_Size  EQU     __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

; Reset Handler

NRF_POWER_RAMON_ADDRESS              EQU   0x40000524  ; NRF_POWER->RAMON address
NRF_POWER_RAMONB_ADDRESS             EQU   0x40000554  ; NRF_POWER->RAMONB address
NRF_POWER_RAMONx_RAMxON_ONMODE_Msk   EQU   0x3         ; All RAM blocks on in onmode bit mask

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  SystemInit
                IMPORT  __main

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
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
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

Default_Handler PROC

                EXPORT   POWER_CLOCK_IRQHandler [WEAK]
                EXPORT   RADIO_IRQHandler [WEAK]
                EXPORT   UART0_IRQHandler [WEAK]
                EXPORT   SPI0_TWI0_IRQHandler [WEAK]
                EXPORT   SPI1_TWI1_IRQHandler [WEAK]
                EXPORT   GPIOTE_IRQHandler [WEAK]
                EXPORT   ADC_IRQHandler [WEAK]
                EXPORT   TIMER0_IRQHandler [WEAK]
                EXPORT   TIMER1_IRQHandler [WEAK]
                EXPORT   TIMER2_IRQHandler [WEAK]
                EXPORT   RTC0_IRQHandler [WEAK]
                EXPORT   TEMP_IRQHandler [WEAK]
                EXPORT   RNG_IRQHandler [WEAK]
                EXPORT   ECB_IRQHandler [WEAK]
                EXPORT   CCM_AAR_IRQHandler [WEAK]
                EXPORT   WDT_IRQHandler [WEAK]
                EXPORT   RTC1_IRQHandler [WEAK]
                EXPORT   QDEC_IRQHandler [WEAK]
                EXPORT   LPCOMP_IRQHandler [WEAK]
                EXPORT   SWI0_IRQHandler [WEAK]
                EXPORT   SWI1_IRQHandler [WEAK]
                EXPORT   SWI2_IRQHandler [WEAK]
                EXPORT   SWI3_IRQHandler [WEAK]
                EXPORT   SWI4_IRQHandler [WEAK]
                EXPORT   SWI5_IRQHandler [WEAK]
POWER_CLOCK_IRQHandler
RADIO_IRQHandler
UART0_IRQHandler
SPI0_TWI0_IRQHandler
SPI1_TWI1_IRQHandler
GPIOTE_IRQHandler
ADC_IRQHandler
TIMER0_IRQHandler
TIMER1_IRQHandler
TIMER2_IRQHandler
RTC0_IRQHandler
TEMP_IRQHandler
RNG_IRQHandler
ECB_IRQHandler
CCM_AAR_IRQHandler
WDT_IRQHandler
RTC1_IRQHandler
QDEC_IRQHandler
LPCOMP_IRQHandler
SWI0_IRQHandler
SWI1_IRQHandler
SWI2_IRQHandler
SWI3_IRQHandler
SWI4_IRQHandler
SWI5_IRQHandler
                B .
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

                LDR     R0, = Heap_Mem
                LDR     R1, = (Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem + Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR

                ALIGN

                ENDIF

                END
