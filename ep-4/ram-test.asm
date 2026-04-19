    .org $8000
reset:
    sta $6000
    stx $6000
    sty $6000
    ; all registers weren't set to 0 (they were in a random state)
    lda #$aa
    sta $6000
    lda $6000

    ldx #$55
    stx $4000
    lda $4000

    lda #$55
    sta $4000
    lda $4000

    ldy #$ff
    sty $2000
    lda $2000

    lda #$00
    sta $3000
    lda $3000

    lda #$0f
    sta $5000
    lda $5000

    lda #$f0
    sta $7000
    lda $7000

    ; Stack check
    ; A = $f0
    ; X = $55
    ; Y = $ff
    pha
    phx
    phy
    ply
    plx
    pla

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