#include <Adafruit_MAX31865.h>
#include <SoftwareSerial.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
using namespace std;

void changeVoltage(bool increase, float changeTarget);
void resetVoltageToZero();
int maintainTemperature();
void sendLoraData(std::vector<float> dataToSend);
void sendSerialCSV(std::vector<float> dataToSend);

#define dirPin 6
#define stepPin 7
#define RREF      430.0
#define RNOMINAL  100.0
#define startingVOLTAGE 102.0

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 sensor3 = Adafruit_MAX31865(9, 11, 12, 13); // CS pin 10 -- CS pins are what allows Arduino board to identify seperate devices
Adafruit_MAX31865 ambientSensor = Adafruit_MAX31865(5, 11, 12, 13); 
Adafruit_MAX31865 sensor2 = Adafruit_MAX31865(4, 11, 12, 13); 
Adafruit_MAX31865 sensor1 = Adafruit_MAX31865(3, 11, 12, 13); 
Adafruit_MAX31865 sensor4 = Adafruit_MAX31865(2, 11, 12, 13); 
SoftwareSerial lora(0,1);

int stepsPerRevolution = 200; // About 10 +- V
int currentVoltage = startingVOLTAGE;
std::vector<float> recentTemperatures;
float target = 0.0;

void setup() {
  Serial.begin(9600);
  lora.begin(9600);
  sensor1.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  ambientSensor.begin(MAX31865_3WIRE);
  sensor2.begin(MAX31865_3WIRE);
  sensor3.begin(MAX31865_3WIRE);
  sensor4.begin(MAX31865_3WIRE);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(dirPin, LOW);
  lora.setTimeout(500);

}

unsigned long lastUpdate = 0;  // Stores last sensor update time
const unsigned long updateInterval = 15000;  // 15 seconds

float ambientTemperature, temp1, temp2, temp3, temp4, average, delta;  // Store sensor values

void loop() {
  unsigned long currentMillis = millis();
  
  // // Update sensor data every 15 seconds
  // if (currentMillis - lastUpdate >= updateInterval) {
    // lastUpdate = currentMillis;  // Reset timer

    ambientTemperature = ambientSensor.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp1 = sensor1.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp2 = sensor2.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp3 = sensor3.temperature(RNOMINAL, RREF) * 1.8 + 32;
    //temp3 = temp2; // Set sensor 3 to sensor 2 because of testing
    temp4 = sensor4.temperature(RNOMINAL, RREF) * 1.8 + 32;    

    Serial.println();
    Serial.println();
    Serial.println("Updated sensor readings:");
    Serial.print("Ambient Temperature: "); Serial.println(ambientTemperature);
    Serial.print("Sensor 1: "); Serial.println(temp1);
    Serial.print("Sensor 2: "); Serial.println(temp2);
    Serial.print("Sensor 3: "); Serial.println(temp3);
    Serial.print("Sensor 4: "); Serial.println(temp4);
    Serial.println("----------------------");

    average = (temp1 + temp2 + temp3 + temp4)/4;
    target = ambientTemperature + 10.0; // Target is 5 degrees above ambient
    delta = target - average;
    float minSensor = min(temp1, min(temp2, min(temp3, temp4)));
    float maxSensor = max(temp1, max(temp2, max(temp3, temp4)));
    recentTemperatures.push_back(average);
    if (recentTemperatures.size() == 5) { // Vector has 5 elements, pop the least recent one
      if (currentMillis - lastUpdate >= 60000) { // Only run maintainTemperature() every minute
        lastUpdate = currentMillis;
        int result = maintainTemperature(); 
        if (result == 1) {
          Serial.println("Box within 0.5 degree of target.");
        }
        else if (result == 0) {
          Serial.println("Temperature still changing.");
        }
        else {
          Serial.println("Voltaged changed. Monitoring temperature changes.");
        }
      }
      recentTemperatures.erase(recentTemperatures.begin()); // Remove first element
    }
  Serial.print("Current Voltage: "); Serial.println(currentVoltage);
  Serial.print("Sensor Deltas: "); Serial.println(maxSensor-minSensor);
  Serial.print("Target Temperature: "); Serial.println(target);
  Serial.print("Average Temperature: "); Serial.println(average);
  Serial.print("Delta: "); Serial.println(delta);

  std::vector<float> loraData;
  loraData.push_back(average);
  loraData.push_back(ambientTemperature);
  loraData.push_back(target);
  loraData.push_back(currentVoltage);
  loraData.push_back(temp1);
  loraData.push_back(temp2);
  loraData.push_back(temp3);
  loraData.push_back(temp4);
  sendLoraData(loraData);
  
  delay(10000); // Run every 15s
}


void sendLoraData(std::vector<float> dataToSend) {
  string combinedMessage = "1|";
  for (int i = 0; i < dataToSend.size(); i++) {
    string stringVersion = to_string(dataToSend[i]);
    size_t decimalPos = stringVersion.find('.');
    if (decimalPos != std::string::npos && decimalPos + 3 < stringVersion.length()) { // Truncate if there is a decimal
      stringVersion = stringVersion.substr(0, decimalPos + 3); // Truncate to 2 decimal
    }

    combinedMessage += stringVersion + "|";
  }

  lora.print("AT+SEND=9,"); lora.print(combinedMessage.size());
  lora.print(","); lora.println(combinedMessage.c_str());

}

int maintainTemperature() {
  // Preconditions: recentTemperatures is full
  // First, check if the temperature is constant at the given voltage
  // We'll allow a 0.75 F difference in range as constant
  float minElement = min(recentTemperatures[0], min(recentTemperatures[1], min(recentTemperatures[2], min(recentTemperatures[3], recentTemperatures[4]))));
  float maxElement = max(recentTemperatures[0], max(recentTemperatures[1], max(recentTemperatures[2], max(recentTemperatures[3], recentTemperatures[4]))));
  float range = maxElement - minElement;
  range = sqrt(range * range); //Abs value of range
  Serial.print("Recent Max Average: "); Serial.println(maxElement);
  Serial.print("Recent Min Average: "); Serial.println(minElement);
  Serial.print("Range between 5 most recent temp average: "); Serial.println(range);

  if (range > 0.40 ) return 0; // If above the range, temperature is still moving

  float currentAverage = recentTemperatures.back(); // Get most recent average
  float averageMinusTarget = currentAverage - target;
  Serial.print("Target: "); Serial.println(target);
  Serial.print("Current Average: "); Serial.println(currentAverage);
  Serial.print("Average - Target: "); Serial.println(averageMinusTarget);

  if (averageMinusTarget < 0.5 && averageMinusTarget > -0.5) { // Within 1 degree from target
    return 1;
  }
  else if (averageMinusTarget > 0.5) { // Temp has been constant, and average is still more than 1 degree larger than target
    changeVoltage(false, averageMinusTarget);
    return -1;
  }
  else if (averageMinusTarget < -0.5) { // Temp has been constant, and average is still more than 1 degree less than target
    changeVoltage(true, averageMinusTarget);
    return -1;
  }


  /* Return Codes
  0 -- Temperature not constant enough to warrant change
  1 -- Temperature within target temperature.
  -1 -- Voltage changed. Awaiting temperature change.
  */
}
void resetVoltageToZero() {
  float turnsNeeded = currentVoltage / 10;
  Serial.print("Turns needed to reset to 0: "); Serial.println(turnsNeeded);
  for (int i = 0; i < turnsNeeded; i++) {
    changeVoltage(false, 0);
    delay(5000);
  }
}

// y=0.188321x-0.0266254 -- x is steps per revolution, y is voltage change

void changeVoltage(bool increase, float changeTarget) {
  Serial.println("changeVoltage entered");
  float absChangeTarget = sqrt(changeTarget * changeTarget);
  if (absChangeTarget > 3.0) { // Need to change temperature by larger than 3 degrees
    stepsPerRevolution = 55; // About +-10 V
  }
  else if (absChangeTarget > 2.0) { // Need to change temperature by larger than 2 degrees
    stepsPerRevolution = 35; // About +-5 V
  }
  else if (absChangeTarget > 1) { // Need to change temperature by larger than 1 degree
    stepsPerRevolution = 25; // 
  }
  else { // // Need to change temperature by less than 2 degrees
    stepsPerRevolution = 15;
  }
  Serial.print("Steps Per Revolution: "); Serial.println(stepsPerRevolution);
  if (increase) {
    if (currentVoltage >= 115) {
      Serial.println("Current voltage too high. Cannot increase more. Returning");
      return;
    }
    Serial.println("Increasing voltage.");
    digitalWrite(dirPin, LOW);
    currentVoltage += (stepsPerRevolution*0.188321) - 0.0266254;
  }
  else {
    if (currentVoltage <= 10) {
      Serial.println("Current voltage too low. Cannot decrease more. Returning");
      return;
    }
    Serial.println("Decreasing voltage.");
    digitalWrite(dirPin, HIGH);
    currentVoltage -= (stepsPerRevolution*0.188321) - 0.0266254;
  }
  for (int i = 0; i < stepsPerRevolution; i++) {
    // These four lines result in 1 step:
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(5000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(5000);
  } // Need to determine increase in voltage for each revolution
}