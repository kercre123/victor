                AREA    |.text|, CODE, READONLY

; Reset handler routine
start_mic_spi   PROC
                EXPORT  start_mic_spi

                ; R0 = TIM_CR1_CEN
                ; R1 = MIC_SPI_CR1
                ; R2 = TIM15->CR1

                PUSH      {r3-r4}
                LDR       r3, =0x40013000   ; SPI1->CR1
                LDR       r4, =0x40003800   ; SPI2->CR1

                CPSID   I
                STRH r0, [r2, #0]
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                STRH r1, [r3, #0]
                STRH r1, [r4, #0]
                CPSIE   I

                POP       {r3-r4}
                BX lr

                ALIGN

                ENDP
                END
