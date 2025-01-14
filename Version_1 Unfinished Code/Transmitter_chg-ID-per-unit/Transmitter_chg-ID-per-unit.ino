#include <Adafruit_MAX31865.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

// MAX31865 Configurations
#define RREF      430.0
#define RNOMINAL  100.0

// Define Chip Select pins for each MAX31865 and SD card
#define SD_CS_PIN 4
#define CS_PINS {10, 6, 5, 9, 3, 2}

// Unique Identifier for the reader
const int uniqueID = 1;

// Array to hold MAX31865 objects for 6 sensors
Adafruit_MAX31865 max31865[6] = {
  Adafruit_MAX31865(10),
  Adafruit_MAX31865(6),
  Adafruit_MAX31865(5),
  Adafruit_MAX31865(9),
  Adafruit_MAX31865(3),
  Adafruit_MAX31865(2)
};

// RTC object
RTC_DS3231 rtc;

// SD card object
File dataFile;

void setup() {
  Serial.begin(9600);

  // Initialize MAX31865 sensors
  for (int i = 0; i < 6; i++) {
    if (!max31865[i].begin(MAX31865_3WIRE)) {
      Serial.print("Failed to initialize sensor ");
      Serial.println(i + 1);
      while (1);
    }
  }

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Initialization failed!");
    while (1);
  }

  // Open the file. Note that only one file can be open at a time,
  // so you have to close this one before opening another.
  dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println("UniqueID,Date,Time,Temp1,Temp2,Temp3,Temp4,Temp5,Temp6");
    dataFile.close();
  } else {
    Serial.println("Error opening datalog.csv");
  }
}

void loop() {
  // Array to store temperatures
  float temperatures[6];
  
  // Read temperatures from all sensors
  for (int i = 0; i < 6; i++) {
    temperatures[i] = max31865[i].temperature(RNOMINAL, RREF);
  }

  // Get current date and time
  DateTime now = rtc.now();

  // Create a string with all data
  String dataString = String(uniqueID) + ",";
  dataString += String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + ",";
  dataString += String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  for (int i = 0; i < 6; i++) {
    dataString += "," + String(temperatures[i]);
  }

  // Open the file, write the data, then close the file
  dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);  // Print to serial for debugging
  } else {
    Serial.println("Error opening datalog.csv");
  }
  Void loop()[
    Serial.write(45)
  ]
  // Wait 10 seconds before the next reading
  delay(10000);
}
