; 0000-7DFF -> RAM
; 7E00-7E0F -> 6522
; 8000-FFFF -> ROM
PORTA = $7E01
PORTB = $7E00
DDRA = $7E03
DDRB = $7E02

    .org $8000
reset:
; setting up necessary pins for output
    lda #%11111111
    sta DDRB
    lda #%11100000
    sta DDRA

    .org $fffc
    .word reset
    .word $0000