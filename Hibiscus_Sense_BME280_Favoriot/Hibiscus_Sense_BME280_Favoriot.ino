/*
* MYRESKILL IOT TRAINING
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
#include <HTTPClient.h>

#define WIFI_SSID             "NAMA_WIFI_ANDA_DISINI"
#define WIFI_PASSWORD         "PASSWORD_ANDA_DISINI"
#define DEVICE_DEVELOPER_ID   "DEVICE_DEVELOPER_ID_ANDA"
#define DEVICE_ACCESS_TOKEN   "DEVICE_ACCESS_TOKEN_ANDA"
#define FAVORIOT_REST_API     "http://apiv2.favoriot.com/v2/streams"

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;
float temperature, humidity, altitude, barometer;

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

void setup() {
  
  Serial.begin(9600);

  delay(2000);

  Serial.println("\nHibiscus Sense BME280 & Favoriot");

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

  Serial.println();

  readSensorBME();
  updateDataStream();

  Serial.println();

}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  readSensorBME();

  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

    updateDataStream();
  }

  Serial.println();

  delay(1000);
}

void readSensorBME(){
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  barometer = bme.readPressure() / 100.0F;
  
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
}

void updateDataStream(){
  String dataInJson = "{";
  dataInJson += "\"device_developer_id\":\"" + String(DEVICE_DEVELOPER_ID) + "\",";
  dataInJson += "\"data\": {";
  dataInJson += "\"temperature\":" + String(temperature) + ",";
  dataInJson += "\"humidity\":" + String(humidity) + ",";
  dataInJson += "\"altitude\":" + String(altitude) + ",";
  dataInJson += "\"barometer\":" + String(barometer);
  dataInJson += "}";
  dataInJson += "}";

  Serial.println("Update to Data Stream: " + dataInJson);
  
  HTTPClient http;

  http.begin(FAVORIOT_REST_API);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", DEVICE_ACCESS_TOKEN);
  
  int httpCode = http.POST(dataInJson);

  if(httpCode > 0){
    Serial.print("> HTTP Request: ");
    
    httpCode == 201 ? Serial.print("Success, ") : Serial.print("Error, ");
    Serial.println(http.getString());
  }
  else{
    Serial.println("> HTTP Request Connection Error!");
  }

  http.end();
}