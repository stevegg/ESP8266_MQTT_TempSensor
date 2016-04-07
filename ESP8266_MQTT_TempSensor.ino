#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"

#define GREENHOUSE
//#define DECK

/******************************************************************/
/*                         WIFI SETTINGS                          */
/******************************************************************/
#define WLAN_SSID       "zzr231"
#define WLAN_PASS       "W3B0ughtTh1sH0us31n2008!"

// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "sgoyette"
#define AIO_KEY         "91e4981c521b4f73bb308fe79ae34a3c"

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

/******************************************************************/
/*                         MQTT SETTINGS                          */
/******************************************************************/
// Store the MQTT server, client ID, username, and password in flash memory.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;

// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = AIO_KEY __DATE__ __TIME__;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);/****************************** Feeds ***************************************/

#ifdef DECK
#pragma message("Compiling for DECK")
// Setup feeds for temperature & humidity
const char TEMPERATURE_FEED[] = AIO_USERNAME "/feeds/DECK_temp";
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, TEMPERATURE_FEED);

const char HUMIDITY_FEED[] = AIO_USERNAME "/feeds/DECK_hum";
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, HUMIDITY_FEED);

const char VDD_FEED[] = AIO_USERNAME "/feeds/DECK_vdd";
Adafruit_MQTT_Publish vdd = Adafruit_MQTT_Publish(&mqtt, VDD_FEED);

#else
#pragma message("Compiling for Greenhouse")

// Setup feeds for temperature & humidity
const char TEMPERATURE_FEED[] = AIO_USERNAME "/feeds/greenhouse_temp";
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, TEMPERATURE_FEED);

const char HUMIDITY_FEED[] = AIO_USERNAME "/feeds/greenhouse_hum";
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, HUMIDITY_FEED);

const char VDD_FEED[] = AIO_USERNAME "/feeds/greenhouse_vdd";
Adafruit_MQTT_Publish vdd = Adafruit_MQTT_Publish(&mqtt, VDD_FEED);
#endif

#define INTERVAL 20 * 60

extern "C" {
  ADC_MODE(ADC_VCC); //vcc read
}

/***************************************************************************/
/*                              SENSOR SETUP                               */
/***************************************************************************/
#ifdef DECK
// DHT 11 sensor
#define DHTPIN 2
#define DHTTYPE DHT11

// DHT sensor
DHT dht(DHTPIN, DHTTYPE, 15);

void initSensor() {
  dht.begin();
}

int readTemperature() {
  return (int)dht.readTemperature();
}

int readHumidity() {
  return (int)dht.readHumidity();
}
#else

Adafruit_BME280 bme;

void initSensor() {
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!"); 
  }
}

int readTemperature() {
  return bme.readTemperature();
}

int readHumidity() {
  return bme.readHumidity();
}
#endif

/***************************************************************************/
/*                   Standard Arduino setup/loop                           */
/***************************************************************************/

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // Set initial state of LED off
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.begin(115200);
  Serial.println(); 
  Serial.println(); 
  Serial.println("Setting up MQTT Temp Sensor...");

  // Connect to WiFi access point.
  Serial.println(); 
  Serial.println();
  delay(10);
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();

  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());

  delay(10);
  Serial.println("Initializing sensor...");
  initSensor();

  Serial.println("Done.");
  
  // connect to adafruit io
  connect();
}

void loop() {

  // ping adafruit io a few times to make sure we remain connected
  if(! mqtt.ping(3)) {
    // reconnect to adafruit io
    if(! mqtt.connected())
      connect();
  }

  // Grab the current state of the sensor
  int humidity_data = (int)readHumidity();
  int temperature_data = (int)readTemperature();

  Serial.print("Temperature : " );
  Serial.println(temperature_data);
  
  // Publish data
  if (! temperature.publish(temperature_data)) {
    Serial.println(F("Failed to publish temperature"));
  } else {
    Serial.print(F("Temperature published to feed "));
    Serial.println(TEMPERATURE_FEED);
  }

  if (! humidity.publish(humidity_data))
    Serial.println(F("Failed to publish humidity"));
  else
    Serial.println(F("Humidity published!"));

  float current_vdd = ESP.getVcc() / 1000.0;
  if (! vdd.publish(current_vdd) ) {
    Serial.println(F("Failed to publish voltage"));
  } else {
    Serial.println(F("Voltage published!"));
  }

  digitalWrite(LED_BUILTIN, HIGH);
  ESP.deepSleep(INTERVAL * 1000000);
}

// connect to adafruit io via MQTT
void connect() {

  Serial.print(F("Connecting to Adafruit IO... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {

    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if(ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(5000);
  }
  Serial.println(F("Adafruit IO Connected!"));
}

