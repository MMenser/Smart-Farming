#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SD.h>
#include <SoftwareSerial.h>

#define CE_PIN   9
#define CSN_PIN 10
#define SD_CS_PIN 4 // SD card chip select pin
#define LORA_RX_PIN 8
#define LORA_TX_PIN 7

const byte thisSlaveAddress[5] = {'R', 'x', 'A', 'A', 'A'};

RF24 radio(CE_PIN, CSN_PIN);
SoftwareSerial loraSerial(LORA_RX_PIN, LORA_TX_PIN);
RTC_DS3231 rtc;

float dataPacket[7]; // Array to hold ID and 6 temperature values
bool newData = false;
File dataFile; // Declare the dataFile globally

// Duty cycle management variables
unsigned long lastTransmitTime = 0;
unsigned long transmitDuration = 0;
const unsigned long dutyCyclePeriod = 100000; // 100 seconds in milliseconds
const unsigned long maxTransmitTime = 1000;   // 1% of 100 seconds

void setup() {
    Serial.begin(9600);
    Serial.println("Intermediary Node Starting");

    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        while (1); // Halt execution if SD card initialization fails
    }

    if (!rtc.begin() || rtc.lostPower()) {
        Serial.println("RTC initialization failed or RTC lost power!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    radio.begin();
    radio.setDataRate(RF24_250KBPS);
    radio.openReadingPipe(1, thisSlaveAddress);
    radio.startListening();

    loraSerial.begin(115200); // Match the baud rate from your example
    configureLoRa();
}

void loop() {
    if (radio.available()) {
        radio.read(&dataPacket, sizeof(dataPacket));
        newData = (dataPacket[0] >= 1 && dataPacket[0] <= 4);
        if (newData) {
            saveToSD();
            if (canTransmit()) {
                unsigned long startTransmitTime = millis();
                sendToLoRa();
                unsigned long endTransmitTime = millis();
                updateDutyCycle(endTransmitTime - startTransmitTime);
            } else {
                Serial.println("Duty cycle limit reached, waiting...");
            }
        } else {
            Serial.println("Invalid transmitter ID");
        }
    }
}

void saveToSD() {
    int transmitterID = (int)dataPacket[0];
    String fileName = "transmitter_" + String(transmitterID) + ".csv";
    dataFile = SD.open(fileName.c_str(), FILE_WRITE);
    if (dataFile) {
        DateTime now = rtc.now();
        dataFile.print(now.timestamp());
        for (int i = 1; i < 7; i++) {
            dataFile.print(",");
            dataFile.print(dataPacket[i]);
        }
        dataFile.println();
        dataFile.close();
        Serial.println("Data saved to SD card");
    } else {
        Serial.println("Error opening file");
    }
}

void sendToLoRa() {
    String data = String((int)dataPacket[0]) + " ";
    for (int i = 1; i < 7; i++) {
        data += String(dataPacket[i]);
        if (i < 6) data += " ";
    }
    String command = "AT+SEND=666," + String(data.length()) + "," + data;
    loraSerial.print(command); // Use print instead of println
    Serial.println("Data sent to LoRa: " + command);
}

void configureLoRa() {
    sendLoRaCommand("AT+BAND=915000000");
    sendLoRaCommand("AT+ADDRESS=999");
    sendLoRaCommand("AT+NETWORKID=3");
    sendLoRaCommand("AT+IPR=115200");
}

void sendLoRaCommand(const String& command) {
    loraSerial.println(command);
    delay(1000);
    while (loraSerial.available()) {
        Serial.println(loraSerial.readString());
    }
}

bool canTransmit() {
    unsigned long currentTime = millis();
    if (currentTime - lastTransmitTime >= dutyCyclePeriod) {
        transmitDuration = 0;
        lastTransmitTime = currentTime;
    }
    return (transmitDuration < maxTransmitTime);
}

void updateDutyCycle(unsigned long transmissionTime) {
    transmitDuration += transmissionTime;
}
