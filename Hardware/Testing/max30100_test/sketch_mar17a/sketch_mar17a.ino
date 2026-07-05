#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

/* ---------- MPU6050 ---------- */
const int MPU_ADDR = 0x68;
int16_t accelX, accelY, accelZ;

/* ---------- DHT + BMP ---------- */
#define DHTPIN 2
#define DHTTYPE DHT22
#define GSR_PIN A0

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

/* ---------- GSR ---------- */
int baseline = 0;
float filteredGSR = 0;
float previousGSR = 0;
float alpha = 0.2;

/* ---------- MOTION BUFFER ---------- */
#define WINDOW_SIZE 10
float accBuffer[WINDOW_SIZE];
int bufferIndex = 0;

/* ---------- GSR FUNCTIONS ---------- */

int readGSR(){
  long sum = 0;
  for(int i=0;i<10;i++){
    sum += analogRead(GSR_PIN);
    delay(5);
  }
  return sum/10;
}

float smoothGSR(int raw){
  filteredGSR = alpha*raw + (1-alpha)*filteredGSR;
  return filteredGSR;
}

/* ---------- ADVANCED SWEAT RATE ---------- */

float calculateSweatRate(float current, bool isMoving){

  float slope = current - previousGSR;

  // Reject spikes during motion
  if(isMoving && abs(slope) > 15){
    slope = 0;
  }

  // Reject extreme noise
  if(abs(slope) > 50){
    slope = 0;
  }

  previousGSR = current;

  return slope;
}

/* ---------- BASELINE ---------- */

void calibrateGSR(){
  Serial.println("Calibrating baseline...");
  delay(5000);

  long sum = 0;
  for(int i=0;i<50;i++){
    sum += analogRead(GSR_PIN);
    delay(20);
  }

  baseline = sum/50;
  Serial.print("Baseline GSR: ");
  Serial.println(baseline);
}

/* ---------- HEAT INDEX ---------- */

float calculateHeatIndex(float T, float H){
  return -8.784 + 1.611*T + 2.338*H - 0.146*T*H
         -0.0123*T*T -0.0164*H*H
         +0.0022*T*T*H +0.0007*T*H*H
         -0.000003*T*T*H*H;
}

/* ---------- MPU READ ---------- */

float readAccelerationMagnitude(){

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  accelX = Wire.read()<<8 | Wire.read();
  accelY = Wire.read()<<8 | Wire.read();
  accelZ = Wire.read()<<8 | Wire.read();

  float ax = accelX / 16384.0;
  float ay = accelY / 16384.0;
  float az = accelZ / 16384.0;

  return sqrt(ax*ax + ay*ay + az*az);
}

/* ---------- VARIANCE CALCULATION ---------- */

float calculateVariance(){

  float mean = 0;
  for(int i=0;i<WINDOW_SIZE;i++){
    mean += accBuffer[i];
  }
  mean /= WINDOW_SIZE;

  float variance = 0;
  for(int i=0;i<WINDOW_SIZE;i++){
    variance += pow(accBuffer[i] - mean, 2);
  }

  return variance / WINDOW_SIZE;
}

/* ---------- MET ---------- */

float calculateMET(float accMag){

  if(accMag < 1.1) return 1.2;
  else if(accMag < 1.3) return 2.5;
  else if(accMag < 1.6) return 4.5;
  else return 7.5;
}

/* ---------- SETUP ---------- */

void setup(){

  Serial.begin(115200);
  Wire.begin();

  // Wake MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  dht.begin();

  if(!bmp.begin(0x76)){
    Serial.println("BMP280 not detected");
    while(1);
  }

  calibrateGSR();

  Serial.println("Advanced System Ready 🚀");
}

/* ---------- LOOP ---------- */

void loop(){

  /* --- ENVIRONMENT --- */
  float Tenv = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(Tenv) || isnan(humidity)) {
    Tenv = 30;
    humidity = 60;
  }

  float Tbody = bmp.readTemperature();

  /* --- MPU --- */
  float accMag = readAccelerationMagnitude();

  accBuffer[bufferIndex] = accMag;
  bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;

  float variance = calculateVariance();

  bool isMoving = variance > 0.005;   // 🔥 tuned threshold

  float MET = calculateMET(accMag);

  /* --- GSR --- */
  int rawGSR = readGSR();
  float stableGSR = smoothGSR(rawGSR);

  float slope = calculateSweatRate(stableGSR, isMoving);

  int diff = baseline - stableGSR;

  float sweatFactor = abs(diff) / 200.0;
  if(sweatFactor > 1) sweatFactor = 1;

  float slopeFactor = abs(slope) / 200.0;
  if(slopeFactor > 1) slopeFactor = 1;

  float physiologicalSweat;

  if(isMoving){
    physiologicalSweat = (0.85*sweatFactor) + (0.15*slopeFactor);
  } else {
    physiologicalSweat = (0.6*sweatFactor) + (0.4*slopeFactor);
  }

  /* --- HEAT INDEX --- */
  float HI = calculateHeatIndex(Tenv, humidity);

  /* --- FINAL INDEX --- */
  float PHSI = (0.60*HI) +
               (0.20*Tbody) +
               (0.10*physiologicalSweat*100) +
               (0.10*MET*10);

  /* --- SAFE TIME --- */
  int safeTime;

  if(PHSI < 30) safeTime = 180;
  else if(PHSI < 40) safeTime = 120;
  else if(PHSI < 50) safeTime = 60;
  else if(PHSI < 60) safeTime = 30;
  else safeTime = 10;

  /* --- OUTPUT --- */

  Serial.println("------ ADVANCED HEAT STRESS ------");

  Serial.print("External Temp: "); Serial.println(Tenv);
  Serial.print("Humidity: "); Serial.println(humidity);

  Serial.print("Heat Index: "); Serial.println(HI);
  Serial.print("Body Temp: "); Serial.println(Tbody);

  Serial.print("Accel: "); Serial.println(accMag);
  Serial.print("Variance: "); Serial.println(variance);
  Serial.print("Motion: "); Serial.println(isMoving ? "YES" : "NO");

  Serial.print("MET: "); Serial.println(MET);
  Serial.print("Sweat: "); Serial.println(physiologicalSweat);

  Serial.print("PHSI+: "); Serial.println(PHSI);

  Serial.print("Safe Time: "); Serial.print(safeTime);
  Serial.println(" min");

  Serial.println("--------------------------------");

  delay(200);
}