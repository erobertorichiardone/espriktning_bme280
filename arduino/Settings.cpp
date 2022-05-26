/*
 *   Copyright 2022 Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "Settings.h"

#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "FS.h" // SPIFFS is declared

Settings *Settings::s_settings = new Settings();

Settings::Settings()
    : m_mqttTopic("PM2_5")
    , m_mqttPort("1883")
{}

Settings::~Settings()
{}

Settings *Settings::self()
{
    return Settings::s_settings;
}

bool Settings::isDirty() const
{
    return m_dirty;
}

void Settings::printSettings() const
{
    Serial.print("All available settings");
    Serial.print("Use Wifi:               ");
    Serial.println(m_useWifi ? "true" : "false");
    Serial.print("Led Intensity at day:   ");
    Serial.println(m_ledIntensityAtDay);
    Serial.print("Led Intensity at night: ");
    Serial.println(m_ledIntensityAtNight);

    Serial.print("MQTT Topic:             ");
    Serial.println(m_mqttTopic);
    Serial.print("MQTT server:            ");
    Serial.println(m_mqttServer);
    Serial.print("MQTT port:              ");
    Serial.println(m_mqttPort);
    Serial.print("MQTT user name:         ");
    Serial.println(m_mqttUserName);
    Serial.print("MQTT password:          ");
    Serial.println(m_mqttPassword);
}

void Settings::save()
{
    DynamicJsonDocument json(512);
    json["use_wifi"] = m_useWifi;
    json["led_intensity_at_day"] = m_ledIntensityAtDay;
    json["led_intensity_at_night"] = m_ledIntensityAtNight;

    json["mqtt_topic"] = m_mqttTopic;
    json["mqtt_server"] = m_mqttServer;
    json["mqtt_port"] = m_mqttPort;
    json["mqtt_username"] = m_mqttUserName;
    json["mqtt_password"] = m_mqttPassword;

    if (!SPIFFS.begin()) {
        // If fails first time, try to format
        SPIFFS.format();
    }

    if (!SPIFFS.begin()) {
        // Give up
        Serial.println("Settings: error saving config, failed to open SPIFFS");
        m_error = Error::SPIFFSError;
        return;
    }
 
    Serial.println("Settings: mounted file system...");
    File configFile = SPIFFS.open("/config.json", "w");

    if (!configFile) {
        Serial.println("Settings: failed to open config file for writing...");
        m_error = Error::FileWriteError;
        return;
    }
 
    serializeJson(json, configFile);
    serializeJsonPretty(json, Serial);
    Serial.println("");
    Serial.println("Settings: config.json saved.");
    configFile.close();
    m_dirty = false;
}

void Settings::load()
{
    Serial.println("Settings: mounting SPIFFS...");
    if (!SPIFFS.begin()) {
        m_error = Error::SPIFFSError;
        Serial.println("Settings: failed to mount FS");
        return;
    }

    Serial.println("Settings: mounted file system...");
    if (!SPIFFS.exists("/config.json")) {
        m_error = Error::FileNotFoundError;
        Serial.println("Settings: could not find config file, performing factory reset...");
        return;
    }

    //file exists, reading and loading
    Serial.println("Settings: reading config file...");
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        m_error = Error::FileOpenError;
        Serial.println("Settings: cannot open config.json for reading");
        return;
    }

    Serial.println("Settings: opened config file...");
    size_t size = configFile.size();

    DynamicJsonDocument json(512);
    DeserializationError error = deserializeJson(json, configFile);
    if (error) {
        m_error = Error::JsonDeserializeError;
        Serial.print("Settings: failed to load json config: ");
        Serial.println(error.c_str());
        return;
    }

    if (json.containsKey("use_wifi")) {
        m_useWifi = !strcmp(json["use_wifi"], "true");
    }
    if (json.containsKey("led_intensity_at_day")) {
        String numString(json["led_intensity_at_day"]);
        m_ledIntensityAtDay = min(long(100), max(long(0), numString.toInt()));
    }
    if (json.containsKey("led_intensity_at_night")) {
        String numString(json["led_intensity_at_night"]);
        m_ledIntensityAtNight = min(long(100), max(long(0), numString.toInt()));
    }

    if (json.containsKey("")) {
        m_mqttTopic = String(json["mqtt_topic"]);
    }
    if (json.containsKey("")) {
        m_mqttServer = String(json["mqtt_server"]);
    }
    if (json.containsKey("")) {
        m_mqttPort = String(json["mqtt_port"]);
    }
    if (json.containsKey("")) {
        m_mqttUserName = String(json["mqtt_username"]);
    }
    if (json.containsKey("")) {
        m_mqttPassword = String(json["mqtt_password"]);
    }

    Serial.println("\nSettings: parsed json...");
    serializeJsonPretty(json, Serial);
    m_dirty = false;
}

bool Settings::useWifi() const
{
    return m_useWifi;
}

void Settings::setUseWifi(bool useWifi)
{
    if (m_useWifi == useWifi) {
        return;
    }

    m_useWifi = useWifi;
    m_dirty = true;
}

int Settings::ledIntensityAtDay() const
{
    return m_ledIntensityAtDay;
}

void Settings::setLedIntensityAtDay(int intensity)
{
    if (m_ledIntensityAtDay == intensity) {
        return;
    }

    m_ledIntensityAtDay = intensity;
    m_dirty = true;
}

int Settings::ledIntensityAtNight() const
{
    return m_ledIntensityAtNight;
}

void Settings::setLedIntensityAtNight(int intensity)
{
    if (m_ledIntensityAtNight == intensity) {
        return;
    }

    m_ledIntensityAtNight = intensity;
    m_dirty = true;
}

String Settings::mqttTopic() const
{
    return m_mqttTopic;
}

void Settings::setMqttTopic(const String &topic)
{
    if (m_mqttTopic == topic ) {
        return;
    }

    m_mqttTopic = topic;
    m_dirty = true;
}

String Settings::mqttServer() const
{
    return m_mqttServer;
}

void Settings::setMqttServer(const String &server)
{
    if (m_mqttServer == server) {
        return;
    }

    m_mqttServer = server;
    m_dirty = true;
}

String Settings::mqttPort() const
{
    return m_mqttPort;
}

void Settings::setMqttPort(const String &port)
{
    if (m_mqttPort == port) {
        return;
    }

    m_mqttPort = port;
    m_dirty = true;
}

String Settings::mqttUserName() const
{
    return m_mqttUserName;
}

void Settings::setMqttUserName(const String &userName)
{
    if (m_mqttUserName == userName) {
        return;
    }

    m_mqttUserName = userName;
    m_dirty = true;
}

String Settings::mqttPassword() const
{
    return m_mqttPassword;
}

void Settings::setMqttPassword(const String &password)
{
    if (m_mqttPassword == password) {
        return;
    }

    m_mqttPassword = password;
    m_dirty = true;
}