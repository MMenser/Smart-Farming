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
WiFiClient client;


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
  printWifiStatus();

  ThingSpeak.begin(client);
  
  sensor1.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  sensor2.begin(MAX31865_3WIRE);
  sensor3.begin(MAX31865_3WIRE);
  sensor4.begin(MAX31865_3WIRE);
  sensor5.begin(MAX31865_3WIRE);
}


void loop() {
  // uint16_t rtd = thermo.readRTD();

  // Serial.print("RTD value: "); Serial.println(rtd);
  // float ratio = rtd;
  // ratio /= 32768;
  // Serial.print("Ratio = "); Serial.println(ratio,8);
  // Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
  // double celsius = thermo.temperature(RNOMINAL, RREF);
  // double farenheit = celsius * 1.8 + 32.0;
  // Serial.print("Temperature = "); Serial.println(farenheit);

  // float temp = thermo.temperature(RNOMINAL, RREF);
  // int success = ThingSpeak.writeField(2810004 ,1, temp, "YWD5EAS53P91TXJI");
  // Serial.print("Sucess = "); Serial.println(success);
  // if (success == 200) {
  //   Serial.println("Successfully written to ThingSpeak.");
  // }

  // delay(5000);
  float temp1 = sensor1.temperature(RNOMINAL, RREF);
  float temp2 = sensor2.temperature(RNOMINAL, RREF);
  float temp3 = sensor3.temperature(RNOMINAL, RREF);
  float temp4 = sensor4.temperature(RNOMINAL, RREF);
  float temp5 = sensor5.temperature(RNOMINAL, RREF);

  Serial.print("Sensor 1 Temp: "); Serial.print(temp1); Serial.println(" °C");
  Serial.print("Sensor 2 Temp: "); Serial.print(temp2); Serial.println(" °C");
  Serial.print("Sensor 3 Temp: "); Serial.print(temp3); Serial.println(" °C");
  Serial.print("Sensor 4 Temp: "); Serial.print(temp4); Serial.println(" °C");
  Serial.print("Sensor 5 Temp: "); Serial.print(temp5); Serial.println(" °C");
  Serial.println("----------------------");

  int data[5] = {temp1, temp2, temp3, temp4, temp5 };
  int success = writeSensorData(data);
  //int success = ThingSpeak.writeField(2810004, 5, data[2], "YWD5EAS53P91TXJI");

  delay(20000); // Read every second
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