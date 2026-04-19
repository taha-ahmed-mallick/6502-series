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

    lda #"H" ; loading character 'H'
    jsr print_char

    lda #"e" ; loading character 'e'
    jsr print_char

    lda #"l" ; loading character 'l'
    jsr print_char

    lda #"l" ; loading character 'l'
    jsr print_char

    lda #"o" ; loading character 'o'
    jsr print_char

    lda #"," ; loading character ','
    jsr print_char

    lda #" " ; loading character ' '
    jsr print_char

    lda #"w" ; loading character 'w'
    jsr print_char

    lda #"o" ; loading character 'o'
    jsr print_char

    lda #"r" ; loading character 'r'
    jsr print_char

    lda #"l" ; loading character 'l'
    jsr print_char

    lda #"d" ; loading character 'd'
    jsr print_char

    lda #"!" ; loading character '!'
    jsr print_char

loop:
    jmp loop

lcd_instruction:
    sta PORTB
    lda #0 ; clear RS/RW/E
    sta PORTA

    lda #E ; set E
    sta PORTA

    lda #0 ; Clear RS/RW/E
    sta PORTA
    rts

print_char:
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