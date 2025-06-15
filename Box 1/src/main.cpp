#include <Adafruit_MAX31865.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
using namespace std;

void changeVoltage(bool increase, float changeTarget);
int maintainTemperature();
void recieveLoraMessage();
void updateLCD();
void sendLoraData();
void sendSDInfo();
void sendSerialCSV(std::vector<float> dataToSend);
void printTemps();

#define boxID 3
#define dirPin 2
#define stepPin 3
#define RREF 430.0
#define RNOMINAL 100.0
#define startingVOLTAGE 54.0
#define buttonPin A3

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 ambientSensor = Adafruit_MAX31865(10, 11, 12, 13); // CS pin 10 -- CS pins are what allows Arduino board to identify seperate devices
Adafruit_MAX31865 sensor1 = Adafruit_MAX31865(9, 11, 12, 13); 
Adafruit_MAX31865 sensor2 = Adafruit_MAX31865(8, 11, 12, 13); 
Adafruit_MAX31865 sensor3 = Adafruit_MAX31865(7, 11, 12, 13); 
Adafruit_MAX31865 sensor4 = Adafruit_MAX31865(6, 11, 12, 13); 
SoftwareSerial lora(0,1);
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x3F for a 16 chars and 2 line display

int stepsPerRevolution = 0;
float currentVoltage = startingVOLTAGE;
std::vector<float> recentTemperatures;
std::vector<float> buttonInputs;
std::vector<float> recentAmbients;
float target = 0.0;

void setup()
{
  Serial.begin(9600);
  lora.begin(9600);
  sensor1.begin(MAX31865_3WIRE); // set to 2WIRE or 4WIRE as necessary
  ambientSensor.begin(MAX31865_3WIRE);
  sensor2.begin(MAX31865_3WIRE);
  sensor3.begin(MAX31865_3WIRE);
  sensor4.begin(MAX31865_3WIRE);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(dirPin, LOW);
  lora.setTimeout(500);
  lcd.init();
  updateLCD();
}

unsigned long lastUpdate = 0;               // Stores last sensor update time
unsigned long lastMaintainTemperature = 0;

unsigned long lastChangeVoltage = 0;
const unsigned long updateInterval = 15000; // 15 seconds

unsigned long lastLoraMessage = 0;           // Stores last time Lora sent a message
const unsigned long loraUpdateInterval = 60000; // Send every 15 minutes

unsigned long lastSDUpdate = 0;           // Stores last time Lora sent a message
const unsigned long sdWriteInterval = 600000; // Send every 10 minutes

float ambientTemperature, temp1, temp2, temp3, temp4, average, maxSensor, minSensor, averageAmbient; // Store sensor values
float delta = 10.0;

void updateLCD() {
  lcd.backlight();
  lcd.setCursor(1, 0); // Set cursor to character 2 on line 0
  lcd.print("Average: ");
  lcd.print(average);

  lcd.setCursor(2, 1); // Move cursor to character 2 on line 1
  lcd.print("Delta: ");
  lcd.print(delta);
}

void printTemps()
{
  Serial.println("Updated sensor readings:");
  Serial.print("Current Ambient Temperature: ");
  Serial.println(ambientTemperature);
  Serial.print("Average Ambient Temperature: ");
  Serial.println(averageAmbient);
  Serial.print("Sensor 1: ");
  Serial.println(temp1);
  Serial.print("Sensor 2: ");
  Serial.println(temp2);
  Serial.print("Sensor 3: ");
  Serial.println(temp3);
  Serial.print("Sensor 4: ");
  Serial.println(temp4);
  Serial.print("Current Voltage: ");
  Serial.println(currentVoltage);
  Serial.print("Sensor Deltas: ");
  Serial.println(maxSensor - minSensor);
  Serial.print("Target Temperature: ");
  Serial.println(target);
  Serial.print("Average Temperature: ");
  Serial.println(average);
  Serial.print("Delta: ");
  Serial.println(delta);
  Serial.println("----------------------");
}

void loop()
{
  unsigned long currentMillis = millis();
  // // Update sensor data every 15 seconds
  if (currentMillis - lastUpdate >= updateInterval)
  {
    lastUpdate = currentMillis; // Reset timer

    ambientTemperature = ambientSensor.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp1 = sensor1.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp2 = sensor2.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp3 = sensor3.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp4 = sensor4.temperature(RNOMINAL, RREF) * 1.8 + 32;

    average = (temp1 + temp2 + temp3 + temp4) / 4;
    target = ambientTemperature + delta; // Target is ambient + delta
    minSensor = min(temp1, min(temp2, min(temp3, temp4)));
    maxSensor = max(temp1, max(temp2, max(temp3, temp4)));

    recentTemperatures.push_back(average);
    recentAmbients.push_back(ambientTemperature);
    if (recentTemperatures.size() == 5 && recentAmbients.size() == 5) { // Vector has 5 elements, pop the least recent one 
      if (currentMillis - lastMaintainTemperature >= 60000 && currentMillis - lastChangeVoltage >= 120000) { 
        // Only run maintainTemperature() every minute, unless voltage was changed less than 2 minutes ago then wait for temp changes.
        lastMaintainTemperature = currentMillis;
        int result = maintainTemperature();
      }
      recentTemperatures.erase(recentTemperatures.begin()); // Remove first element
      recentAmbients.erase(recentAmbients.begin());
    }

    printTemps();
  }

  if (currentMillis - lastLoraMessage >= loraUpdateInterval) { // Every 1 minute
    lastLoraMessage = currentMillis;
    sendLoraData();
    updateLCD();
  }

  if (currentMillis - lastSDUpdate >= sdWriteInterval) {
    lastSDUpdate = currentMillis;
    sendSDInfo();
  }

  float buttonState = analogRead(buttonPin);
  buttonInputs.push_back(buttonState);
  if (buttonInputs.size() == 5) {
    float buttonAverage = (buttonInputs[0] + buttonInputs[1] + buttonInputs[2] + buttonInputs[3] + buttonInputs[4])/5;
    if (buttonAverage > 10 && buttonAverage < 14 ) { // Increase button pressed
      Serial.println("Button increased");
      delta++;
      updateLCD();
    }
    else if (buttonAverage > 18 && buttonAverage < 23) { // Decrease button presed
      Serial.println("Button decreased");
      delta--;
      updateLCD();
    }
    buttonInputs.erase(buttonInputs.begin());
  }

  
  if (lora.available()) {
    recieveLoraMessage();
  }
  delay(100);
}

void recieveLoraMessage() {
  String message = "";
  message = lora.readString();
  if (message.length() < 6) {
    return;
  }

  // Assume message always looks like: +RCV=9,2,D5,-34,11
  message = message.substring(9);
  int commaIndex = message.indexOf(',');
  message = message.substring(0, commaIndex);
  char changeParamter = message[0]; // D for Delta, V for Voltage
  String value = message.substring(1);
  Serial.print("Message: "); Serial.println(message);
  Serial.print("Change Parameter: "); Serial.println(changeParamter);
  if (changeParamter == 'D') {
    delta = atof(value.c_str());
    Serial.println("Delta updated.");
  }
  else if (changeParamter == 'V') {
    currentVoltage = atoi(value.c_str());
    Serial.println("Voltage updated.");
  }
  else {
    Serial.println("Unknown change parameter/value.");
  }

  return;
}


void sendLoraData()
{
  std::vector<float> dataToSend = {average, averageAmbient, delta, currentVoltage, temp1, temp2, temp3, temp4};
  string combinedMessage = "1|"; // NEED TO CHANGE FOR EACH BOX
  for (int i = 0; i < dataToSend.size(); i++)
  {
    if (i == 2) {
      combinedMessage += to_string(int(delta)) + "|";
    }
    else {
      string stringVersion = to_string(dataToSend[i]);
      size_t decimalPos = stringVersion.find('.');
      if (decimalPos != std::string::npos && decimalPos + 3 < stringVersion.length())
      {                                                          // Truncate if there is a decimal
        stringVersion = stringVersion.substr(0, decimalPos + 3); // Truncate to 2 decimal
      }
  
      combinedMessage += stringVersion + "|";
    }
  }

  String loraCommand = "AT+SEND=9," + String(combinedMessage.length()) + "," + combinedMessage.c_str();
  Serial.print("Sending: "); Serial.println(loraCommand);
  lora.println(loraCommand);
  delay(100);
}

void sendSDInfo()
{
  std::vector<float> dataToSend = {average, averageAmbient, delta, currentVoltage, temp1, temp2, temp3, temp4};
  string combinedMessage = "1|"; // NEED TO CHANGE FOR EACH BOX
  for (int i = 0; i < dataToSend.size(); i++)
  {
    if (i == 2) {
      combinedMessage += to_string(int(delta)) + "|";
    }
    else {
      string stringVersion = to_string(dataToSend[i]);
      size_t decimalPos = stringVersion.find('.');
      if (decimalPos != std::string::npos && decimalPos + 3 < stringVersion.length())
      {                                                          // Truncate if there is a decimal
        stringVersion = stringVersion.substr(0, decimalPos + 3); // Truncate to 2 decimal
      }
  
      combinedMessage += stringVersion + "|";
    }
  }

  String loraCommand = "AT+SEND=100," + String(combinedMessage.length()) + "," + combinedMessage.c_str();
  Serial.print("Sending: "); Serial.println(loraCommand);
  lora.println(loraCommand);
  delay(100);
}



int maintainTemperature()
{
  // Preconditions: recentTemperatures is full
  // First, check if the temperature is constant at the given voltage
  // We'll allow a 0.75 F difference in range as constant
  float minElement = min(recentTemperatures[0], min(recentTemperatures[1], min(recentTemperatures[2], min(recentTemperatures[3], recentTemperatures[4]))));
  float maxElement = max(recentTemperatures[0], max(recentTemperatures[1], max(recentTemperatures[2], max(recentTemperatures[3], recentTemperatures[4]))));
  float range = maxElement - minElement;
  range = sqrt(range * range); // Abs value of range
  Serial.print("Recent Max Average: ");
  Serial.println(maxElement);
  Serial.print("Recent Min Average: ");
  Serial.println(minElement);
  Serial.print("Range between 5 most recent temp average: ");
  Serial.println(range);

  averageAmbient = (recentAmbients[0] + recentAmbients[1] + recentAmbients[2] + recentAmbients[3] + recentAmbients[4]) / 5;
  float currentAverage = recentTemperatures.back(); // Get most recent average
  float averageMinusTarget = currentAverage - (delta + averageAmbient);
  Serial.print("Target: ");
  Serial.println(target);
  Serial.print("Current Average: ");
  Serial.println(currentAverage);
  Serial.print("Average - Target: ");
  Serial.println(averageMinusTarget);

  if (averageMinusTarget < 0.5 && averageMinusTarget > -0.5)
  { // Within 1 degree from target
    Serial.println("Box within 0.5 degree of target.");
    return 1;
  }
  else if (averageMinusTarget > 0.5)
  { // Temp has been constant, and average is still more than 1 degree larger than target
    changeVoltage(false, averageMinusTarget);
    return -1;
  }
  else if (averageMinusTarget < -0.5)
  { // Temp has been constant, and average is still more than 1 degree less than target
    changeVoltage(true, averageMinusTarget);
    return -1;
  }

  /* Return Codes
  0 -- Temperature not constant enough to warrant change
  1 -- Temperature within target temperature.
  -1 -- Voltage changed. Awaiting temperature change.
  */
}

// y=0.188321x-0.0266254 -- x is steps per revolution, y is voltage change

void changeVoltage(bool increase, float changeTarget)
{
  lastChangeVoltage = millis();
  float averageVoltageChange = 0;
  float percentVoltageReduction = 0.70; // Variac is harder to turn at higher voltage. Reduce expected voltage chnage if at higher voltage.

  float absChangeTarget = sqrt(changeTarget * changeTarget);
  if (absChangeTarget > 3.0)
  {                          // Need to change temperature by larger than 3 degrees
    stepsPerRevolution = 60; 
    averageVoltageChange = 10.0;
  }
  else if (absChangeTarget > 2.0)
  {                          // Need to change temperature by larger than 2 degrees
    stepsPerRevolution = 35; 
    averageVoltageChange = 6.0;
  }
  else if (absChangeTarget > 1)
  {                          // Need to change temperature by larger than 1 degree
    stepsPerRevolution = 25;
    averageVoltageChange = 4.0;
  }
  else
  { // // Need to change temperature by less than 2 degrees
    stepsPerRevolution = 15;
    averageVoltageChange = 2.0;
  }
  Serial.print("Steps Per Revolution: ");
  Serial.println(stepsPerRevolution);
  if (increase)
  {
    if (currentVoltage >= 115)
    {
      Serial.println("Current voltage too high. Cannot increase more. Returning");
      return;
    }
    Serial.println("Increasing voltage.");
    digitalWrite(dirPin, LOW);
    if (currentVoltage > 76) {
      Serial.println("Voltage above 76.");
      currentVoltage += averageVoltageChange * percentVoltageReduction;
    }
    else {
      currentVoltage += averageVoltageChange;
    }
  }
  else
  {
    if (currentVoltage <= 10)
    {
      Serial.println("Current voltage too low. Cannot decrease more. Returning");
      return;
    }
    Serial.println("Decreasing voltage.");
    digitalWrite(dirPin, HIGH);
    if (currentVoltage > 76) {
      Serial.println("Voltage above 76.");
      currentVoltage -= averageVoltageChange * percentVoltageReduction;
    }
    else {
      currentVoltage -= averageVoltageChange;
    }

  }
  for (int i = 0; i < stepsPerRevolution; i++)
  {
    // These four lines result in 1 step:
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(5000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(5000);
  } // Need to determine increase in voltage for each revolution
}