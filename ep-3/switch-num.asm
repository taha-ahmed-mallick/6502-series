    .org $8000
reset:
    sta $6000
    stx $6000
    sty $6000
    ; all registers were set to 0
    lda #$aa
    sta $6000

    ldx #$55
    stx $4000
    lda #$00
    ldx #$00
loop:
    lda $6000
    ldx $4000

    sta $4000
    stx $6000
    jmp loop

    .org $fffc
    .word reset
    .word $0000