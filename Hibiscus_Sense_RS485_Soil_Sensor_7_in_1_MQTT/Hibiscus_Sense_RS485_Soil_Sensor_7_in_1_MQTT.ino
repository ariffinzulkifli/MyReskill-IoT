#include <WiFi.h>
#include <MQTT.h>
#include <SoftwareSerial.h>

#define WIFI_SSID             "NAMA_WIFI_ANDA_DISINI"
#define WIFI_PASSWORD         "PASSWORD_ANDA_DISINI"
#define MQTT_HOST             "broker.hivemq.com"
#define MQTT_PREFIX_TOPIC     "MyReskill_IoT_NOMBOR_TELEFON_ANDA_DISINI/hibiscus"
#define MQTT_PUBLISH_TOPIC    "/data"
#define MQTT_SUBSCRIBE_TOPIC  "/control"

#define RS485RX               18
#define RS485TX               19

#define sensorFrameSize       19
#define sensorWaitingTime     1000
#define sensorID              0x01
#define sensorFunction        0x03
#define sensorByteResponse    0x0E

SoftwareSerial sensor(RS485RX, RS485TX);

unsigned char byteRequest[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};
unsigned char byteResponse[19] = {};

float moisture, temperature, ph, ec, nitrogen, phosphorus, potassium;

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

  Serial.println("\nHibiscus Sense Soil Multi Parameter Sensor & MQTT");

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
  ec = sensorValue((int)byteResponse[7], (int)byteResponse[8]);
  ph = sensorValue((int)byteResponse[9], (int)byteResponse[10]) * 0.1;
  nitrogen = sensorValue((int)byteResponse[11], (int)byteResponse[12]);
  phosphorus = sensorValue((int)byteResponse[13], (int)byteResponse[14]);
  potassium = sensorValue((int)byteResponse[15], (int)byteResponse[16]);

  Serial.println("Moisture: " + (String)moisture + " %");
  Serial.println("Temperature: " + (String)temperature + " °C");
  Serial.println("pH: " + (String)ph);
  Serial.println("EC: " + (String)ec + " uS/cm");
  Serial.println("Nitrogen (N): " + (String)nitrogen + " mg/kg");
  Serial.println("Phosporus (P): " + (String)phosphorus + " mg/kg");
  Serial.println("Potassium (K): " + (String)potassium + " mg/kg");

  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

    String dataInJson = "{";
    dataInJson += "\"moisture\":" + String(moisture) + ",";
    dataInJson += "\"temperature\":" + String(temperature) + ",";
    dataInJson += "\"ph\":" + String(ph) + ",";
    dataInJson += "\"ec\":" + String(ec) + ",";
    dataInJson += "\"nitrogen\":" + String(nitrogen) + ",";
    dataInJson += "\"phosphorus\":" + String(phosphorus) + ",";
    dataInJson += "\"potassium\":" + String(potassium);
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
