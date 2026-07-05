#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("DHT Test");
  dht.begin();
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Read error");
  } else {
    Serial.print("Temp: ");
    Serial.print(t);
    Serial.print("C  Humidity: ");
    Serial.print(h);
    Serial.println("%");
  }

  delay(2000);
}