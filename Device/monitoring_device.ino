//Library for MAX30105
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

//Library for ESP826
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "time.h"

//Library DS18B20 sensor
#include <OneWire.h>
#include <DallasTemperature.h>

//needed for WiFi Manager
#include <DNSServer.h>
#include <WiFiManager.h>   

//Library for Firebase
#include <FirebaseESP8266.h>

//Library for OLED LCD Display
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Setting OLED LCD Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MAX30105 particleSensor;

//Penempatan Pin GPIO sensor
const int oneWireBus = 2; 

//Pengaturan komunikasi dan sensor
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

//Define for Firebase
#define FIREBASE_HOST "aegis-covid-monitoring-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "so8J4soz4l2KZT8PIwlc6GwNg1jvmKVbuKbJ9jB2"

//Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

uint32_t irBuffer[100];
uint32_t redBuffer[100];

int32_t bufferLength;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;
long irValue;
float beatsPerMinute, beatTemp, avgBeat, temp, tempC, sumTemp, avgTemp;
float SpO2, sumBpm, sumSpO2,  avgBpm, avgSpO2;
int beatAvg;
int count = 15;
int current = 0;
FirebaseData firebaseData;
WiFiManager wm;
bool IsConnected = false;
bool ConfigPortal = false;

unsigned long currentTime = 0;
unsigned long previousTime = 0;

//Icon
const unsigned char logo [] PROGMEM = { // 32x32px
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xe0, 0x00, 0x0c, 0x00, 0x30, 0x00, 0x08, 0x7e, 0x10, 
  0x00, 0x08, 0x00, 0x10, 0x00, 0x08, 0x00, 0x10, 0x00, 0x08, 0x00, 0x10, 0x03, 0xc1, 0xe0, 0x10, 
  0x0c, 0x36, 0x18, 0x10, 0x10, 0x1c, 0x04, 0x10, 0x10, 0x00, 0x04, 0x10, 0x20, 0x02, 0x02, 0x10, 
  0x20, 0x02, 0x02, 0x10, 0x20, 0x06, 0x02, 0x10, 0x20, 0x07, 0x42, 0x10, 0x30, 0x1d, 0x40, 0x10, 
  0x13, 0xfd, 0xff, 0x10, 0x18, 0x09, 0x8c, 0x10, 0x08, 0x01, 0x88, 0x10, 0x04, 0x01, 0x90, 0x10, 
  0x02, 0x00, 0x20, 0x10, 0x01, 0x80, 0xc0, 0x10, 0x00, 0xc1, 0x80, 0x10, 0x00, 0x22, 0x00, 0x10, 
  0x00, 0x1c, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x00, 0x10, 0x00, 0x08, 0x00, 0x10, 
  0x00, 0x08, 0x18, 0x10, 0x00, 0x0c, 0x00, 0x30, 0x00, 0x07, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00
};
const unsigned char wifi [] PROGMEM = { // 8*8
  0x03, 0x03, 0x1b, 0x1b, 0xdb, 0xdb, 0xdb, 0xdb
};
const unsigned char yes [] PROGMEM = { // 12*12
  0x00, 0x01, 0x02, 0x04, 0x88, 0x50, 0x30, 0x00
};
const unsigned char no [] PROGMEM = {
  0x00, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00
};

//ID_Device
const char ID_Device[] = "CM001"; 

void setup() {
    WiFi.mode(WIFI_STA);
    Serial.begin(115200);

    //Start OLED DISPLAY
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;);
    }
    //Intro
    display.clearDisplay();
    display.setTextColor(1);
    display.setTextSize(1);
    display.drawBitmap(44, 8, logo, 32, 32, WHITE);
    display.setCursor(46,45);  display.println("AEGIS");
    display.setCursor(17,55);  display.println("Covid Monitoring");
    display.display();
    delay(4000);

    //Display
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(6,2);  display.print(ID_Device);
    display.drawRoundRect(0, 12, 128, 52, 8, WHITE);
    display.display();
    
    //Connect to wifi.
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(16, 18);
    display.print("WiFi... ");
    display.display();
    
    //Check Autoconnect WiFi
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(60);//Set Timeout Connect WiFi 60
    if(wm.autoConnect(ID_Device)){
        Serial.println("connected");
        display.setTextColor(1);
        display.drawBitmap(102, 18, yes, 8, 8, WHITE);
        IsConnected = true;
    } else {
        Serial.println("Configportal running");
        display.setTextColor(1);
        display.drawBitmap(102, 18, no, 8, 8, WHITE);
        ConfigPortal = true;
    }
    display.display();
    delay(1500); 

    //Connect to Firebase
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(16, 29);
    display.println("Firebase...");
    if (IsConnected == true) {
        display.drawBitmap(102, 29, yes, 8, 8, WHITE);
    } else {
        display.drawBitmap(102, 29, no, 8, 8, WHITE);
    }
    display.display();
    delay(1500);

    //Initialize Pulse oximeter
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(16, 40);
    display.print("Oximeter...");
    // Initialize sensor
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
    {
        display.drawBitmap(102, 40, no, 8, 8, WHITE);
    } else {
        display.drawBitmap(102, 40, yes, 8, 8, WHITE);
    }
    byte ledBrightness = 60; //Options: 0=Off to 255=50mA
    byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
    byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
    byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
    int pulseWidth = 411; //Options: 69, 118, 215, 411
    int adcRange = 4096; //Options: 2048, 4096, 8192, 16384
    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    display.display();
    delay(1500);

    //Memulai sensor DS18B20
    sensors.begin();
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(16, 51);
    display.println("DS18B20...");
    display.drawBitmap(102, 51, yes, 8, 8, WHITE);
    display.display();
    delay(4000);

    //Initialize a NTPClient
    timeClient.begin();
    timeClient.setTimeOffset(25200);

    //Reset Firebase
    Firebase.deleteNode(firebaseData,"/Monitoring/Average");

    //Clear
    display.fillRect(16, 18, 107, 44, BLACK);
    display.display();
}

void loop(){
  bufferLength = 100;
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false)
      particleSensor.check();
      
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  while (1)
  {
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }
    for (byte i = 75; i < 100; i++)
    {
      irValue = particleSensor.getIR();
      while (particleSensor.available() == false)
        particleSensor.check();

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      
      particleSensor.nextSample();
      
      Serial.print("SPO2 = ");
      if(validSPO2 == 1){
        Serial.print(spo2, DEC);
        if(spo2 > 90){
          Serial.print(spo2, DEC);
          SpO2 = spo2;
        }
      }
      Serial.print(", HR = ");
      if(validHeartRate == 1){
        Serial.print(heartRate, DEC);
        if (heartRate > 60 && heartRate < 120){
          Serial.print(heartRate, DEC);
          beatAvg = heartRate;
        }
      }
      current ++;
      // Tampilkan di serial monitor
      Serial.print("Temperature: ");
      Serial.print(tempC);
      Serial.print("'C");
      if (irValue < 50000){
          Serial.println(" | No finger?");
      }else{
          Serial.print(" | Detak Jantung : ");
          Serial.print(beatAvg);
          Serial.print(" BPM"); 
          Serial.print(" | Saturasi Oksigen : ");
          Serial.print(SpO2);
          Serial.println("%"); 
      } 
      showData();
      checkwifi();
      average();
      wm.process();
      timeClient.update();
    }
    Serial.println();
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    sensors.requestTemperatures(); 
    temp = sensors.getTempCByIndex(0);
    tempC = temp + (temp * 0.7/100);
    
   // Kirim data ke Firebase
    Firebase.setFloat(firebaseData, "/Monitoring/BPM", beatAvg);
    Firebase.setFloat(firebaseData, "/Monitoring/SpO2", SpO2);
    Firebase.setFloat(firebaseData, "/Monitoring/Temp", tempC);
  }
}

//Hitung rata2
void average() {
  String formattedTime = timeClient.getFormattedTime();
  sumTemp += tempC;
  sumBpm += beatAvg;
  sumSpO2 += SpO2;
  if (current == count){
    avgTemp = sumTemp/count;
    avgBpm = sumBpm/count;
    avgSpO2 = sumSpO2/count;
    Serial.print("Rata-rata = ");
    Serial.print("| Suhu = ");
    Serial.print(avgTemp);
    Serial.print(" | BPM = ");
    Serial.print(avgBpm);
    Serial.println(" | SpO2 = ");
    Serial.println(avgSpO2);
    Firebase.setFloat(firebaseData, "/Monitoring/Average/Temp/"+formattedTime, avgTemp);
    Firebase.setFloat(firebaseData, "/Monitoring/Average/Bpm/"+formattedTime, avgBpm);
    Firebase.setFloat(firebaseData, "/Monitoring/Average/SpO2/"+formattedTime, avgSpO2);
    sumTemp = 0;
    sumBpm = 0;
    sumSpO2 = 0;
    current = 0;
  }
}

void showData() {
    display.setTextSize(1);
    display.setCursor(10,20);
    display.print("Suhu :");
 
    display.setTextSize(1);
    display.setCursor(54, 20);
    display.fillRect(54, 20, 30, 20, BLACK);
    display.print(tempC);
    display.setCursor(94, 20);
    display.print("C");
    
    display.setTextSize(1);
    display.setCursor(10,34); // column row
    display.print("Nadi :");
 
    display.setTextSize(1);
    display.setCursor(54, 34);
    display.fillRect(54, 34, 40, 20, BLACK);
    if (irValue < 50000){
        display.print("--");
    }else{
        display.print(beatAvg);
    }
    display.setCursor(94, 34);
    display.print("BPM");
        
    display.setTextSize(1);
    display.setCursor(10,48);
    display.print("SpO2 :");
 
    display.setTextSize(1);
    display.setCursor(54, 48);
    display.fillRect(54, 48, 30, 15, BLACK);
    if (irValue < 50000){
        display.print("--");
    }else{
        display.print(SpO2);
    }    
    display.setCursor(94, 48);
    display.print("%");
    
    display.display();
}

void checkwifi(){
    if(WiFi.status() != WL_CONNECTED){
       if(wm.getConfigPortalActive() == true){
          display.setTextSize(1);
          display.setTextColor(1);
          display.setCursor(110, 2);
          display.print("?");
          display.display();
        }else{
          display.fillRect(110, 2, 8, 8, BLACK);
          display.display();
        }
    }else{
      display.drawBitmap(110, 2, wifi, 8, 8, WHITE);
      display.display();
    }
}


