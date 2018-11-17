/**
 * \author Moritz Ulmer <moritz.ulmer@posteo.de>
 * \date 2018
 * \copyright Apache License 2.0
 */

#include <Timer.h>

// io.h has to be included before mqtt.h
#include "io.h"

#include "configuration.h"
#include "connectivity.h"
#include "mqtt.h"

const uint pump_pin = 13;

//----------------------------------------------------------------------------
// Global instances
WiFiClient wifiClient;
Timer timer;

bernd_box::Io io;
bernd_box::Mqtt mqtt(wifiClient, bernd_box::client_id, bernd_box::mqtt_server);
bernd_box::Wifi wifi(bernd_box::ssid, bernd_box::password);

//----------------------------------------------------------------------------
// List of available tasks
int8_t checkConnectivityId;
void checkConnectivity();

int8_t readAnalogSensorsId;
void readAnalogSensors();

int8_t updateAciditySensorId;
void updateAciditySensor();

int8_t readAirSensorsId;
void readAirSensors();

int8_t readLightSensorsId;
void readLightSensors();

int8_t togglePumpStateId;
void togglePumpState();

//----------------------------------------------------------------------------
// Implementations of available tasks

// If not connected to WiFi and MQTT, attempt to reconnect. Restart on fail
void checkConnectivity() {
  if (!wifi.isConnected()) {
    Serial.println("WiFi: Disconnected. Attempting to reconnect");
    if (wifi.connect(bernd_box::wifi_connect_timeout) == false) {
      Serial.printf("WiFi: Could not connect to %s. Restarting\n",
                    bernd_box::ssid);
      ESP.restart();
    }
  }

  // If not connected to an MQTT broker, attempt to reconnect. Else reboot
  if (!mqtt.isConnected()) {
    Serial.println("MQTT: Disconnected. Attempting to reconnect");
    if (mqtt.connect(bernd_box::connection_attempts) == false) {
      Serial.println("MQTT: Could not connect to broker. Restarting\n");
      ESP.restart();
    }
  }

  mqtt.receive();
}

// Read and then print the analog sensors
void readAnalogSensors() {
  io.setStatusLed(true);

  // Read all sensors. Then print and send them over MQTT
  Serial.printf("\n%-10s|%-4s|%-15s|%s\n", "Sensor", "Pin", "Value", "Unit");
  Serial.printf("----------|----|---------------|----\n");
  const auto& sensor = io.adcs_.find(bernd_box::Sensor::kTotalDissolvedSolids);
  float value = io.readAnalog(sensor->first);
  Serial.printf("%-10s|%-4i|%-15f|%s\n", sensor->second.name.c_str(),
                sensor->second.pin_id, value, sensor->second.unit.c_str());
  mqtt.send(sensor->second.name.c_str(), value);
  // for (auto& it : io.adcs_) {
  //   float value = io.readAnalog(it.first);

  //   Serial.printf("%-10s|%-4i|%-15f|%s\n", it.second.name.c_str(),
  //                 it.second.pin_id, value, it.second.unit.c_str());
  //   mqtt.send(it.second.name.c_str(), value);
  // }

  io.setStatusLed(false);
}

// Take multiple acidity readings and average them. Task stops after enough
// measurements have been collected.
void updateAciditySensor() {
  io.setStatusLed(true);

  io.takeAcidityMeasurement();

  // Once enough measurements have been taken, stop the task and send
  if (io.isAcidityMeasurementFull()) {
    timer.stop(updateAciditySensorId);

    float measurement = io.getMedianAcidityMeasurement();
    const bernd_box::AdcSensor& sensor =
        io.adcs_.find(bernd_box::Sensor::kAciditiy)->second;

    Serial.printf("Median acidity value is %f %s\n", measurement,
                  sensor.unit.c_str());
    mqtt.send(sensor.name, measurement);
  }

  io.setStatusLed(false);
}

// Reads, prints and then sends all air sensor parameters
void readAirSensors() {
  io.setStatusLed(true);

  for (const auto& bme : io.bme280s_) {
    float value = io.readBme280Air(bme.first);
    Serial.printf("The %s is %f %s\n", bme.second.name.c_str(), value,
                  bme.second.unit.c_str());
    mqtt.send(bme.second.name.c_str(), value);
  }

  io.setStatusLed(false);
}

// Reads, prints and then sends all light sensors
void readLightSensors() {
  io.setStatusLed(true);

  for (const auto& max44009 : io.max44009s_) {
    float value = io.readMax44009Light(max44009.first);
    Serial.printf("Ambient brightness (ID: %u) is %f %s\n",
                  static_cast<int>(max44009.first), value,
                  max44009.second.unit.c_str());
    mqtt.send(max44009.second.name.c_str(), value);
  }

  io.setStatusLed(false);
}

bool isPumpOn = false;

void togglePumpState() {
  if (isPumpOn) {
    digitalWrite(pump_pin, LOW);
    isPumpOn = false;
  } else {
    digitalWrite(pump_pin, HIGH);
    isPumpOn = true;
  }
}

void setup() {
  Serial.begin(115200);

  // Try to connect to Wifi within wifi_connect_timeout, else restart
  io.setStatusLed(true);
  if (wifi.connect(bernd_box::wifi_connect_timeout) == false) {
    Serial.printf("WiFi: Could not connect to %s. Restarting\n",
                  bernd_box::ssid);
    ESP.restart();
  }
  wifi.printState();

  // Try to connect to the MQTT broker 3 times, else restart
  io.setStatusLed(false);
  if (mqtt.connect(bernd_box::connection_attempts) == false) {
    Serial.println("MQTT: Could not connect to broker. Restarting\n");
    ESP.restart();
  }

  // Try to configure the IO devices, else restart
  if (io.init() != bernd_box::Result::kSuccess) {
    Serial.println("IO: Initialization failed. Restarting\n");
    ESP.restart();
  }

  io.disableAllAnalog();

  pinMode(pump_pin, OUTPUT);
  digitalWrite(pump_pin, HIGH);

  checkConnectivityId = timer.every(100, checkConnectivity);
  readAnalogSensorsId = timer.every(1000, readAnalogSensors);
  togglePumpStateId = timer.every(1000 * 30, togglePumpState);
  // readAirSensorsId = timer.every(10000, readAirSensors);
  // readLightSensorsId = timer.every(10000, readLightSensors);
  // updateAciditySensorId = timer.every(30, updateAciditySensor);
}

void loop() { timer.update(); }
