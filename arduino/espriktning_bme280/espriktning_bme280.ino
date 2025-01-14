/*
  Copyright 2022 Marco Martin <notmart@gmail.com>, ERR <e@richiardone.eu>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details
  
  You should have received a copy of the GNU Library General Public
  License along with this program; if not, write to the
  Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  Add following libraries:
    WiFiMQTTManager Library 1.0.1-beta
    WiFiManager 2.0.11-beta
    PubSubClient 2.8.0
    BME280 3.0.0 by Tyler Glenn
    NeoPixelBus by Makuna 2.7.0
    
*/

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <BME280I2C.h>

#include "SegmentPixels.h"
#include "Tokenizer.h"
#include "WifiMQTTManager.h"
#include "CommandLine.h"
#include "Settings.h"
#include "pm1006.h"


#define LEDINTENSITYATNIGHT 10  // from 0 to 100
#define LDRBIAS 500 // ambient luminosity to determine if daylight or not
#define TEMP_COMPENSATION_SUM -2 // temperature adjust because of esp8266 heating
#define TEMP_COMPENSATION_ADD 0.8 //  temperature adjust because of esp8266 heating
#define VERBOSE true
#define MQTT_TOPIC_PM2_5 "/pm2_5"
#define MQTT_TOPIC_TEMP "/temp"
#define MQTT_TOPIC_PRES "/pres"
#define MQTT_TOPIC_HUM "/hum"

#define PIN_PM1006_RX  5 //D1
#define PIN_PM1006_TX  4 //D2
#define PIN_RESETFLASH 0
#define PIN_LDR        A0 //A0
#define PIN_FAN        13 //D7
#define PIN_PIXELS     14 //D5
#define PIN_BME280_SCL 2
#define PIN_BME280_SDA 12

static SoftwareSerial pmSerial(PIN_PM1006_RX, PIN_PM1006_TX);
static PM1006 pm1006(&pmSerial);

BME280I2C::Settings bmesettings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_16,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76
);

BME280I2C bme(bmesettings);

static const uint fanOnTime = 0;
static const uint fanOffTime = 20000;
static const uint measurementTime = 30000;

unsigned long factoryResetButtonDownTime = 0;
unsigned long lastCycleTime = 0;

static const uint ldrInterval = 120000;

unsigned long lastLdrTime = 0;

bool ledsOn = true;
bool fan = false;

uint8_t initial_wait;

uint16_t pm2_5 = 0;

bool bme_flag;
float bme_temp(NAN);
float bme_hum(NAN);
float bme_pres(NAN);

bool display_alt = false;

WifiMQTTManager wifiMQTT("ESPriktning");

NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> pixelBus(SegmentPixels::numPixelsForDigits(2, 3), PIN_PIXELS);
SegmentPixels pixels(&pixelBus, 2, 3);
Tokenizer tokenizer;

void syncPixelsAnimation()
{
  for (int i = 0; i < 20; ++i) {
    pixels.updateAnimation();
    delay(25);
  }
}

void getBMEData(){

  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_hPa);
  bme.read(bme_pres, bme_temp, bme_hum, tempUnit, presUnit);

  // temperature correction
  bme_temp += TEMP_COMPENSATION_SUM;
  bme_temp *= TEMP_COMPENSATION_ADD;
  
  #ifdef VERBOSE
  Serial.println("Info: read " + String(bme_temp) + " °C " + String(bme_hum) + " %RH " + String(bme_pres) + " hPa");
  #endif
  
}


void setup()
{
  Serial.begin(115200);

  enableWiFiAtBootTime();
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);

  // Recycle the flash button as a factory reset
  pinMode(PIN_RESETFLASH, INPUT_PULLUP);
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LDR, INPUT);

  bme_flag = true;
  Wire.begin(PIN_BME280_SDA, PIN_BME280_SCL);
  if(!bme.begin()){
    #ifdef VERBOSE
    Serial.println("");
    Serial.println("");
    Serial.println("Warning: BME280 sensor not found");
    #endif
    bme_flag = false;
  }
  getBMEData();

  pmSerial.begin(PM1006::BIT_RATE);

  Settings *s = Settings::self();
  s->load();

  lastCycleTime = millis();
  pixels.begin();
  pixels.setAnimationDuration(s->animationDuration());
  double intensity = double(s->ledIntensityAtDay()) / 100;
  pixels.setLedIntensity(intensity);

  ledsOn = intensity > 0;
  pixels.setColor(3, 6, 8);
  pixels.setNumber(88);
  syncPixelsAnimation();

  if (s->useWifi()) {
    wifiMQTT.setup();
    pixels.setColor(10, 6, 5);
    pixels.setNumber(88);
  }

  initial_wait = 2;
}

void loop()
{
  Settings *s = Settings::self();

  // Every now and then shut down the leds, measure the light and shut down until is dark (configurable?)
  if (millis() - lastLdrTime > ldrInterval) {
    double intensity = double(s->ledIntensityAtNight()) / 100;
    pixels.setColor(0, 0, 0);
    ledsOn = false;
    if (millis() - lastLdrTime > ldrInterval + 2000) {
      
      Serial.print("LDR:");
      Serial.println(analogRead(PIN_LDR));
      double intensity = 0.0;
      if (analogRead(PIN_LDR) > LDRBIAS) {
        intensity = double(s->ledIntensityAtDay()) / 100;
      } else {
        intensity = double(s->ledIntensityAtNight()) / 100;
      }
      ledsOn = intensity > 0;
      pixels.setLedIntensity(intensity);
      
      if(ledsOn){
        if(display_alt){
          if(bme_flag){
            pixels.setTempColorNumber(bme_temp);
          } else {
            pixels.setPM25ColorNumber(pm2_5);
          }
          display_alt = false;
        } else {
          pixels.setPM25ColorNumber(pm2_5);
           display_alt = true;
        }
        
      }
      lastLdrTime = millis();
    }
  }
  pixels.updateAnimation();
  
  // TODO: parse some commands form the tokenizer
  if (tokenizer.tokenizeFromSerial()) {
    parseCommand(tokenizer, wifiMQTT, pixels);
  }

  // Commandline may have modified settings
  if (s->isDirty()) {
    s->save();
  }

  // flash/factory reset button
  if (digitalRead(0) == 0) {
    if (factoryResetButtonDownTime == 0) {
      factoryResetButtonDownTime = millis();
    } else if (millis() - factoryResetButtonDownTime > 2000) {
      wifiMQTT.factoryReset();
    }
  } else {
    factoryResetButtonDownTime = 0;
  }


  long delta = millis() - lastCycleTime;
  if(delta > measurementTime) {
    lastCycleTime = millis();

    if(initial_wait == 0) {
      printf("Info: Attempting measurements:\n");
      if(pm1006.read_pm25(&pm2_5)){
        Serial.print("Info: PM2.5 New sensor value:");
        Serial.println(String(pm2_5).c_str());
      } else {
        Serial.println("Info: PM2.5 Measurement failed");
      }
      if(bme_flag){
        getBMEData();
      }

      // publish after initial wait
      if (s->useWifi()) {
        // std::shared_ptr<PubSubClient> client = wifiMQTT.ensureMqttClientConnected();
        // client->loop();
        //client->publish(Settings::self()->mqttTopic(), String(pm2_5).c_str(), true);
        wifiMQTT.tryPublish(Settings::self()->mqttTopic() + MQTT_TOPIC_PM2_5, String(pm2_5));
        if(bme_flag){
          wifiMQTT.tryPublish(Settings::self()->mqttTopic() + MQTT_TOPIC_TEMP, String(bme_temp));
          wifiMQTT.tryPublish(Settings::self()->mqttTopic() + MQTT_TOPIC_PRES, String(bme_pres));
          wifiMQTT.tryPublish(Settings::self()->mqttTopic() + MQTT_TOPIC_HUM, String(bme_hum));            
        }
      }
      
    } else {
      initial_wait--;
    }


  } else if (delta > fanOffTime) {
    if (fan) {
      Serial.println("turning fan off");
      digitalWrite(PIN_FAN, 0);
      fan = false;
    }
  } else if (delta > fanOnTime) {
    if (!fan) {
      Serial.println("turning fan on");
      digitalWrite(PIN_FAN, 1);
      fan = true;
    }
  }
  //  pixels.updateAnimation();
}
