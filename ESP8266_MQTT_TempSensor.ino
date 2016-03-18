#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <PubSubClient.h>

#define SECOND 1000
#define MINUTE SECOND * 60
#define HOUR MINUTE * 60

#define INTERVAL 20 * SECOND

/******************************************************************/
/*                         WIFI SETTINGS                          */
/******************************************************************/
#define wifi_ssid "zzr231"
#define wifi_password "W3B0ughtTh1sH0us31n2008!"

/******************************************************************/
/*                         MQTT SETTINGS                          */
/******************************************************************/
#define mqtt_server "192.168.0.23"

#define humidity_topic "sensor/humidity"
#define temperature_topic "sensor/temperature"

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_BME280 bme;

static boolean sensorInitialized = false;

void setup() {
 Serial.begin(115200);
 Serial.println("Setting up MQTT Temp Sensor...");
 setup_wifi();
 delay(1);
 client.setServer(mqtt_server, 1883);
 bme.begin();

}

void setup_wifi() {
 Serial.println("Starting Wifi Setup...");
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

long lastMsg = millis() - (INTERVAL + SECOND);
float temp = 0.0;
float hum = 0.0;
float diff = 1.0;

void loop() {
if (!client.connected()) {
   reconnect();
 }
 client.loop();

 long now = millis();
 if ( (now - lastMsg ) > INTERVAL ) {
  Serial.println("Starting send...");
   lastMsg = now;
   
   float newTemp = bme.readTemperature();
   float newHum = bme.readHumidity();

   if (checkBound(newTemp, temp, diff)) {
     temp = newTemp;
     Serial.print("New temperature:");
     Serial.println(String(temp).c_str());
     client.publish(temperature_topic, String(temp).c_str(), true);
   }

   if (checkBound(newHum, hum, diff)) {
     hum = newHum;
     Serial.print("New humidity:");
     Serial.println(String(hum).c_str());
     client.publish(humidity_topic, String(hum).c_str(), true);
   }
 }

}
