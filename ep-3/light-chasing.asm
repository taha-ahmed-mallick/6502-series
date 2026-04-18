    .org $8000
reset:
    lda #$ff
    sta $7E03
    lda #$50
loop:
    sta $7E01
    ror
    jmp loop

    .org $fffc
    .word reset
    .word $0000