// VentilationHelper.h

#ifndef _VENTILATIONHELPER_H
#define _VENTILATIONHELPER_H

#include <FS.h>
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

// Uncomment to enable printing out nice debug messages.
// TODO: Debug is stopped.
#define WIFIFCMM_DEBUG

// Define where debug output will be printed.
#define DEBUG_FC Serial

// Setup debug printing macros.
#ifdef WIFIFCMM_DEBUG
#define DEBUG_FC_PRINT(...) { DEBUG_FC.print(__VA_ARGS__); }
#define DEBUG_FC_PRINTLN(...) { DEBUG_FC.println(__VA_ARGS__); }
#else
#define DEBUG_FC_PRINT(...) {}
#define DEBUG_FC_PRINTLN(...) {}
#endif

#define MQTT_SERVER_LEN 40
#define MQTT_PORT_LEN 8
#define MQTT_CLIENT_ID_LEN 32
#define MQTT_USER_LEN 16
#define MQTT_PASS_LEN 16
#define BASE_TOPIC_LEN 32
#define INLET_SENSOR_CRC_LEN 16

// Waiting for connection before switch off device.
#define WAIT_FOR_CONNECTION_TIMEOUT_MS 360000 // 1 hour

const char MQTT_SERVER_KEY[] = "mqttServer";
const char MQTT_PORT_KEY[] = "mqttPort";
const char MQTT_CLIENT_ID_KEY[] = "mqttClientId";
const char MQTT_USER_KEY[] = "mqttUser";
const char MQTT_PASS_KEY[] = "mqttPass";
const char BASE_TOPIC_KEY[] = "baseTopic";
const char INLET_SENSOR_KEY[] = "inletSensorCRC";
const char MODE_KEY[] = "mode";
const char CONFIG_FILE_NAME[] = "/config.json";

const char TOPIC_SEPARATOR[] = "/";
const char TOPIC_HUMIDITY[] = "humidity";
const char TOPIC_DESIRED_TEMPERATURE[] = "desiredtemp";
const char TOPIC_BYPASS_STATE[] = "bypassstate";
const char TOPIC_TEMPERATURE[] = "temperature";
const char TOPIC_SET[] = "set";
const char TOPIC_MODE[] = "mode";
const char TOPIC_DEVICE_STATE[] = "state";
const char TOPIC_FAN_DEGREE[] = "fandegree";
const char TOPIC_INLET_TEMPERATURE[] = "inlettemp";
const char PAYLOAD_HEAT[] = "heat";
const char PAYLOAD_COLD[] = "cold";
const char PAYLOAD_ON[] = "on";
const char PAYLOAD_OFF[] = "off";
const char PAYLOAD_READY[] = "ready";
const char PAYLOAD_OK[] = "ok";

const char EVERY_ONE_LEVEL_TOPIC[] = "+";
const char NOT_AVILABLE[] = "N/A";

const char MUST_BE_ONE[] = "Must be one";

enum DeviceData
{
	CurrentDeviceState = 1,
	DeviceIsReady = 2,
	DeviceOk = 4,
	WindowCurrentState = 8
};

struct DeviceSettings
{
	char MqttServer[MQTT_SERVER_LEN] = "x.cloudmqtt.com";
	char MqttPort[MQTT_PORT_LEN] = "1883";
	char MqttClientId[MQTT_CLIENT_ID_LEN] = "ESP8266Client";
	char MqttUser[MQTT_USER_LEN];
	char MqttPass[MQTT_PASS_LEN];
	char BaseTopic[BASE_TOPIC_LEN] = "flat/bedroom1";
};

#ifdef WIFIFCMM_DEBUG
void printTopicAndPayload(const char* operationName, const char* topic, char* payload, unsigned int length);
#endif

bool connectWiFi();

float calcAverage(float* data, uint8 dataLength, uint8 precision);

void ReadConfiguration(DeviceSettings* settings);
bool mangeConnectAndSettings(WiFiManager* wifiManager, DeviceSettings* settings);
void SaveConfiguration(DeviceSettings* settings);
void saveConfigCallback();

#endif
