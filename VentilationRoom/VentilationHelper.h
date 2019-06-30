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
#define DEVICE_TOPIC_LEN 32

// Waiting for connection before switch off device.
#define WAIT_FOR_CONNECTION_TIMEOUT_MS 15 * 60 * 1000 // 15 minutes

const char MQTT_SERVER_KEY[] = "mqttServer";
const char MQTT_PORT_KEY[] = "mqttPort";
const char MQTT_CLIENT_ID_KEY[] = "mqttClientId";
const char MQTT_USER_KEY[] = "mqttUser";
const char MQTT_PASS_KEY[] = "mqttPass";
const char BASE_TOPIC_KEY[] = "baseTopic";
const char DEVICE_TOPIC_KEY[] = "deviceTopic";
const char CONFIG_FILE_NAME[] = "/config.json";

const char TOPIC_SET[] = "set";
const char PAYLOAD_READY[] = "ready";
const char PAYLOAD_OPEN[] = "open";
const char PAYLOAD_CLOSE[] = "close";
const char PAYLOAD_ALL[] = "all";

const char WINDOW_TOPIC[] = "window";
const char GATES_TOPIC[] = "gates";
const char GATE_TOPIC[] = "gate";

enum DeviceData
{
	AllData,
	BroadcastDevice,
	DeviceIsReady,
	DeviceOk,
	WindowState,
	GateState,
	AllGatesState
};

struct DeviceSettings
{
	char MqttServer[MQTT_SERVER_LEN] = "x.cloudmqtt.com";
	char MqttPort[MQTT_PORT_LEN] = "1883";
	char MqttClientId[MQTT_CLIENT_ID_LEN] = "ESP8266Client";
	char MqttUser[MQTT_USER_LEN];
	char MqttPass[MQTT_PASS_LEN];
	char BaseTopic[BASE_TOPIC_LEN] = "flat";
	char DeviceTopic[DEVICE_TOPIC_LEN] = "bedroom1";
};

bool connectWiFi();

void ReadConfiguration(DeviceSettings* settings);
bool manageConnectAndSettings(WiFiManager* wifiManager, DeviceSettings* settings);
void SaveConfiguration(DeviceSettings* settings);
void saveConfigCallback();

#endif
