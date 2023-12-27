#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <DHT.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_GFX.h>
#include "LiquidCrystal_PCF8574.h"
#include <Adafruit_SSD1306.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

#define SCREEN_WIDTH 16
#define SCREEN_HEIGHT 2
#define DHTPIN 4
#define DHTTYPE DHT12

DHT dht(DHTPIN, DHTTYPE);

#define API_KEY  "AIzaSyC9NbfEWwnW2l4bbJsVLml44h--MGvTc_8"
#define FIREBASE_PROJECT_ID  "temperature-9d0fb"
#define USER_EMAIL "alosyndiel4@gmail.com"
#define USER_PASSWORD  "alosyndiel123"

TwoWire wire1;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String path = "Monitor";

#define LCD_ADDRESS 0x3F
#define LCD_ROWS 2
#define LCD_COLUMNS 16
#define SCROLL_DELAY 150
#define BACKLIGHT 255

LiquidCrystal_PCF8574 lcdI2C(LCD_ADDRESS);
Adafruit_SGP30 sgp;

void initializeSensors() {
  if (!sgp.begin()) {
    Serial.println("Error: SGP30 sensor not found. Please check connections.");
    while (1);
  }
}

void connectToWiFi() {
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("AutoConnectAP")) {
    Serial.println("Failed to connect to Wi-Fi");
    while (1);
  }
  Serial.println("WiFi connected to " + WiFi.localIP().toString());
}

void initializeFirebase() {
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);
  delay(3000);

  Serial.println("Getting User UID");
  while (auth.token.uid.empty()) {
    Serial.print('.');
    delay(1000);
  }

  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
}


void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Hello User, Please wait while I set things up");

  initializeSensors();
  connectToWiFi();
  initializeFirebase();

  timeClient.begin();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  delay(2000);
  Serial.println("**********************************");
  Serial.println("   Welcome to My system   ");
  Serial.println("**********************************");

  delay(2000);
  Serial.println("Initializing...");
  delay(1000);

  wire1.begin();

  lcdI2C.begin(LCD_COLUMNS, LCD_ROWS, wire1);
  lcdI2C.display();
  delay(2000);
  lcdI2C.clear();
}

void loop() {
  delay(2000);

  FirebaseJson content;

  if (!sgp.IAQmeasure()) {
    Serial.println("Failed to read SGP30 sensor");
    return;
  }

  uint16_t co2 = sgp.eCO2;
  uint16_t tvoc = sgp.TVOC;

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  lcdI2C.clear();
  lcdI2C.setCursor(0, 0);
  lcdI2C.print("CO2: ");
  lcdI2C.print(co2);
  lcdI2C.print(" ppm");

  lcdI2C.setCursor(0, 1);
  lcdI2C.print("TVOC: ");
  lcdI2C.print(tvoc);
  lcdI2C.print(" ppb");

  lcdI2C.setCursor(0, 2);
  lcdI2C.print("Temp: ");
  lcdI2C.print(temperature);
  lcdI2C.print(" °C");

  lcdI2C.setCursor(0, 3);
  lcdI2C.print("Humidity: ");
  lcdI2C.print(humidity);
  lcdI2C.print(" %");

  if (!isnan(temperature) && !isnan(humidity)) {
    content.set("fields/CO2/ppIntegerValue", co2);
    content.set("fields/TVOC/ppIntegerValue", tvoc);
    content.set("fields/Temperature/ppFloatValue", temperature);
    content.set("fields/Humidity/ppFloatValue", humidity);
  } else {
    Serial.println("Invalid temperature or humidity value (NaN)");
  }

  timeClient.update();
  time_t now = timeClient.getEpochTime();

  if (now == 0) {
    Serial.println("Error getting current time");
  } else {
    char timestampStr[30];
    strftime(timestampStr, sizeof(timestampStr), "%FT%TZ", gmtime(&now));
    content.set("fields/DateTime/timestampValue", timestampStr);
  }

  uid = String(millis());

  Serial.print("Creating document... ");
  delay(3000);
  Serial.println("Done");
  delay(3000);
  Serial.println("");
  Serial.print("Sending document.... ");

  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), content.raw())) {
    Serial.printf("Success\n%s\n\n", fbdo.payload().c_str());
  } else {
    Serial.print(fbdo.errorReason());
  }

  delay(10000);
}
