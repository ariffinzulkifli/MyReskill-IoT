/*
* PROGRAM MYRESKILL IOT 
* ANJURAN MPC
* 
* Penulis: Mohamad Ariffin Zulkifli
* Syarikat: Myinvent Technologies Sdn Bhd
*
* Mikropengawal ESP32 hanya menyokong Wi-Fi berfrekuensi 2.4GHz.
* Oleh itu, masukkan nama Wi-Fi yang berfrekuensi 2.4GHz sahaja.
* 
*/

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <WiFi.h>
#include <MQTT.h>

#define WIFI_SSID             "NAMA_WIFI_ANDA_DISINI"
#define WIFI_PASSWORD         "PASSWORD_ANDA_DISINI"
#define MQTT_HOST             "broker.hivemq.com"
#define MQTT_PREFIX_TOPIC     "MyReskill_IoT_NOMBOR_TELEFON_ANDA_DISINI/hibiscus"
#define MQTT_PUBLISH_TOPIC    "/data"
#define MQTT_SUBSCRIBE_TOPIC  "/control"

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

WiFiClient net;
MQTTClient mqtt(1024);

unsigned long lastMillis = 0;

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi '" + String(WIFI_SSID) + "' ...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println(" connected!");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("Incoming Status: " + payload);
  Serial.println();
}

void connectToMqttBroker(){
  Serial.print("Connecting to '" + String(MQTT_HOST) + "' ...");
  
  mqtt.begin(MQTT_HOST, net);
  mqtt.onMessage(messageReceived);

  String uniqueString = String(WIFI_SSID) + "-" + String(random(1, 98)) + String(random(99, 999));
  char uniqueClientID[uniqueString.length() + 1];
  
  uniqueString.toCharArray(uniqueClientID, uniqueString.length() + 1);
  
  while (!mqtt.connect(uniqueClientID)) {
    Serial.print(".");
    delay(500);
  }

  Serial.println(" connected!");

  Serial.println("Subscribe to: " + String(MQTT_PREFIX_TOPIC) + String(MQTT_SUBSCRIBE_TOPIC));
  
  mqtt.subscribe(String(MQTT_PREFIX_TOPIC) + String(MQTT_SUBSCRIBE_TOPIC));

}

void setup() {
  
  Serial.begin(9600);

  delay(2000);

  Serial.println("\nHibiscus Sense BME280 & MQTT");

  unsigned status = bme.begin();
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  connectToWiFi();
  connectToMqttBroker();

  Serial.println();

}

void loop() {
  mqtt.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if (!mqtt.connected()) {
    connectToMqttBroker();
  }

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float barometer = bme.readPressure() / 100.0F;
  
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println(" %RH");

  Serial.print("Approx. Altitude = ");
  Serial.print(altitude);
  Serial.println(" m");

  Serial.print("Barometer = ");
  Serial.print(barometer);
  Serial.println(" hPa");

  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

    String dataInJson = "{";
    dataInJson += "\"temperature\":" + String(temperature) + ",";
    dataInJson += "\"humidity\":" + String(humidity) + ",";
    dataInJson += "\"altitude\":" + String(altitude) + ",";
    dataInJson += "\"barometer\":" + String(barometer);
    dataInJson += "}";

    Serial.println("Data to Publish: " + dataInJson);
    Serial.println("Length of Data: " + String(dataInJson.length()));
    Serial.println("Publish to: " + String(MQTT_PREFIX_TOPIC) + String(MQTT_PUBLISH_TOPIC));
    
    mqtt.publish(String(MQTT_PREFIX_TOPIC) + String(MQTT_PUBLISH_TOPIC), dataInJson);
  }

  Serial.println();

  delay(1000);
}
