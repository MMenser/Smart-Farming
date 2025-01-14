#include <WiFiS3.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN   9
#define CSN_PIN 10

const byte thisSlaveAddress[5] = {'R', 'x', 'A', 'A', 'A'};

RF24 radio(CE_PIN, CSN_PIN);

const char* ssid = "136B Rowling Lane";
const char* password = "CXNK007D83F0";

const char* thingSpeakAPIKeys[] = {
    "U0YLURNIN6DG18XP",
    "BQU3YRENRZ0WNY4M",
    "5C7C2KVWI3KV64PD",
    "2WKWF2PBDE4PNZLO"
};

const char* server = "api.thingspeak.com";

WiFiClient client;

float dataPacket[7]; // Array to hold ID and 6 temperature values
bool newData = false;
float avgFirstFour;
float avgLastTwo;
void setup() {
    Serial.begin(9600);
    Serial.println("SimpleRx Starting");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    radio.begin();
    radio.setDataRate(RF24_250KBPS);
    radio.openReadingPipe(1, thisSlaveAddress);
    radio.startListening();
}

void loop() {
    getData();
    if (newData) {
        calculateAverages();
        showData();
        sendToThingSpeak((int)dataPacket[0]);
        newData = false;
    }
}

void getData() {
    if (radio.available()) {
        radio.read(&dataPacket, sizeof(dataPacket));
        newData = true;

        if ((int)dataPacket[0] < 1 || (int)dataPacket[0] > 4) {
            Serial.println("Invalid transmitter ID");
            newData = false;
        }
    }
}

void calculateAverages() {
    avgFirstFour = (dataPacket[1] + dataPacket[2] + dataPacket[3] + dataPacket[4]) / 4.0;
    avgLastTwo = (dataPacket[5] + dataPacket[6]) / 2.0;
}

void showData() {
    Serial.print("Data received: ID=");
    Serial.print((int)dataPacket[0]);
    Serial.print(" Values=");
    for (int i = 1; i < 7; i++) {
        Serial.print(dataPacket[i]);
        Serial.print(" ");
    }
    Serial.println();

    Serial.print("Average of first four temperatures: ");
    Serial.println(avgFirstFour);

    Serial.print("Average of last two temperatures: ");
    Serial.println(avgLastTwo);
}

void sendToThingSpeak(int transmitterID) {
    if (transmitterID < 1 || transmitterID > 4) {
        Serial.println("Invalid transmitter ID");
        return;
    }

    const char* thingSpeakAPIKey = thingSpeakAPIKeys[transmitterID - 1];

    if (client.connect(server, 80)) {
        String url = "/update?api_key=";
        url += thingSpeakAPIKey;
        url += "&field1=" + String(dataPacket[1]);
        url += "&field2=" + String(dataPacket[2]);
        url += "&field3=" + String(dataPacket[3]);
        url += "&field4=" + String(dataPacket[4]);
        url += "&field5=" + String(dataPacket[5]);
        url += "&field6=" + String(dataPacket[6]);
        url += "&field7=" + String(avgFirstFour);
        url += "&field8=" + String(avgLastTwo);

        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + server + "\r\n" +
                     "Connection: close\r\n\r\n");
        delay(100);
    }

    while (client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }

    Serial.println("Data sent to ThingSpeak");
}
