#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define GSR_PIN A0

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

/* ---------- GSR VARIABLES ---------- */

int baseline = 0;
float filteredGSR = 0;
float previousGSR = 0;

float alpha = 0.2;

/* ---------- READ GSR ---------- */

int readGSR(){

  long sum = 0;

  for(int i=0;i<10;i++){
    sum += analogRead(GSR_PIN);
    delay(10);
  }

  return sum/10;
}

/* ---------- SMOOTHING ---------- */

float smoothGSR(int raw){

  filteredGSR = alpha*raw + (1-alpha)*filteredGSR;

  return filteredGSR;
}

/* ---------- SWEAT RATE ---------- */

float calculateSlope(float current){

  float slope = current - previousGSR;

  previousGSR = current;

  return slope;
}

/* ---------- BASELINE CALIBRATION ---------- */

void calibrateGSR(){

  Serial.println("Calibrating baseline... place fingers on electrodes");

  delay(5000);

  long sum = 0;

  for(int i=0;i<50;i++){
    sum += analogRead(GSR_PIN);
    delay(50);
  }

  baseline = sum/50;

  Serial.print("Baseline GSR: ");
  Serial.println(baseline);
}

/* ---------- OSHA HEAT INDEX ---------- */

float calculateHeatIndex(float T, float H){

  float HI = -8.784 +
             1.611*T +
             2.338*H -
             0.146*T*H -
             0.0123*T*T -
             0.0164*H*H +
             0.0022*T*T*H +
             0.0007*T*H*H -
             0.000003*T*T*H*H;

  return HI;
}

/* ---------- SETUP ---------- */

void setup(){

  Serial.begin(115200);

  dht.begin();

  if(!bmp.begin(0x76)){
    Serial.println("BMP280 not detected");
    while(1);
  }

  calibrateGSR();

  Serial.println("Heat Stress Monitoring Started");
}

/* ---------- LOOP ---------- */

void loop(){

  /* ----- ENVIRONMENT DATA ----- */

  float Tenv = dht.readTemperature();
  float humidity = dht.readHumidity();

  float Tbody = bmp.readTemperature();
  float pressure = bmp.readPressure()/100;

  /* ----- GSR READ ----- */

  int rawGSR = readGSR();

  if(rawGSR == 0){

    Serial.println("GSR: 0 (No contact)");
    Serial.println("---------------------------------");

    delay(1000);
    return;
  }

  float stableGSR = smoothGSR(rawGSR);

  float slope = calculateSlope(stableGSR);

  int diff = baseline - stableGSR;

  /* ----- STRESS DETECTION ----- */

  String stressLevel;

  if(abs(slope) > 120)
    stressLevel = "Acute Stress Spike";
  else if(diff > 80)
    stressLevel = "High Stress";
  else if(diff > 30)
    stressLevel = "Mild Stress";
  else
    stressLevel = "Relaxed";

  /* ----- SWEAT / HEAT FATIGUE DETECTION ----- */

  float sweatFactor = abs(diff)/200.0;

  if(sweatFactor > 1) sweatFactor = 1;

  float slopeFactor = abs(slope)/200.0;

  if(slopeFactor > 1) slopeFactor = 1;

  float physiologicalSweat = (0.7*sweatFactor) + (0.3*slopeFactor);

  /* ----- HEAT INDEX ----- */

  float HI = calculateHeatIndex(Tenv, humidity);

  /* ----- HEAT STRESS INDEX ----- */

  float PHSI = (0.70*HI) +
               (0.15*Tbody) +
               (0.15*physiologicalSweat*100);

  /* ----- SAFE WORK TIME ----- */

  int safeTime;

  if(PHSI < 30)
    safeTime = 180;
  else if(PHSI < 40)
    safeTime = 120;
  else if(PHSI < 50)
    safeTime = 60;
  else if(PHSI < 60)
    safeTime = 30;
  else
    safeTime = 10;

  /* ----- OUTPUT ----- */

  Serial.println("------ HEAT STRESS MONITOR ------");

  Serial.print("External Temp: ");
  Serial.print(Tenv);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Body Temp: ");
  Serial.print(Tbody);
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");

  Serial.print("Raw GSR: ");
  Serial.println(rawGSR);

  Serial.print("Filtered GSR: ");
  Serial.println(stableGSR);

  Serial.print("Sweat Rate (Slope): ");
  Serial.println(slope);

  Serial.print("Stress Level: ");
  Serial.println(stressLevel);

  Serial.print("Heat Index: ");
  Serial.println(HI);

  Serial.print("Physiological Heat Stress Index: ");
  Serial.println(PHSI);

  Serial.print("Safe Work Time: ");
  Serial.print(safeTime);
  Serial.println(" minutes");

  Serial.println("---------------------------------");

  delay(300);
}