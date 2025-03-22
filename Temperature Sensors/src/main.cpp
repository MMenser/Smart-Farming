#include <Adafruit_MAX31865.h>
#include <WiFiS3.h>
#include <ThingSpeak.h>

void printWifiStatus();
int writeSensorData(int data[5]);

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 sensor1 = Adafruit_MAX31865(9, 11, 12, 13); // CS pin 10 -- CS pins are what allows Arduino board to identify seperate devices
Adafruit_MAX31865 sensor2 = Adafruit_MAX31865(5, 11, 12, 13); 
Adafruit_MAX31865 sensor3 = Adafruit_MAX31865(4, 11, 12, 13); 
Adafruit_MAX31865 sensor4 = Adafruit_MAX31865(3, 11, 12, 13); 
Adafruit_MAX31865 sensor5 = Adafruit_MAX31865(2, 11, 12, 13); 
// use hardware SPI, just pass in the CS pin
// Adafruit_MAX31865 sensor1 = Adafruit_MAX31865(9); // CS pin 10
// Adafruit_MAX31865 sensor2 = Adafruit_MAX31865(5);  // CS pin 9 -- CS pins are what allows Arduino board to identify seperate devices

#define RREF      430.0
#define RNOMINAL  100.0

char ssid[] = "AVC";
char pass[] = "HoggleMan69";
int status = WL_IDLE_STATUS;
WiFiServer server(80);


void setup() {
  Serial.begin(9600);
  Serial.println("Adafruit MAX31865 PT100 Sensor Test!");

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed.");
    // don't continue
    while (true);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();
  printWifiStatus();

  //ThingSpeak.begin(client);
  
  sensor1.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  sensor2.begin(MAX31865_3WIRE);
  sensor3.begin(MAX31865_3WIRE);
  sensor4.begin(MAX31865_3WIRE);
  sensor5.begin(MAX31865_3WIRE);
}

unsigned long lastUpdate = 0;  // Stores last sensor update time
const unsigned long updateInterval = 15000;  // 15 seconds

float temp1, temp2, temp3, temp4, temp5;  // Store sensor values

void loop() {
  unsigned long currentMillis = millis();

  // Update sensor data every 15 seconds
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;  // Reset timer

    temp1 = sensor1.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp2 = sensor2.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp3 = sensor3.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp4 = sensor4.temperature(RNOMINAL, RREF) * 1.8 + 32;
    temp5 = sensor5.temperature(RNOMINAL, RREF) * 1.8 + 32;    

    Serial.println("Updated sensor readings:");
    Serial.print("Sensor 1: "); Serial.println(temp1);
    Serial.print("Sensor 2: "); Serial.println(temp2);
    Serial.print("Sensor 3: "); Serial.println(temp3);
    Serial.print("Sensor 4: "); Serial.println(temp4);
    Serial.print("Sensor 5: "); Serial.println(temp5);
    Serial.println("----------------------");
  }

  // Handle incoming HTTP requests continuously
  WiFiClient client = server.available();  
  if (client) {
    Serial.println("New client connected");

    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Send HTTP response headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println(F("<meta charset=\"UTF-8\">"));
            client.print(F("<p style=\"font-size:3vw;\">Sensor 1: ")); client.print(temp1);
            client.print(F("<p style=\"font-size:3vw;\">Sensor 2: ")); client.print(temp2);
            client.print(F("<p style=\"font-size:3vw;\">Sensor 3: ")); client.print(temp3);
            client.print(F("<p style=\"font-size:3vw;\">Sensor 4: ")); client.print(temp4);
            client.print(F("<p style=\"font-size:3vw;\">Sensor 5: ")); client.print(temp5);

          client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }
}


int writeSensorData(int data[5]) {
  for (int i = 0; i < 5; i++) {
    ThingSpeak.setField(i + 1, data[i]);
  }
  
  int success = ThingSpeak.writeFields(2810004, "YWD5EAS53P91TXJI");
  if (success != 0) {
    Serial.println("Error sending data");
    return -1;
  }
  
  Serial.println("Data sent successfully");
  return 0;
}


void printWifiStatus() {
/* -------------------------------------------------------------------------- */
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}