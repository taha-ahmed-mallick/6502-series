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

// CORE FUNCTIONS
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

// BASIC FUNCTIONS
uint8_t readRaw(uint16_t addr)
{
    setAddress(addr); // Set the address to read from
    __asm__("nop");   // Short delay to allow signals to stabilize
    uint8_t data = 0;
    // Read the data from the Data Bus (D2-D7, D8-D9)
    data |= ((PIND >> 2) & 0x3F); // Shift D2-D7 down to bits 0-5
    data |= ((PINB << 6) & 0xC0); // Shift D8-D9 up to bits 6-7
    return data;
}

void writeRaw(uint16_t addr, uint8_t data)
{
    setAddress(addr); // Set the address to write to

    PORTD = (PORTD & 0x03) | ((data << 2) & 0xFC); // Place data on D2-D7
    PORTB = (PORTB & 0xFC) | ((data >> 6) & 0x03); // Place data on D8-D9

    __asm__("nop"); // tAS and tAH timing delay

    PORTC &= ~(1 << WE); // WE LOW (Active)
    __asm__("nop");      // Short delay to allow signals to stabilize
    PORTC |= (1 << WE);  // WE HIGH (Inactive)
}

int8_t pageWrite(uint16_t startAddr, uint8_t *data, uint8_t length)
{
    if (startAddr > 0x7FFF)
        return 0; // Address out of range
    if ((startAddr % 64) != 0)
        return -1; // Inalignment error: start address must be a multiple of 64
    if (length == 0 || length > 64)
        return -2; // Invalid length: must be between 1 and 64

    setDataBusOutput();  // Switch Data Bus to OUTPUT mode
    PORTC |= (1 << OE);  // OE HIGH (Inactive)
    PORTC |= (1 << WE);  // WE HIGH (Inactive)1
    PORTC &= ~(1 << CE); // CE LOW (Active)

    uint8_t bytesWritten = 0;
    for (; bytesWritten < length; bytesWritten++)
        writeRaw(startAddr + bytesWritten, data[bytesWritten]);

    delay(11);          // just required after the last byte is written, tWC = 10ms
    PORTC |= (1 << CE); // CE HIGH (Inactive)
    return bytesWritten;
}

// UTILITY FUNCTIONS
void SDPUnlock()
{
    PORTC &= ~(1 << CE); // CE LOW (Active)
    setDataBusOutput();  // Switch Data Bus to OUTPUT mode
    PORTC |= (1 << OE);  // OE HIGH (Inactive)
    PORTC |= (1 << WE);  // WE HIGH (Inactive)

    writeRaw(0x5555, 0xAA);
    writeRaw(0x2AAA, 0x55);
    writeRaw(0x5555, 0x80);
    writeRaw(0x5555, 0xAA);
    writeRaw(0x2AAA, 0x55);
    writeRaw(0x5555, 0x20);
    delay(11);
    // writeRaw(0x0000, 0x42); // Check
    // delay(11);
    PORTC |= (1 << CE); // CE HIGH (Inactive)
}

void read(uint16_t start, uint16_t stop)
{
    PORTC &= ~(1 << CE); // CE LOW (Active)
    setDataBusInput();
    PORTC |= (1 << WE);  // WE HIGH (Inactive)
    PORTC &= ~(1 << OE); // OE LOW (Active)

    bool firstRead = true;
    char format[64];
    bool patternPrinted = false;
    uint8_t prevData[16];
    uint8_t currentData[16];

    for (uint16_t address = start; address <= stop; address++)
    {
        uint8_t data = readRaw(address);
        if (firstRead)
        {
            prevData[address % 16] = data;
            if (address % 16 == 15)
            {
                firstRead = false;
                sprintf(format, "0x%04X : %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n", address - 15, prevData[0], prevData[1], prevData[2], prevData[3], prevData[4], prevData[5], prevData[6], prevData[7], prevData[8], prevData[9], prevData[10], prevData[11], prevData[12], prevData[13], prevData[14], prevData[15]);
                Serial.print(format);
            }
        }
        else
        {
            currentData[address % 16] = data;
            if (address % 16 == 15)
            {
                // Compare currentData with prevData
                bool currentMatch = memcmp(currentData, prevData, 16) == 0;
                if (currentMatch)
                {
                    patternPrinted = true;
                    if (address == stop)
                    {
                        char format[16];
                        sprintf(format, "0x%04X : *\n", address - 15);
                        Serial.print(format);
                    }
                }
                else
                {
                    if (patternPrinted)
                    {
                        patternPrinted = false;
                        char format[16];
                        sprintf(format, "0x%04X : *\n", address - 31);
                        Serial.print(format);
                    }
                    sprintf(format, "0x%04X : %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n", address - 15, currentData[0], currentData[1], currentData[2], currentData[3], currentData[4], currentData[5], currentData[6], currentData[7], currentData[8], currentData[9], currentData[10], currentData[11], currentData[12], currentData[13], currentData[14], currentData[15]);
                    Serial.print(format);
                    // Copy currentData to prevData for the next comparison
                    memcpy(prevData, currentData, 16);
                }
            }
        }
    }
    PORTC |= (1 << CE);                                                          // CE HIGH (Inactive)
    Serial.println("======================Read Complete======================"); // Final newline after reading is done
}

void blank(uint16_t start = 0x0000, uint16_t stop = 0x7fff)
{
    PORTC &= ~(1 << CE); // CE LOW (Active)
    setDataBusOutput();  // Switch Data Bus to OUTPUT mode
    PORTC |= (1 << OE);  // OE HIGH (Inactive)
    PORTC |= (1 << WE);  // WE HIGH (Inactive)

    uint16_t totalBytes = stop - start + 1;
    int barWidth = 40, pos = 0; // Blanking: [/40/] 000%
    float progress = 0;

    uint8_t page[64]; // Buffer for page writes, pre-filled with 0xFF
    memset(page, 0xFF, sizeof(page));

    if (start%64 !=0) {
        // If start address is not page-aligned, we need to write bytes one by one until we reach the next page boundary
        for (; start < stop && (start % 64) != 0; start++) {
            writeRaw(start, 0xFF);
            delay(11); // Wait for the write cycle to complete tWC = 10
        }
    }

    if (stop %64 != 63) {
        // If stop address is not the end of a page, we need to write bytes one by one from the last page boundary until we reach stop
        for (; stop > start && (stop % 64) != 63; stop--) {
            writeRaw(stop, 0xFF);
            delay(11); // Wait for the write cycle to complete tWC = 10
        }
    }

    for (uint16_t address = start; address <= stop; address+=64)
    {
        pageWrite(address, page, 64); // Write a full page of 0xFF
            progress = (float)(address - start) / totalBytes;
            pos = progress * barWidth;
            Serial.print("\e[2K\e[GBlanking: [");
            for (int i = 0; i < barWidth; i++)
            {
                if (i < pos)
                    Serial.print("-");
                else if (i == pos)
                    Serial.print(">");
                else
                    Serial.print(".");
            }
            char format[8];
            sprintf(format, "] %03d%%", (int)round(progress * 100));
            Serial.print(format);
    }
    PORTC |= (1 << CE); // CE HIGH (Inactive)
    Serial.println("\n====================Blanking Complete====================");
}

void write(uint16_t addr, uint8_t data)
{
    PORTC &= ~(1 << CE); // CE LOW (Active)
    setDataBusOutput();  // Switch Data Bus to OUTPUT mode
    PORTC |= (1 << OE);  // OE HIGH (Inactive)
    PORTC |= (1 << WE);  // WE HIGH (Inactive)
    writeRaw(addr, data);
    delay(11);          // Wait for the write cycle to complete tWC = 10
    PORTC |= (1 << CE); // CE HIGH (Inactive)
}

uint8_t read(uint16_t addr)
{
    PORTC &= ~(1 << CE);  // CE LOW (Active)
    setDataBusInput();    // Switch Data Bus to INPUT mode
    PORTC |= (1 << WE);   // WE HIGH (Inactive)
    PORTC &= ~(1 << OE);  // OE LOW (Active)
    delayMicroseconds(1); // Short delay to allow signals to stabilize
    PORTC |= (1 << CE);   // CE HIGH (Inactive)
    return readRaw(addr);
}

void setup()
{
    // put your setup code here, to run once:
    PORTC |= 0x07; // Set A0-A2 HIGH (0b00000111) Inactive State for Control PINS
    DDRC |= 0x07;  // Set A0-A2 as OUTPUT (0b00000111) Control PINS
    delay(1);      // just to settle things down
    // setting up serial communication
    Serial.begin(115200);
    while (!Serial)
        ;        // Wait for Serial to be ready (for native USB devices)
    delay(2000); // Give the user a moment to open the Serial Monitor
    Serial.println("System Starting...");
    // Setting Up Hardware SPI
    DDRB |= 0b00101100; // Set D10 (SS), D11 (MOSI), D13 (SCK) as OUTPUT ADDR PINS
    SPCR = 0b01010001;  // Enable SPI, Master Mode, Clock = Fosc/16=1MHz
    SPSR = 0x00;        // Clearing the register
    setAddress(0x0000); // Clear any garbage on the shift registers

    PORTC &= ~(1 << CE); // CE LOW (Active)
    PORTC |= (1 << WE);  // WE HIGH (Inactive)
    PORTC &= ~(1 << OE); // OE LOW (Active)
    
    // blank(); // Blank the EEPROM to 0xFF (All bits set to 1)
    // delay(100); // Just to be safe, give it a moment to finish blanking
    uint8_t arr[64];
    for (uint8_t i = 0; i < 64; i++)
        (i % 2 == 0) ? (arr[i] = 0x55) : (arr[i] = 0xAA); // Fill the array with a checkerboard pattern
    for (uint16_t i = 0; i < 512; i++)
    {
        pageWrite(i * 64, arr, 64); // Write the checkerboard pattern to the first 64 pages (4096 bytes)
    }
    read(0x0000, 0x7fff); // Read whole eeprom
    Serial.println("Awaiting Commands...");
}

void loop()
{
    // put your main code here, to run repeatedly:
    while (Serial.available() > 0)
    {
        char cmd = Serial.read();
        // v-> version, b -> blank, r -> read, w -> write, d -> disable, e-> enable
        switch (cmd)
        {
        case 'v':
            Serial.println("EEPROM PROGRAMMER v1.0 --- Active");
            break;
        case 'b':
            blank();
            break;
        case 'r':
            read(0x0000, 0x7fff);
            break;
        case 'w':
            break;
        default:
            Serial.println("Unknown command.");
            break;
        }
    }
}
