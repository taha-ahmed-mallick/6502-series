#include <Arduino.h>

#define DS A0
#define SHCP A1
#define STCP A2
#define WE A3
#define OE A4
#define CE A5
#define eeprom_D0 2
#define eeprom_D7 9

void setAddress(int);
byte read(int);
void read(uint16_t, uint16_t);
void write(int, byte);

void fastWrite(int address, byte data)
{
    setAddress(address);
    for (int pin = eeprom_D0; pin <= eeprom_D7; pin++)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, (data >> (pin - eeprom_D0)) & 0x01);
    }
    digitalWrite(WE, LOW);
    delayMicroseconds(1);
    digitalWrite(WE, HIGH);
    // No 10ms delay here!
}

void unlockSDP()
{
    fastWrite(0x5555, 0xAA);
    fastWrite(0x2AAA, 0x55);
    fastWrite(0x5555, 0x80);
    fastWrite(0x5555, 0xAA);
    fastWrite(0x2AAA, 0x55);
    fastWrite(0x5555, 0x20);
    delay(10); // Final delay after the whole sequence
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(57600);

    pinMode(STCP, OUTPUT);
    pinMode(SHCP, OUTPUT);
    pinMode(DS, OUTPUT);
    pinMode(WE, OUTPUT);
    pinMode(OE, OUTPUT);
    pinMode(CE, OUTPUT);

    digitalWrite(WE, HIGH);
    digitalWrite(OE, LOW);
    digitalWrite(CE, LOW);
    read(0x0000, 0x7fff);
    // unlockSDP();
    // byte data = read(0x00);
    // Serial.print("0x00 : ");
    // Serial.println(data, HEX);
    // byte val = read(0x00);
    // Serial.println(val, HEX);
    // Serial.println(read(0b101010101011111));
    write(0x00, 0xaa);
    // Serial.println(read(0x000));
    // delay(1);
    // read(0x5550, 0x555f);
    // val = read(0x00);
    // Serial.println(val, HEX);
    // delay(5000);
    // digitalWrite(CE, HIGH);
    delay(5000);
    read(0x00);
    // setAddress(0x00);
}

void setAddress(int address)
{
    digitalWrite(STCP, LOW);
    // two shift registers daisy chained together, so we need to send 16 bits of data
    shiftOut(DS, SHCP, MSBFIRST, (address >> 8)); // HIGH BYTE
    shiftOut(DS, SHCP, MSBFIRST, address);        // LOW BYTE
    digitalWrite(STCP, HIGH);
    delayMicroseconds(1);
}

byte read(int address)
{
    for (int pin = eeprom_D0; pin <= eeprom_D7; pin++)
        pinMode(pin, INPUT);
    digitalWrite(WE, HIGH);
    digitalWrite(OE, LOW);
    setAddress(address);
    delayMicroseconds(1);
    byte data = 0;
    for (int pin = eeprom_D7; pin >= eeprom_D0; pin--)
    {
        data <<= 1;
        data |= digitalRead(pin);
    }
    return data;
}

void read(uint16_t start, uint16_t stop)
{
    for (uint16_t address = start; address <= stop; address++)
    {
        byte data = read(address);
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
        // if (address == 0x7fff) // last address: 7fff
        //     break;
    }
}

void write(int address, byte data)
{
    for (int pin = eeprom_D0; pin <= eeprom_D7; pin++)
        pinMode(pin, OUTPUT);
    digitalWrite(OE, HIGH);
    setAddress(address);
    delayMicroseconds(1);
    for (int pin = eeprom_D0; pin <= eeprom_D7; pin++)
    {
        digitalWrite(pin, data & 0x1);
        data >>= 1;
    }
    delayMicroseconds(1);
    digitalWrite(WE, LOW);
    delayMicroseconds(100);
    digitalWrite(WE, HIGH);
    delay(10);
    digitalWrite(OE, LOW);
    for (int pin = eeprom_D0; pin <= eeprom_D7; pin++)
        pinMode(pin, INPUT);
}

void loop()
{
    // put your main code here, to run repeatedly:
}