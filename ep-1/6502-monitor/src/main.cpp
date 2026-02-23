#include <Arduino.h>

const char ADDR[16] = {A4, A3, A2, A1, A0, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3};
#define CLOCK 2
#define RW A5
int count = 0;

void onClock();

void setup() {
  // put your setup code here, to run once:
  for (int i = 0; i < 16; i++) {
    pinMode(ADDR[i], INPUT);
  }
  pinMode(CLOCK, INPUT);
  attachInterrupt(digitalPinToInterrupt(CLOCK), onClock, RISING);
  Serial.begin(57600);
}

void onClock() {
  char output[15];
  Serial.print("Clock triggered ");
  Serial.println(count);
  unsigned int address = 0;
  for (int i = 0; i < 16; i++) {
    int bit = digitalRead(ADDR[i]) ? 1 : 0;
    Serial.print(bit);
    address = (address << 1) + bit;
  }
  sprintf(output, "\t%04x %c", address, digitalRead(RW) ? 'r' : 'w');
  Serial.println(output);
  count++;
}

void loop() {
  // put your main code here, to run repeatedly:
}
