#include <Wire.h>
#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Adafruit MLX90614 test");

  Wire.begin(D9, D10);   // use default I2C pins

  if (!mlx.begin()) {
    Serial.println("Sensor not detected!");
    while (1);
  }

  Serial.println("Sensor detected!");
}

void loop() {
  Serial.print("Ambient: ");
  Serial.print(mlx.readAmbientTempC());
  Serial.print(" °C  ");

  Serial.print("Object: ");
  Serial.print(mlx.readObjectTempC());
  Serial.println(" °C");

  delay(1000);
}