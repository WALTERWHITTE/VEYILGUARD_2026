#include <Wire.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not detected!");
    while (1);
  }

  Serial.println("BMP280 detected");
}

void loop() {

  Serial.print("Temp: ");
  Serial.print(bmp.readTemperature());
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(bmp.readPressure()/100);
  Serial.println(" hPa");

  Serial.println();

  delay(2000);
}