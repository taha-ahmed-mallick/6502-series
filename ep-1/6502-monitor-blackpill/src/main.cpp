#include <Arduino.h>

void onClock();

void printBinary(uint32_t value, uint8_t bits)
{
    for (int i = bits - 1; i >= 0; i--)
    { // prints MSB first
        Serial.print((value >> i) & 0x01);
    }
    Serial.print("\t");
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    while (!Serial);
    delay(2000);
    Serial.println("Starting...");
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN);
    GPIOA->MODER &= ~(0xC03FFFFF);
    GPIOB->MODER &= ~(0xFF3FF3FF);
    // GPIOA->PUPDR |= 0x40155555;
    // GPIOB->PUPDR |= 0x55155155;
    pinMode(PB12, INPUT);
    attachInterrupt(digitalPinToInterrupt(PB12), onClock, RISING); // FALLING
    pinMode(PC13, OUTPUT);
    digitalWrite(PC13, HIGH);
}

void onClock()
{
    uint32_t regA = GPIOA->IDR;
    uint32_t regB = GPIOB->IDR;

    bool rw;
    uint8_t data = 0;
    uint16_t addr = 0;

    rw = ((regB >> 13) & 0x01); // 1-> R, 0-> W

    data = ((regB >> 2) & 0x07);  // lower 3 bits of data
    data |= ((regB >> 3) & 0xF8); // upper 5 bits of data

    addr = (regA & 0xFF);           // PA0-PA7 for address
    addr |= ((regB & 0x03) << 8);   // PB0-PB1 for address
    addr |= ((regA & 0x700) << 2);  // PA8-PA10 for address
    addr |= ((regA & 0x8000) >> 2); // PA15 for address
    addr |= (regB & 0xC000);        // PB14-PB15 for address

    printBinary(addr, 16);
    printBinary(data, 8);

    char output[16];
    sprintf(output, "%04x  %c  %02x", addr, rw ? 'r' : 'W', data);

    Serial.println(output);
    digitalWrite(PC13, !digitalRead(PC13));
}

void loop()
{
    // put your main code here, to run repeatedly:
}