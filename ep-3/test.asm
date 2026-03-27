    .org $8000
reset:
    lda #$aa
    sta $6000

    lda #$55
    sta $4000

loop:
    lda $6000
    sta $4000

    lda $4000
    sta $6000

    jmp loop

    .org $fffc
    .word reset
    .word $0000