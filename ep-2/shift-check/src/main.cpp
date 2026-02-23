#include <Arduino.h>
#define STCP A2
#define SHCP A1
#define DS A0

void setup()
{
    // put your setup code here, to run once:
    pinMode(STCP, OUTPUT);
    pinMode(SHCP, OUTPUT);
    pinMode(DS, OUTPUT);

    digitalWrite(STCP, LOW);
    // two shift registers daisy chained together, so we need to send 16 bits of data
    shiftOut(DS, SHCP, MSBFIRST, 0xf0); // HIGH BYTE
    shiftOut(DS, SHCP, MSBFIRST, 0xaa); // LOW BYTE
    digitalWrite(STCP, HIGH);
    digitalWrite(STCP, LOW);
}

void loop()
{
    // put your main code here, to run repeatedly:
}