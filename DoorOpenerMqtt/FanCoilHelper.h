// FanCoilHelper.h

#ifndef _FANCOILHELPER_h
#define _FANCOILHELPER_h

#include <FS.h>
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <KMPDinoWiFiESP.h>

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

#define OPEN_DOOR_INTERVAL_MS 10000

#define PING_INTERVAL_MS 30000

#define DOOR_RELAY Relay1

const char MQTT_SERVER_KEY[] = "mqttServer";
const char MQTT_PORT_KEY[] = "mqttPort";
const char MQTT_CLIENT_ID_KEY[] = "mqttClientId";
const char MQTT_USER_KEY[] = "mqttUser";
const char MQTT_PASS_KEY[] = "mqttPass";
const char BASE_TOPIC_KEY[] = "baseTopic";
const char CONFIG_FILE_NAME[] = "/config.json";

const char TOPIC_SEPARATOR[] = "/";
const char TOPIC_SET[] = "set";
const char TOPIC_DOOR_STATE[] = "doorstate";
const char PAYLOAD_ON[] = "on";
const char PAYLOAD_OFF[] = "off";
const char PAYLOAD_READY[] = "ready";
const char PAYLOAD_PING[] = "ping";

const char EVERY_ONE_LEVEL_TOPIC[] = "+";

enum DoorAction
{
	Off = 0,
	On  = 1
};

enum DeviceData
{
	DoorState = 1,
	DevicePing = 2,
	DeviceIsReady = 4,
};

struct DeviceSettings
{
	char MqttServer[MQTT_SERVER_LEN] = "x.cloudmqtt.com";
	char MqttPort[MQTT_PORT_LEN] = "1883";
	char MqttClientId[MQTT_CLIENT_ID_LEN] = "ESP8266Client";
	char MqttUser[MQTT_USER_LEN];
	char MqttPass[MQTT_PASS_LEN];
	char BaseTopic[BASE_TOPIC_LEN] = "flat/door";
};

#ifdef WIFIFCMM_DEBUG
void printTopicAndPayload(const char* operationName, const char* topic, char* payload, unsigned int length);
#endif

bool connectWiFi();

void ReadConfiguration(DeviceSettings* settings);
bool mangeConnectParamers(WiFiManager* wifiManager, DeviceSettings* settings);
void SaveConfiguration(DeviceSettings* settings);
void saveConfigCallback();

#endif
