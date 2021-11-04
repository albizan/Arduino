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

// Import NFC Libreries
#if 0
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);
#elif 1
#include <PN532_HSU.h>
#include <PN532.h>

PN532_HSU pn532hsu(Serial1);
PN532 nfc(pn532hsu);
#else
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);
#endif

// Import NEO Pixel Libraries
#include "Adafruit_NeoPixel.h"
#ifdef __AVR__
#include <avr/power.h>
#endif

// Import DHT libreries ()
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Import JSON Library
#include <Arduino_JSON.h>

// Define MQTT global variables
#define BROKER_IP    "broker.hivemq.com"
#define DEV_NAME     "test95"
#define MQTT_USER    ""
#define MQTT_PW      ""

// Define DHT global variables
#define DHTPIN 2     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11

// Define WIFI Credentials
const char ssid[] = "HUAWEI MATE 10 LITE";
const char pass[] = "12345679";

// Create neo pixel string object
#define NEO_PIXEL_PIN 3
#define STRIP_LEDS 20
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_LEDS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Create global variables
WiFiClient net;
MQTTClient client;
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t dhtDelay = 20000;
unsigned long dhtTimestamp = 0;
uint32_t nfcDelay = 1500;
unsigned long nfcTimestamp = 0;

void paintGreen() {
  for (int i = 0; i < STRIP_LEDS; i++ ) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
    strip.show();
  }
  delay(5000);
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
  delay(5000);
  for (int i = 0; i < STRIP_LEDS; i++ ) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
    strip.show();
  }
}
void receiveNFCStatus(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if (topic == "/check_NFC_result") {
    if (payload == "OK") {
      paintGreen();
    } else if (payload == "NO") {
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
  client.subscribe("/check_NFC_result");
  Serial.println("Subscribed to topic check_NFC_result");
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

  // NFC Setup
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an NFC Card...");


  dht.begin();
  sensor_t sensor;
  dht.humidity().getSensor(&sensor);
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

    // Print to see data is correct
    // Serial.println(data);

    // JSON.stringify(myVar) can be used to convert the json var to a String
    String dataAsString = JSON.stringify(data);

    // Send data to mqtt client
    client.publish("/data_AZ_GC", dataAsString);
  }

  // Read NFC every second
  if (millis() - nfcTimestamp > nfcDelay) {
    nfcTimestamp = millis();
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
      String code;

      if (uidLength == 4)
      {
        code = String(uid[0]);
        code += String(uid[1]);
        code += String(uid[2]);
        code += String(uid[3]);
      }

      if (uidLength == 7)
      {
        code = String(uid[0]);
        code += String(uid[1]);
        code += String(uid[2]);
        code += String(uid[3]);
        code += String(uid[4]);
        code += String(uid[5]);
        code += String(uid[6]);
      }

      // Send code to mqtt topic
      Serial.print("code = ");
      Serial.println(code);
      client.publish("/check_NFC_code", code);
    }
  }

  // Run function when a message is received from mqtt broker
  client.onMessage(receiveNFCStatus);

}
