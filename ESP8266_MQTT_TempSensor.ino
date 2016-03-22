#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


/******************************************************************/
/*                         WIFI SETTINGS                          */
/******************************************************************/
#define wifi_ssid "zzr231"
#define wifi_password "W3B0ughtTh1sH0us31n2008!"

/******************************************************************/
/*                         MQTT SETTINGS                          */
/******************************************************************/
#define mqtt_server "192.168.0.23"

#define mqtt_topic "sensor/readings"

#define LOCATION "GREENHOUSE"
#define INTERVAL 20 * 60

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_BME280 bme;

extern "C" {
  ADC_MODE(ADC_VCC); //vcc read
}

static boolean sensorInitialized = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // Set initial state of LED off
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.begin(115200);
  Serial.println("Setting up MQTT Temp Sensor...");
  setup_wifi();
  delay(1);
  client.setServer(mqtt_server, 1883);
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!"); 
  }

}

void setup_wifi() {
 Serial.println("\r\nStarting Wifi Setup...");
 delay(10);
 // We start by connecting to a WiFi network
 Serial.println();
 Serial.print("Connecting to ");
 Serial.println(wifi_ssid);

 WiFi.begin(wifi_ssid, wifi_password);

 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }

 Serial.println("");
 Serial.println("WiFi connected");
 Serial.println("IP address: ");
 Serial.println(WiFi.localIP());
}

void reconnect() {
 // Loop until we're reconnected
 while (!client.connected()) {
   Serial.print("Attempting MQTT connection...");
   // Attempt to connect
   // If you do not want to use a username and password, change next line to
   // if (client.connect("ESP8266Client")) {
   if (client.connect("ESP8266Client")) {
     Serial.println("connected");
   } else {
     Serial.print("failed, rc=");
     Serial.print(client.state());
     Serial.println(" try again in 5 seconds");
     // Wait 5 seconds before retrying
     delay(500);
   }
 }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
 return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

//long lastMsg = millis() - (INTERVAL + SECOND);

DynamicJsonBuffer jsonBuffer;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  Serial.println("Starting send...");
  //lastMsg = now;

  float vdd = ESP.getVcc() / 1000.0;
  float newTemp = bme.readTemperature();
  float newHum = bme.readHumidity();

  String jsonResult = "";
  
  JsonObject& root = jsonBuffer.createObject();
  root["temperature"] = newTemp;
  root["humidity"] = newHum;
  root["location"] = LOCATION;
  root["vcc"] = vdd;  

  root.prettyPrintTo(jsonResult);
  
  Serial.print(jsonResult);
  
  client.publish(mqtt_topic, String(jsonResult).c_str(), true);

  Serial.println("\r\nGoing to sleep for " + String((int)INTERVAL) + " seconds...");
  ESP.deepSleep(INTERVAL * 1000000);
}
