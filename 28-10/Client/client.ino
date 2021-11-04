#include <MQTT.h>
#ifdef ARDUINO_SAMD_MKRWIFI1010
#include <WiFiNINA.h>
#elif ARDUINO_SAMD_MKR1000
#include <WiFi101.h>
#elif ESP8266
#include <ESP8266WiFi.h>
#else
#error unknown board
#endif

// Import NEO Pixel Libraries
#include "Adafruit_NeoPixel.h"
#ifdef __AVR__
#include <avr/power.h>
#endif

// Import distance reader libraries
#include "Ultrasonic.h"

// Import DHT libreries ()
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Import JSON Library
#include <Arduino_JSON.h>

// Define MQTT global variables
#define BROKER_IP    "broker.hivemq.com"
#define DEV_NAME     "xxxx"
#define MQTT_USER    ""
#define MQTT_PW      ""

// Define DHT global variables
#define DHTPIN 2     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11

// Define WIFI Credentials
const char ssid[] = "xxxx";
const char pass[] = "xxxx";

// Create neo pixel string object
#define NEO_PIXEL_PIN 3
#define STRIP_LEDS 20
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_LEDS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Create global variables
WiFiClient net;
MQTTClient client;
DHT_Unified dht(DHTPIN, DHTTYPE);
Ultrasonic ultrasonic(4);
uint32_t dhtDelay = 5000;
unsigned long dhtTimestamp = 0;

void paintGreen() {
  for (int i = 0; i < STRIP_LEDS; i++ ) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
    strip.show();
  }
  delay(1000);
  for (int i = 0; i < STRIP_LEDS; i++ ) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
    strip.show();
  }
}

void paintRed() {
  for (int i = 0; i < STRIP_LEDS; i++ ) {
    strip.setPixelColor(i, strip.Color(255, 0, 0));
    strip.show();
  }
  delay(1000);
  for (int i = 0; i < STRIP_LEDS; i++ ) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
    strip.show();
  }
}
void receiveAlert(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if (topic == "/28-10-alert") {
    if (payload == "OK") {
      paintGreen();
    } else if (payload == "ALERT") {
      paintRed();
    }
  }
}
void connect() {
  Serial.println("Connecting to WIFI...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("#");
    delay(2000);
  }
  Serial.println("Connected to WIFI !");

  Serial.println("Connecting to MQTT Broker...");
  while (!client.connect(DEV_NAME, MQTT_USER, MQTT_PW)) {
    Serial.print("#");
    delay(2000);
  }
  Serial.println("Connected to MQTT Broker!");
  client.subscribe("/28-10-alert");
  Serial.println("Subscribed to topic 28-10-alert");
}

void setup() {// Neo Pixel setup
  strip.begin();
  strip.setBrightness(255);
  strip.show(); // Initialize all pixels to 'off'


  Serial.begin(115200);

  // Start connecting to wifi
  WiFi.begin(ssid, pass);

  // Start connecting to MQTT Brokeri
  client.begin(BROKER_IP, 1883, net);

  // Connect to Wifi and MQTT Broker
  connect();


  dht.begin();
  sensor_t sensor;
  dht.humidity().getSensor(&sensor);

  // Run function when a message is received from mqtt broker
  client.onMessage(receiveAlert);
}

void loop() {
  // Create JSON string
  JSONVar data;
  client.loop();
  if (!client.connected())
    connect();

  // Every tot seconds, read DHT data and send to broker
  if (millis() - dhtTimestamp > dhtDelay) {
    dhtTimestamp = millis();
    // Create event object, sensor write in this object
    sensors_event_t event;

    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);

    // Insert hum value in json data
    data["hum"] = event.relative_humidity;

    // Get temp event and print its value.
    dht.temperature().getEvent(&event);

    // Insert temp value in json data object
    data["temp"] = event.temperature;

    // Read from light sensor
    int light = analogRead(A0);
    light = map(light, 0, 800, 0, 100);
    data["light"] = light;

    // Read from distance sensor
    long RangeInCentimeters;
    data["distance"] = ultrasonic.MeasureInCentimeters();

    // JSON.stringify(myVar) can be used to convert the json var to a String
    String dataAsString = JSON.stringify(data);



    Serial.println(dataAsString);

    // Send data to mqtt client
    client.publish("/28-10", dataAsString);
  }



}
