; 6522 is located at $7E00 - $7E0F (actually to $7FFF)
    .org $8000
reset:
    ; set port A to output
    lda #$ff
    sta $7E03
loop:
    lda #$55
    sta $7E01
    lda #$aa
    sta $7E01
    jmp loop

    .org $fffc
    .word reset
    .word $0000