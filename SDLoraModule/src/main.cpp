#include <SPI.h>
#include "SdFat.h"
#include <SoftwareSerial.h>

#define SPI_SPEED SD_SCK_MHZ(4)
#define SD_CS_PIN 10 // Change to match your wiring

void recieveMessage(String input);

SdExFat sd;
SoftwareSerial lora(0, 1);

void setup()
{
  Serial.begin(9600);
  lora.begin(9600);
  while (!Serial)
    yield(); // Wait for Serial

  lora.setTimeout(500);
  if (!sd.begin(SD_CS_PIN, SPI_SPEED))
  {
    Serial.println("SD init failed!");
    return;
  }
}

void loop()
{
  if (lora.available()) {
    recieveMessage();
  }
}
// RCV=36,48,4|87.09|81.39|10|107.20|83.20|85.56|89.86|89.71|,-91,-2
void recieveMessage()
{
  String message = lora.readStringUntil('\n');
  Serial.println(message);
  message.trim();

  if (!message.startsWith("RCV="))
    return;

  int firstPipe = message.indexOf('|');
  if (firstPipe == -1)
    return;

  // Get sender address
  int equalIndex = message.indexOf('=');
  int firstCommaIndex = message.indexOf(',');
  String senderString = message.substring(equalIndex + 1, firstCommaIndex);
  int sender = senderString.toInt();

  // Get the data values
  String dataPart = message.substring(firstPipe + 1);
  int lastComma = dataPart.lastIndexOf(',');
  dataPart = dataPart.substring(0, lastComma); // Remove RSSI/SNR
  dataPart.replace('|', ','); // CSV format

  // Add timestamp (ms since boot)
  String logLine = String(millis()) + "," + dataPart;

  // Log to the correct file
  ExFile file;
  String filename;

  switch (sender)
  {
    case 9:  filename = "Box1.csv"; break;
    case 18: filename = "Box2.csv"; break;
    case 27: filename = "Box3.csv"; break;
    case 36: filename = "Box4.csv"; break;
    default:
      Serial.println("Unknown box ID: " + String(sender));
      return;
  }

  if (!file.open(filename.c_str(), FILE_WRITE))
  {
    Serial.println("Failed to open file!");
    return;
  }

  file.println(logLine);
  file.flush();
  file.close();

  Serial.println("Logged data from Box " + String(sender) + ": " + logLine);
}
