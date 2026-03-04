#include <Arduino.h>

// uint32_t addressPins[] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA15, PB0, PB1, PB2}; // LSB to MSB
// uint32_t dataPins[] = {PB6, PB7, PB8, PB9, PB10, PB12, PB13, PB14}; // LSB to MSB
// uint32_t controlPins[] = {PB3, PB4, PB5}; // CE, OE, WE

// const size_t addressPinCount = sizeof(addressPins) / sizeof(addressPins[0]);
// const size_t dataPinCount = sizeof(dataPins) / sizeof(dataPins[0]);
// const size_t controlPinCount = sizeof(controlPins) / sizeof(controlPins[0]);

void setDataOUT() {
    GPIOB->MODER = (GPIOB->MODER & 0xC0C00FFF) | 0x15155000; // setting PB6-PB10; PB12-PB14 as OUTPUT (Data Pins)
}
void setDataIN() {
    GPIOB->MODER = (GPIOB->MODER & 0xC0C00FFF); // setting PB6-PB10; PB12-PB14 as INPUT (Data Pins)
}

void setAddress(uint16_t address) {
    GPIOA->BSRR = (0x87FF << 16); // Clear PA0-PA10, PA15
    uint32_t setMaskA = 0;
    setMaskA |= (address & 0x07FF) | ((address & 0x800) << 4); // A0-A10; A11 to PA15
    GPIOA->BSRR = setMaskA;

    GPIOB->BSRR = (0x07 << 16); // Clear PB0-PB2
    uint32_t setMaskB = 0;
    setMaskB = (address >> 12) & 0x07; // A12-A14 to PB
    GPIOB->BSRR = setMaskB;
}

void setData(uint8_t data) {
    GPIOB->BSRR = (0x77C0 << 16); // Clear PB6-PB10, PB12-PB14
    
    // Set Data Pins
    uint32_t setMask = 0;
    setMask |= (data & 0x1F) << 6;
    setMask |= (data & 0xE0) << 7;
    GPIOB->BSRR = setMask;
}

uint8_t read(uint16_t address) {
    setDataIN();
    setAddress(address);

    // ENSURE WE IS HIGH and OE IS LOW
    GPIOB->BSRR = (1 << 5);       // WE Inactive
    GPIOB->BSRR = (1 << (4+16)); // OE Active (Low)
    for (int i = 0; i < 10000; i++) __asm__("nop"); 

    uint32_t val = GPIOB->IDR;
    uint8_t data = 0;
    data |= ((val >> 6) & 0x1F) | ((val >> 7) & 0xE0); // Extract D0-D4 from PB6-PB10 // Extract D5-D7 from PB12-PB14 (skipping PB11)
    return data;
}

void read(int start, int stop) {
    for (int address = start; address <= stop; address++)
    {
        uint8_t data = read(address);
        if (address % 16 == 0)
        {
            char format[12];
            sprintf(format, "0x%04x : ", address);
            Serial.print(format);
        }
        char format[3];
        sprintf(format, "%02x", data);
        Serial.print(format);
        Serial.print(" ");
        if (address % 16 == 15)
            Serial.println();
        delayMicroseconds(1);
    }
}

void write(uint16_t address, uint8_t data) {
    setDataOUT();
    setAddress(address);
    setData(data);
    
    GPIOB->BSRR = (1 << 4); // OE InActive (High)
    GPIOB->BSRR = 1 << 5; // WE HIGH (inactive)
    
    for (int i = 0; i < 150; i++)
        __asm__("nop"); // Short delay for address stabilization
    delayMicroseconds(50); // Short delay for control signal stabilization

    GPIOB->BSRR = 1 << (5+16); // WE LOW (active)
    delayMicroseconds(1); // Short delay for write operation
    GPIOB->BSRR = 1 << 5; // WE HIGH (inactive)
    delay(10);
}

// A "Raw" write doesn't have the 10ms delay, used for sequences
void writeRaw(uint16_t address, uint8_t data) {
    setAddress(address);
    setData(data);

    // Give it 100us to settle (The limit is 150us total)
    for (volatile int i = 0; i < 2500; i++) __asm__("nop"); 

    // 2. Pulse WE
    GPIOB->BSRR = 1 << (5 + 16); // WE LOW
    for (volatile int i = 0; i < 500; i++) __asm__("nop"); // Ensure pulse is seen
    GPIOB->BSRR = 1 << 5;        // WE HIGH (Data latched)

    // 3. Short recovery
    for (volatile int i = 0; i < 100; i++) __asm__("nop");
}

void unlockEEPROM() {
    setDataOUT();
    GPIOB->BSRR = 1 << (3+16); // CE LOW (active)
    GPIOB->BSRR = 1 << 4; // OE HIGH (inactive)
    GPIOB->BSRR = 1 << 5; // WE HIGH (inactive)
    delay(500);
    // The Magic Sequence (Standard JEDEC)
    writeRaw(0x5555, 0xAA);
    writeRaw(0x2AAA, 0x55);
    writeRaw(0x5555, 0x80);
    writeRaw(0x5555, 0xAA);
    writeRaw(0x2AAA, 0x55);
    writeRaw(0x5555, 0x20); // 0x20 is the command to disable protection
    
    // delay(20); // Give it time to process the internal change
}

void setup()
{
    Serial.begin(115200);
    Serial.setTimeout(10);
    while (!Serial)
    {
        delay(10);
    }
    // put your setup code here, to run once:
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN);
    GPIOA->MODER = (GPIOA->MODER & 0x3FC00000) | 0x40155555; // setting PA0-PA10, PA15 as OUTPUT (Address Pins A0-A11)
    GPIOB->MODER = (GPIOB->MODER & 0xFFFFF000) | 0x00000555; // setting PB0-PB2, PB3-PB5 as OUTPUT (Address Pins A12-A14, Control Pins)

    GPIOB->BSRR = 1 << (3+16); // CE LOW (active)
    setDataIN();
    GPIOB->BSRR = 1 << (4+16); // OE LOW (active)
    GPIOB->BSRR = 1 << 5; // WE HIGH (inactive)

    uint8_t value = read(0x0000); // Example read
    Serial.println("System Ready.");
    Serial.print("Read value at address 0x0000: 0x");
    Serial.println(value, HEX);

    unlockEEPROM(); // Disable write protection so we can read/write freely
    // // all my code will go here:

    setDataOUT();
    GPIOB->BSRR = 1 << 4; // OE HIGH (inactive)
    for (int i = 0; i < 0x20; i++) {
        write(i, i); // Write a sequence of bytes for testing
    }

    setDataIN();
    GPIOB->BSRR = 1 << (4+16); // OE LOW (active)
    read(0x0000, 0x0020); // Example: Read first 256 bytes (0x0000 to 0x00ff)
    // till here after this chip is disabled
    GPIOB->BSRR = 1 << 3; // CE HIGH (inactive)
}

void loop()
{
    // put your main code here, to run repeatedly:
}