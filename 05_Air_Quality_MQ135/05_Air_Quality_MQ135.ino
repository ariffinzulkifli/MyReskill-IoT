#include <MQUnifiedsensor.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <MQTT.h>

#define BOARD       "ESP-32"
#define VOLTAGE     3.3
#define PIN         34
#define TYPE        "MQ-135"
#define ADC_BIT     12
#define RATIO_MQ135 3.6 // RS/R0 = 3.6 ppm
#define RGB_PIN     16
#define RGB_PIXELS  1

MQUnifiedsensor MQ135(BOARD, VOLTAGE, ADC_BIT, PIN, TYPE);
Adafruit_NeoPixel RGB(RGB_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

#define WIFI_SSID             "NAMA_WIFI_ANDA_DISINI"
#define WIFI_PASSWORD         "PASSWORD_ANDA_DISINI"
#define MQTT_HOST             "broker.hivemq.com"
#define MQTT_PREFIX_TOPIC     "MyReskill_IoT_NOMBOR_TELEFON_ANDA_DISINI/hibiscus"
#define MQTT_PUBLISH_TOPIC    "/data"
#define MQTT_SUBSCRIBE_TOPIC  "/control"

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

  delay(2000);

  Serial.println("\nHibiscus Sense MQ135 & MQTT");

  RGB.begin();

  connectToWiFi();
  connectToMqttBroker();

  MQ135.setRegressionMethod(1); // ppm =  a*ratio^b
  MQ135.init();
  
  Serial.print("Calibrating the sensor, please wait ");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++) {
    MQ135.update(); // Update data, the microcontroller will read the voltage from the analog pin
    calcR0 += MQ135.calibrate(RATIO_MQ135);
    Serial.print(".");
  }
  MQ135.setR0(calcR0/10);
  Serial.println(" done!");
  
  if(isinf(calcR0)) {
    Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    while(1);
  }
  
  if(calcR0 == 0){
    Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    while(1);
  }

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
  
  MQ135.update();

  /*
    Exponential Regression:
    
    GAS     | a      | b
    CO      | 605.18 | -3.937  
    Alcohol | 77.255 | -3.18 
    CO2     | 110.47 | -2.862
    Toluene | 44.947 | -3.445
    NH4     | 102.2  | -2.473
    Acetone | 34.668 | -3.369
  */

  MQ135.setA(605.18); MQ135.setB(-3.937);
  float CO = MQ135.readSensor();

  MQ135.setA(77.255); MQ135.setB(-3.18);
  float Alcohol = MQ135.readSensor();

  MQ135.setA(110.47); MQ135.setB(-2.862);
  float CO2 = MQ135.readSensor();
  
  MQ135.setA(44.947); MQ135.setB(-3.445);
  float Toluene = MQ135.readSensor();
  
  MQ135.setA(102.2 ); MQ135.setB(-2.473);
  float NH4 = MQ135.readSensor();
  
  MQ135.setA(34.668); MQ135.setB(-3.369);
  float Acetone = MQ135.readSensor();
  
  Serial.println("Carbon Monoxide (CO): " + String(CO) + " ppm"); 
  Serial.println("Alcohol: " + String(Alcohol) + " ppm");
  Serial.println("Carbon Dioxide (CO2): " + String(CO2 + 400) + " ppm"); 
  Serial.println("Toluene: " + String(Toluene) + " ppm"); 
  Serial.println("Ammonia (NH4): " + String(NH4) + " ppm"); 
  Serial.println("Acetone: " + String(Acetone) + " ppm");
  
  if(Toluene < 3.0) {
    RGB.setPixelColor(0, RGB.Color(0, 0, 0));
    RGB.show();
  }
  else {
    RGB.setPixelColor(0, RGB.Color(150, 0, 0));
    RGB.show();
  }

  if (millis() - lastMillis > 60000) {
    lastMillis = millis();

    String dataInJson = "{";
    dataInJson += "\"carbon_monoxide\":" + String(CO) + ",";
    dataInJson += "\"alcohol\":" + String(Alcohol) + ",";
    dataInJson += "\"carbon_dioxide\":" + String(CO2 + 400) + ",";
    dataInJson += "\"toluene\":" + String(Toluene) + ",";
    dataInJson += "\"ammonia\":" + String(NH4) + ",";
    dataInJson += "\"acetone\":" + String(Acetone);
    dataInJson += "}";

    Serial.println("Data to Publish: " + dataInJson);
    Serial.println("Length of Data: " + String(dataInJson.length()));
    Serial.println("Publish to: " + String(MQTT_PREFIX_TOPIC) + String(MQTT_PUBLISH_TOPIC));
    
    mqtt.publish(String(MQTT_PREFIX_TOPIC) + String(MQTT_PUBLISH_TOPIC), dataInJson);
  }

  Serial.println();

  delay(100);
}
