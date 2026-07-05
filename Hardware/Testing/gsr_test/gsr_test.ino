#include <DHT.h>

#define DHTPIN 2        // DATA pin connected to D2
#define DHTTYPE DHT22   // Sensor type

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(2000); // allow sensor to stabilize

  Serial.println("DHT22 Test Starting...");
  dht.begin();
}

void loop() {

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Sensor read failed");
  } 
  else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C  |  Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }

  delay(200); // DHT22 needs ~2s between readings
}