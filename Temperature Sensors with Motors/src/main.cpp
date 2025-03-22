#include <Adafruit_MAX31865.h>
#define dirPin 6
#define stepPin 7
#define stepsPerRevolution 200

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 sensor1 = Adafruit_MAX31865(9, 11, 12, 13); // CS pin 10 -- CS pins are what allows Arduino board to identify seperate devices
Adafruit_MAX31865 ambientSensor = Adafruit_MAX31865(5, 11, 12, 13); 
Adafruit_MAX31865 sensor3 = Adafruit_MAX31865(4, 11, 12, 13); 
Adafruit_MAX31865 sensor4 = Adafruit_MAX31865(3, 11, 12, 13); 
Adafruit_MAX31865 sensor5 = Adafruit_MAX31865(2, 11, 12, 13); 
// use hardware SPI, just pass in the CS pin
// Adafruit_MAX31865 sensor1 = Adafruit_MAX31865(9); // CS pin 10
// Adafruit_MAX31865 sensor2 = Adafruit_MAX31865(5);  // CS pin 9 -- CS pins are what allows Arduino board to identify seperate devices

#define RREF      430.0
#define RNOMINAL  100.0

void setup() {
  Serial.begin(9600);
  
  sensor1.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  ambientSensor.begin(MAX31865_3WIRE);
  sensor3.begin(MAX31865_3WIRE);
  sensor4.begin(MAX31865_3WIRE);
  sensor5.begin(MAX31865_3WIRE);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(dirPin, LOW);
}

unsigned long lastUpdate = 0;  // Stores last sensor update time
const unsigned long updateInterval = 15000;  // 15 seconds

float ambientTemperature, temp1, temp2, temp3, temp4, average, target, delta;  // Store sensor values

void loop() {
  unsigned long currentMillis = millis();

  // Update sensor data every 15 seconds
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;  // Reset timer

    ambientTemperature = ambientSensor.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp1 = sensor1.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp2 = sensor3.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp3 = sensor4.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp4 = sensor5.temperature(RNOMINAL, RREF) * 1.8 + 32;    

    Serial.println("Updated sensor readings:");
    Serial.print("Ambient Temperature: "); Serial.println(ambientTemperature);
    Serial.print("Sensor 1: "); Serial.println(temp1);
    Serial.print("Sensor 2: "); Serial.println(temp2);
    Serial.print("Sensor 3: "); Serial.println(temp3);
    Serial.print("Sensor 4: "); Serial.println(temp4);
    Serial.println("----------------------");

    average = (temp1 + temp2 + temp3 + temp4)/4;
  }
  target = ambientTemperature + 10.00; // Target is 10 degrees above ambient
  delta = target - average;
  Serial.print("Target Temperature: "); Serial.println(target);
  Serial.print("Average Temperature: "); Serial.println(average);
  Serial.print("Delta: "); Serial.println(delta);
  
  if (delta > 2.00) {
    digitalWrite(dirPin, HIGH);
    for (int i = 0; i < stepsPerRevolution; i++) {
      // These four lines result in 1 step:
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(5000);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(5000);
    } // Need to determine increase in voltage for each revolution
  }
  else if (delta < -2.00) {
    digitalWrite(dirPin, LOW);
    for (int i = 0; i < stepsPerRevolution; i++) {
      // These four lines result in 1 step:
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(5000);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(5000);
    } // Need to determine increase in voltage for each revolution
  }

  delay(10000);
}

void increaseVoltage() {

}