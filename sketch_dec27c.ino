#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <Adafruit_SGP30.h>
#include <DHT.h>

// Pin configuration
#define LCD_ADDRESS 0x27 // I2C address of your LCD
#define DHTPIN 4          // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11     // DHT sensor type (DHT11 or DHT22)
#define SGP30_ADDRESS 0x58 // I2C address of SGP30

// Create objects
LiquidCrystal_PCF8574 lcd;
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SGP30 sgp;

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2); // Initialize the LCD
  lcd.setBacklight(255);

  Serial.println("Initializing DHT sensor...");
  dht.begin();

  Serial.println("Initializing SGP30 sensor...");
  unsigned long startTime = millis();
  while (!sgp.begin()) {
    if (millis() - startTime > 5000) {
      Serial.println("Error: SGP30 sensor not found");
      break;
    }
    delay(500);
  }

  Serial.println("Sensor initialization complete");
}

void loop() {
  // Read temperature and humidity from DHT sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Display temperature and humidity on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.print(" %");

  // Read CO2 and TVOC from SGP30 sensor
  if (sgp.IAQmeasure()) {
    lcd.setCursor(0, 2);
    lcd.print("CO2: ");
    lcd.print(sgp.eCO2);
    lcd.print(" ppm");

    lcd.setCursor(0, 3);
    lcd.print("TVOC: ");
    lcd.print(sgp.TVOC);
    lcd.print(" ppb");
  } else {
    Serial.println("Error reading SGP30 sensor");
  }

  delay(2000); // Update every 2 seconds
}
