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
#include <Adafruit_APDS9960.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
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

Adafruit_APDS9960 apds;
Adafruit_BME280 bme;
Adafruit_MPU6050 mpu;

sensors_event_t a, g, temp;

Adafruit_NeoPixel pixels(1, 16, NEO_GRB + NEO_KHZ800);

int freq = 2000;
int channel = 0;
int resolution = 8;

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

  pixels.begin();
  pinMode(2, OUTPUT);

  ledcSetup(channel, freq, resolution);
  ledcAttachPin(13, channel);

  Serial.begin(115200);
  
  if (!apds.begin()) Serial.println("Failed to find APDS chip");
  if (!bme.begin()) Serial.println("Failed to find BME chip");
  if (!mpu.begin()) Serial.println("Failed to find MPU6050 chip");

  apds.enableProximity(true);

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  connectToWiFi();
  connectToMqttBroker();

  Serial.println();

  pixels.setPixelColor(0, pixels.Color(0, 0, 0)); pixels.show();

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

  uint8_t proximity = apds.readProximity();

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float barometer = bme.readPressure() / 100.0F;

  float x_axis = a.acceleration.x;
  float y_axis = a.acceleration.y;
  float z_axis = a.acceleration.z;

  float x_gyro = g.gyro.x;
  float y_gyro = g.gyro.y;
  float z_gyro = g.gyro.z;

  Serial.print("Proximity: ");
  Serial.println(proximity);
  
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

  Serial.print("Acceleration X: ");
  Serial.print(x_axis);
  Serial.print(", Y: ");
  Serial.print(y_axis);
  Serial.print(", Z: ");
  Serial.print(z_axis);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(x_gyro);
  Serial.print(", Y: ");
  Serial.print(y_gyro);
  Serial.print(", Z: ");
  Serial.print(z_gyro);
  Serial.println(" rad/s");

  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

    String dataInJson = "{";
    dataInJson += "\"proximity\":" + String(proximity) + ",";
    dataInJson += "\"temperature\":" + String(temperature) + ",";
    dataInJson += "\"humidity\":" + String(humidity) + ",";
    dataInJson += "\"altitude\":" + String(altitude) + ",";
    dataInJson += "\"x_axis\":" + String(x_axis) + ",";
    dataInJson += "\"y_axis\":" + String(y_axis) + ",";
    dataInJson += "\"z_axis\":" + String(z_axis) + ",";
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
