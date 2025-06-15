#include <SPI.h>
#include "SdFat.h"
#include <SoftwareSerial.h>

#define SPI_SPEED SD_SCK_MHZ(4)
#define SD_CS_PIN 10 // Change to match your wiring

void recieveMessage(String input);

SdExFat sd;
ExFile box1file;
ExFile box2file;
ExFile box3file;
ExFile box4file;
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

  // Replace pipes with commas for CSV format
  dataPart.replace('|', ',');
  // // Log to the correct file
  switch (sender)
  {
  case 9:
    if (!box1file.open("Box1.csv", FILE_WRITE))
    {
      Serial.println("Failed to open file!");
      return;
    }
    box1file.println(dataPart);
    box1file.flush();
    box1file.close();
    break;
  case 18:
    if (!box2file.open("Box2.csv", FILE_WRITE))
    {
      Serial.println("Failed to open file!");
      return;
    }
    box2file.println(dataPart);
    box2file.flush();
    box2file.close();
    break;
  case 27:
    if (!box3file.open("Box3.csv", FILE_WRITE))
    {
      Serial.println("Failed to open file!");
      return;
    }
    box3file.println(dataPart);
    box3file.flush();
    box3file.close();
    break;
  case 36:
    if (!box4file.open("Box4.csv", FILE_WRITE))
    {
      Serial.println("Failed to open file!");
      return;
    }
    box4file.println(dataPart);
    box4file.flush();
    box4file.close();
    break;
  default:
    Serial.println("Unknown box ID: " + String(sender));
    break;
  }

  Serial.println("Logged data from Box " + String(sender) + ": " + dataPart);
}
