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

#include <WiFi.h>
#include <MQTT.h>
#include <SoftwareSerial.h>

#define WIFI_SSID             "NAMA_WIFI_ANDA_DISINI"
#define WIFI_PASSWORD         "PASSWORD_ANDA_DISINI"
#define MQTT_HOST             "broker.hivemq.com"
#define MQTT_PREFIX_TOPIC     "MyReskill_IoT_NOMBOR_TELEFON_ANDA_DISINI/hibiscus"
#define MQTT_PUBLISH_TOPIC    "/data"
#define MQTT_SUBSCRIBE_TOPIC  "/control"

#define RS485RX             18
#define RS485TX             19

SoftwareSerial relay(RS485RX, RS485TX);

unsigned char turnOnCH1[8] = {0xFF, 0x05, 0x00, 0x00, 0xFF, 0x00, 0x99, 0xE4};
unsigned char turnOffCH1[8] = {0xFF, 0x05, 0x00, 0x00, 0x00, 0x00, 0xD8, 0x14};
unsigned char turnOnCH2[8] = {0xFF, 0x05, 0x00, 0x01, 0xFF, 0x00, 0xC8, 0x24};
unsigned char turnOffCH2[8] = {0xFF, 0x05, 0x00, 0x01, 0x00, 0x00, 0x89, 0xD4};

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

  if(payload == "CH1ON"){
    relay.write(turnOnCH1, 8);
  }
  
  if(payload == "CH1OFF"){
    relay.write(turnOffCH1, 8);
  }

  if(payload == "CH2ON"){
    relay.write(turnOnCH2, 8);
  }
  
  if(payload == "CH2OFF"){
    relay.write(turnOffCH2, 8);
  }
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
  Serial.begin(115200);
  relay.begin(9600);

  Serial.println("\nHibiscus Sense Relay & MQTT");

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
}
