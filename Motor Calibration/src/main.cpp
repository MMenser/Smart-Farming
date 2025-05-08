#include <Arduino.h>

#define dirPin 2
#define stepPin 3
int stepsPerRevolution = 60;

void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(dirPin, LOW);

  for (int i = 0; i < stepsPerRevolution; i++)
  {
    // These four lines result in 1 step:
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(5000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(5000);
  } // Need to determine increase in voltage for each revolution
}

void loop() {
  // put your main code here, to run repeatedly:
}