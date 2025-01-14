#include <WiFiS3.h>
#include <SoftwareSerial.h>

#define LORA_RX_PIN 8
#define LORA_TX_PIN 7

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
SoftwareSerial loraSerial(LORA_RX_PIN, LORA_TX_PIN);

float dataPacket[7]; // Array to hold ID and 6 temperature values
bool newData = false;
float avgFirstFour;
float avgLastTwo;

void setup() {
    Serial.begin(9600);
    loraSerial.begin(115200);  // Use the baud rate from the provided example
    Serial.println("Final Receiver Node Starting");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    configureLoRa();
}

void loop() {
    checkSerial();
    if (newData) {
        calculateAverages();
        showData();
        sendToThingSpeak((int)dataPacket[0]);
        newData = false;
    } else {
        Serial.println("No new data received.");
    }
    delay(5000); // Add a delay to reduce the frequency of the "No new data" message
}

void checkSerial() {
    while (loraSerial.available() > 0) {
        String inString = loraSerial.readString();
        Serial.print("Received Data : ");
        Serial.println(inString);
        String DataIn = getValue(inString, ',', 2);  // Extract the actual data part
        parseData(DataIn);
        newData = true;
    }
}

String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void parseData(String receivedData) {
    int index = 0;
    int startIndex = 0;
    int endIndex = receivedData.indexOf(' ');

    while (endIndex != -1) {
        dataPacket[index] = receivedData.substring(startIndex, endIndex).toFloat();
        startIndex = endIndex + 1;
        endIndex = receivedData.indexOf(' ', startIndex);
        index++;
    }
    dataPacket[index] = receivedData.substring(startIndex).toFloat();
}

void calculateAverages() {
    avgFirstFour = (dataPacket[1] + dataPacket[2] + dataPacket[3] + dataPacket[4]) / 4.0;
    avgLastTwo = (dataPacket[5] + dataPacket[6]) / 2.0;
}

void showData() {
    Serial.print("Data received: Transmitter ID=");
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

void configureLoRa() {
    String lora_band = "915000000";
    String lora_networkid = "3";
    String lora_RX_address = "666";

    loraSerial.println("AT+BAND=" + lora_band);
    delay(1000);
    printResponse();

    loraSerial.println("AT+ADDRESS=" + lora_RX_address);
    delay(1000);
    printResponse();

    loraSerial.println("AT+NETWORKID=" + lora_networkid);
    delay(1000);
    printResponse();

    loraSerial.println("AT+IPR=9600");
    delay(1000);
    printResponse();
    
    loraSerial.println("AT+RX");
    delay(1000);
    printResponse();
}

void printResponse() {
    while (loraSerial.available()) {
        String response = loraSerial.readString();
        Serial.println(response);
    }
}
