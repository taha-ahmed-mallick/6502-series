; 0000-7DFF -> RAM
; 7E00-7E0F -> 6522
; 8000-FFFF -> ROM
PORTA = $7E01
PORTB = $7E00
DDRA = $7E03
DDRB = $7E02

E = %10000000
RW = %01000000
RS = %00100000

    .org $8000
reset:
    ldx #$ff
    txs ; Initialize stack pointer
; setting up necessary pins for output
    lda #%11111111
    sta DDRB
    lda #%11100000
    sta DDRA

    lda #%00111000 ; set 8-bit mode, 2 line display, 5x8 font
    jsr lcd_instruction

    lda #%00001111 ; Display on, cursor on, blinking on
    jsr lcd_instruction

    lda #%00000110 ; Cursor increments and no display shift
    jsr lcd_instruction

    lda #%00000001 ; Clear display
    jsr lcd_instruction

    ldx #$00
display:
    lda message, x
    beq loop
    jsr print_char
    inx
    jmp display

loop:
    jmp loop

message: .asciiz "Hello, world!"

lcd_wait:
    pha
    lda #%00000000
    sta DDRB ; Set PORTB as input
lcdbusy:
    lda #RW ; Set RW
    sta PORTA
    lda #(RW | E) ; Set RW and E
    sta PORTA
    lda PORTB ; Read busy flag
    and #%10000000 ; Check if busy flag is set
    bne lcdbusy ; If busy, keep checking

    lda #RW ; Set RW
    sta PORTA
    lda #%11111111
    sta DDRB ; Set PORTB back to output
    pla
    rts


lcd_instruction:
    jsr lcd_wait
    sta PORTB
    lda #0 ; clear RS/RW/E
    sta PORTA

    lda #E ; set E
    sta PORTA

    lda #0 ; Clear RS/RW/E
    sta PORTA
    rts

print_char:
    jsr lcd_wait
    sta PORTB
    lda #RS ; set RS
    sta PORTA

    lda #(RS | E) ; set RS and E
    sta PORTA

    lda #RS ; Set RS
    sta PORTA
    rts

    .org $fffc
    .word reset
    .word $0000