#include <Arduino.h>
#define STCP PB2 // Pin 10 on Arduino Uno
#define WE PC0   // Pin A0 on Arduino Uno
#define OE PC1   // Pin A1 on Arduino Uno
#define CE PC2   // Pin A2 on Arduino Uno
// DDR: Data Direction Register (1-> OUT, 0-> IN)
// DDRD: D0-D7 (0b11111111 -> All OUT)
// DDRB: D8-D13 (0bxx111111 -> D8-D13 OUT)
// DDRC: A0-A5 (0bxx111111 -> A0-A5 OUT)

// PORT: Output Register (1-> HIGH, 0-> LOW)
// PORTD: D0-D7
// PORTB: D8-D13
// PORTC: A0-A5

/*When DDR is set to 0 (Input Mode):
This is the "Under-the-Hood" secret you need to know for your EEPROM read/write cycles.

Set PORT bit to 1: It enables an internal 20k Pull-up Resistor. This gently pulls the pin to 5V.

Set PORT bit to 0: The pin is in High-Impedance (Hi-Z) mode. It’s completely floating/disconnected.

Crucial for your EEPROM: When you switch the Data Bus to "Read" mode (setting DDR bits to 0),
you should also set the PORT bits to 0. You don't want internal pull-up resistors fighting
the EEPROM’s output drivers.*/

// PIN: Pin State Register (Read Only)
// PIND: D0-D7
// PINB: D8-D13
// PINC: A0-A5

void setDataBusInput()
{
    DDRD &= ~0xFC;  // Set D2-D7 as INPUT (0b00000011)
    DDRB &= ~0x03;  // Set D8-D9 as INPUT (0b11111100)
    PORTD &= ~0xFC; // Disable pull-up resistors on D2-D7
    PORTB &= ~0x03; // Disable pull-up resistors on D8-D9
}

void setDataBusOutput()
{
    DDRD |= 0xFC; // Set D2-D7 as OUTPUT (0b11111100)
    DDRB |= 0x03; // Set D8-D9 as OUTPUT (0b00000011)
}

void setAddress(uint16_t addr)
{
    // 1. Drop the Latch (Pin 10 / PB2) to start the 'recording'
    PORTB &= ~(1 << STCP);

    // 2. Send the High Byte (A8 - A15)
    // We shift the 16-bit 'addr' right by 8 to get the top 8 bits
    SPDR = (uint8_t)(addr >> 8);

    // Wait for the SPIF flag in SPSR to flip to 1 (Transmission Complete)
    while (!(SPSR & (1 << SPIF)))
        ;

    // 3. Send the Low Byte (A0 - A7)
    // Casting to uint8_t automatically grabs the bottom 8 bits
    SPDR = (uint8_t)(addr);

    // Wait again for the hardware to finish the second byte
    while (!(SPSR & (1 << SPIF)))
        ;

    // 4. Raise the Latch (Pin 10 / PB2)
    // This physically transfers the bits from the 595 shift registers
    // to the output pins (and your 15 LEDs)
    PORTB |= (1 << STCP);
}

uint8_t read(uint16_t addr)
{
    setAddress(addr);    // Set the address to read from
    PORTC |= (1 << WE);  // WE HIGH (Inactive)
    PORTC &= ~(1 << OE); // OE LOW (Active)
    __asm__("nop");      // Short delay to allow signals to stabilize
    uint8_t data = 0;
    // Read the data from the Data Bus (D2-D7, D8-D9)
    data |= ((PIND >> 2) & 0x3F); // Shift D2-D7 down to bits 0-5
    data |= ((PINB << 6) & 0xC0); // Shift D8-D9 up to bits 6-7
    return data;
}

void write(uint16_t addr, uint8_t data)
{
    setAddress(addr); // Set the address to write to

    PORTD = (PORTD & 0x03) | ((data << 2) & 0xFC); // Place data on D2-D7
    PORTB = (PORTB & 0xFC) | ((data >> 6) & 0x03); // Place data on D8-D9

    __asm__("nop"); // tAS and tAH timing delay

    PORTC &= ~(1 << WE); // WE LOW (Active)
    __asm__("nop");      // Short delay to allow signals to stabilize
    PORTC |= (1 << WE);  // WE HIGH (Inactive)
}

void SDPUnlock()
{
    setDataBusOutput(); // Switch Data Bus to OUTPUT mode
    PORTC |= (1 << OE); // OE HIGH (Inactive)
    PORTC |= (1 << WE); // WE HIGH (Inactive)

    write(0x5555, 0xAA);
    write(0x2AAA, 0x55);
    write(0x5555, 0x80);
    write(0x5555, 0xAA);
    write(0x2AAA, 0x55);
    write(0x5555, 0x20);
    delay(11);
    write(0x0000, 0x42); // Check
    delay(11);
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("System Starting...");
    // Setting Up Hardware SPI
    DDRB |= 0b00101100; // Set D10 (SS), D11 (MOSI), D13 (SCK) as OUTPUT ADDR PINS
    SPCR = 0b01010000;  // Enable SPI, Master Mode, Clock = Fosc/16=1MHz
    SPSR = 0x00;        // Clearing the register
    setAddress(0x0000); // Clear any garbage on the shift registers

    DDRC |= 0x07; // Set A0-A2 as OUTPUT (0b00000111) Control PINS

    PORTC &= ~(1 << CE); // CE LOW (Active)

    PORTC |= (1 << WE);                // WE HIGH (Inactive)
    PORTC &= ~(1 << OE);               // OE LOW (Active)
    setDataBusInput();                 // Start with Data Bus in INPUT mode (for reading)
    Serial.println(read(0x0000), HEX); // Before

    setDataBusOutput();  // Switch Data Bus to OUTPUT mode
    PORTC |= (1 << OE);  // OE HIGH (Inactive)
    PORTC |= (1 << WE);  // WE HIGH (Inactive)
    write(0x0000, 0xAA); // Write a test value to the EEPROM
    delay(11);           // Wait for the write cycle to complete tWC = 10

    setDataBusInput();                 // Start with Data Bus in INPUT mode (for reading)
    Serial.println(read(0x0000), HEX); // After writing 0xAA, we should read back 0xAA

    // SDPUnlock(); // Unlock the EEPROM for writing

    // setDataBusInput(); // Start with Data Bus in INPUT mode (for reading)
    // Serial.println(read(0x0000), HEX); // After
    PORTC |= (1 << CE); // CE HIGH (Inactive)
}

void loop()
{
    // put your main code here, to run repeatedly:
}
