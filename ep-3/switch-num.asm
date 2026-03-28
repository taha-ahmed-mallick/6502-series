    .org $8000
reset:
    sta $6000
    stx $6000
    sty $6000
    ; all registers were set to 0
    lda #$aa
    sta $6000

    ldy #$55
    sty $4000
    lda #$00
    ldy #$00
loop:
    lda $6000
    ldy $4000

    sta $4000
    sty $6000
    jmp loop

    .org $fffc
    .word reset
    .word $0000