#include <WiFi.h>
#include <MQTT.h>
#include <SoftwareSerial.h>

#define WIFI_SSID             "UniSZA-WiFi"
#define WIFI_PASSWORD         "unisza2016"
#define MQTT_HOST             "broker.hivemq.com"
#define MQTT_PREFIX_TOPIC     "MyReskill_IoT_60177875232/hibiscus"
#define MQTT_PUBLISH_TOPIC    "/soil"
#define MQTT_SUBSCRIBE_TOPIC  "/status"

#define RS485RX             18
#define RS485TX             19

#define sensorFrameSize     9
#define sensorWaitingTime   1000
#define sensorID            0x01
#define sensorFunction      0x03
#define sensorByteResponse  0x0E

SoftwareSerial sensor(RS485RX, RS485TX);

unsigned char byteRequest[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4,0x0B};
unsigned char byteResponse[9] = {};

float moisture, temperature;

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
  Serial.begin(115200);
  sensor.begin(4800);

  Serial.println("\nHibiscus Sense Soil Sensor & MQTT");

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

  sensor.write(byteRequest, 8);

  unsigned long resptime = millis();
  while ((sensor.available() < sensorFrameSize) && ((millis() - resptime) < sensorWaitingTime)) {
    delay(1);
  }

  while (sensor.available()) {
    for (int n = 0; n < sensorFrameSize; n++) {
      byteResponse[n] = sensor.read();
    }

    if (byteResponse[0] != sensorID && byteResponse[1] != sensorFunction && byteResponse[2] != sensorByteResponse) {
      
    }
  }

  moisture = sensorValue((int)byteResponse[3], (int)byteResponse[4]) * 0.1 ;
  temperature = sensorValue((int)byteResponse[5], (int)byteResponse[6]) * 0.1;
  
  Serial.println("Moisture: " + (String)moisture + " %");
  Serial.println("Temperature: " + (String)temperature + " °C");

  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

    String dataInJson = "{";
    dataInJson += "\"moisture\":" + String(moisture) + ",";
    dataInJson += "\"temperature\":" + String(temperature);
    dataInJson += "}";

    Serial.println("Data to Publish: " + dataInJson);
    Serial.println("Length of Data: " + String(dataInJson.length()));
    Serial.println("Publish to: " + String(MQTT_PREFIX_TOPIC) + String(MQTT_PUBLISH_TOPIC));
    
    mqtt.publish(String(MQTT_PREFIX_TOPIC) + String(MQTT_PUBLISH_TOPIC), dataInJson);
  }

  Serial.println();

  delay(1000);
}

int sensorValue(int x, int y) {
  int t = 0;
  t = x * 256;
  t = t + y;
  
  return t;
}
