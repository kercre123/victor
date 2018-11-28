                AREA    |.text|, CODE, READONLY

                IMPORT  DECIMATION_TABLE
                EXPORT  dec_odd
                EXPORT  dec_even
                EXPORT  start_mic_spi

SAMPLES_PER_IRQ equ     20

                MACRO
                set_table   $e, $offset, $index
                LDRH    r3, [r1, #$offset * 2]  ; 0000000000000000?A?B?C?D?E?F?G?H
                IF $e != 0
                LSRS    r3, r3, #1              ; Alternate channel
                ENDIF
                ORRS    r3, r3, r0              ; 0000000000000000?A1B1C1D1E1F1G1H
                LSLS    r4, r3, #7              ; 000000000?A1B1C1D1E1F1G1H0000000
                ANDS    r3, r3, r4              ; 0000000000000000DAEBFCGDH0000000
                ORRS    r3, r3, r6              ; r3 = DECIMATION_TABLE[byte][$index]

                IF      $index > 0
                ADDS    r3, r3, #$index * 4
                ENDIF
                MEND

                MACRO
                stage2
                LDM     r3, {r3-r4}
                ADD     r9, r9, r3          ; acc2 += *ptr++
                ADD     r8, r8, r4          ; acc1 += *ptr++
                MEND
                
                MACRO
                stage3i
                LDM     r3, {r3-r4, r7}     ; acc0 = ptr[2]
                ADD     r9, r9, r3          ; acc2 += *ptr++
                ADD     r8, r8, r4          ; acc1 += *ptr++
                MEND

                MACRO
                stage3
                LDM     r3, {r3-r5}
                ADD     r9, r9, r3          ; acc2 += *ptr++
                ADD     r8, r8, r4          ; acc1 += *ptr++
                ADD     r7, r7, r5          ; acc0 += *ptr++
                MEND
                
                MACRO
                stage3f
                LDM     r3, {r3-r5}
                ADD     r3, r3, r9
                ASRS    r3, r3, #16
                STRH    r3, [r2]            ; *output = (acc2 + *ptr) >> 16
                ADDS    r2, r2, #8          ; output += 4
                ADD     r4, r4, r8
                MOV     r9, r4              ; acc2 = acc1 + *ptr
                ADD     r5, r5, r7
                MOV     r8, r5              ; acc1 = acc0 + *ptr
                MEND
                
                MACRO
$label          decimate    $e
                ; r0    = 0b010101010101010
                ; r1    = input samples (stride of 2 x 8b)
                ; r2    = output samples (stride of 4 x 16b)

                ; r3-r5 = Table Lookup values
                ; r6    = DECIMATION_TABLE
                ; r7    = Acc0
                
                ; r8    = Acc1
                ; r9    = Acc2
                ; r12   = Loop Counter

                PUSH    {r4-r7}         ; Preserve to callstack
                MOV     r4, r8
                MOV     r5, r9
                PUSH    {r4-r5}
                PUSH    {r0}
                LDM     r0!, {r4,r5}    ; Load our accumulators
                MOV     r8, r4
                MOV     r9, r5

                LDR     r6, =0x08000000 >> (SAMPLES_PER_IRQ-1)
                MOV     r12, r6
                LDR     r6, =DECIMATION_TABLE
                LDR     r0, =0x2aaa

$label.next
                set_table   $e, 0, 0        ; r3 = DECIMATION_TABLE[byte][0]
                stage2
                set_table   $e, 1, 2        ; r3 = DECIMATION_TABLE[byte][2]
                stage2
                set_table   $e, 2, 4        ; r3 = DECIMATION_TABLE[byte][4]
                stage2
                set_table   $e, 3, 6        ; r3 = DECIMATION_TABLE[byte][6]
                stage2
                set_table   $e, 4, 8        ; r3 = DECIMATION_TABLE[byte][8]
                stage3i
                set_table   $e, 5, 11       ; r3 = DECIMATION_TABLE[byte][11]
                stage3
                set_table   $e, 6, 14       ; r3 = DECIMATION_TABLE[byte][14]
                stage3
                set_table   $e, 7, 17       ; r3 = DECIMATION_TABLE[byte][17]
                stage3
                set_table   $e, 8, 20       ; r3 = DECIMATION_TABLE[byte][20]
                stage3
                set_table   $e, 9, 23       ; r3 = DECIMATION_TABLE[byte][23]
                stage3
                set_table   $e, 10, 26      ; r3 = DECIMATION_TABLE[byte][26]
                stage3
                set_table   $e, 11, 29      ; r3 = DECIMATION_TABLE[byte][29]
                stage3f

                ADDS    r1, r1, #0x18       ; Increment input samples
                ADD     r12, r12            ; r12 = r12 << 1
                CMP     r6, r12             ; r12 < r6
                BLT     $label.end
                B       $label.next

$label.end      POP     {r0}
                MOV     r4, r8              ; Stash our accumulators
                MOV     r5, r9
                STM     r0!, {r4-r5}

                POP     {r4-r5}             ; Register restoration
                MOV     r8, r4
                MOV     r9, r5
                POP     {r4-r7}
                BX      lr
                MEND

; Inner decimation loop function
dec_odd         PROC
odd             decimate    0
                ENDP

dec_even        PROC
even            decimate    1
                ENDP

; Reset handler routine
start_mic_spi   PROC
                ; R0 = TIM_CR1_CEN
                ; R1 = MIC_SPI_CR1
                ; R2 = TIM15->CR1

                CPSID   I
                STRH    r0, [r2, #0]
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                LDR     r0, =0x40013000   ; SPI1->CR1
                LDR     r3, =0x40003800   ; SPI2->CR1
                STRH    r1, [r0, #0]
                STRH    r1, [r3, #0]
                CPSIE   I
                BX      lr

                ALIGN

                ENDP
                END
