// WiFiFanCoilMqttMng.ino
// Company: KMP Electronics Ltd, Bulgaria
// Web: http://kmpelectronics.eu/
// Supported boards:
//    KMP ProDino WiFi-ESP WROOM-02 (http://www.kmpelectronics.eu/en-us/products/prodinowifi-esp.aspx)
// Description:
// Example link: 
// Prerequisites: 
//    You should install following libraries:
//		Install with Library Manager. "ArduinoJson by Benoit Blanchon" https://github.com/bblanchon/ArduinoJson
// Version: 1.3.0
// Start date: 13.09.2017
// Last version date: 10.10.2017
// Author: Plamen Kovandjiev <p.kovandiev@kmpelectronics.eu>

#include "FanCoilHelper.h"
#include <KMPDinoWiFiESP.h>       // Our library. https://www.kmpelectronics.eu/en-us/examples/prodinowifi-esp/howtoinstall.aspx
#include <KMPCommon.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>         // Install with Library Manager. "PubSubClient by Nick O'Leary" https://pubsubclient.knolleary.net/
#include <WiFiManager.h>          // Install with Library Manager. "WiFiManager by tzapu" https://github.com/tzapu/WiFiManager

DeviceSettings _settings;

WiFiClient _wifiClient;
PubSubClient _mqttClient;

// Text buffers for topic and payload.
char _topicBuff[128];
char _payloadBuff[32];

unsigned long _checkPingInterval;

bool _isConnected = false;
bool _isStarted = false;

void publishData(DeviceData deviceData, bool sendCurrent = false)
{
	_checkPingInterval = millis() + PING_INTERVAL_MS;

	if (!_isConnected || !_isStarted)
	{
		return;
	}

	if (CHECK_ENUM(deviceData, DoorState))
	{
		strConcatenate(_topicBuff, 3, _settings.BaseTopic, TOPIC_SEPARATOR, TOPIC_DOOR_STATE);

		const char * mode = KMPDinoWiFiESP.GetRelayState(DOOR_RELAY) ? PAYLOAD_ON : PAYLOAD_OFF;

		mqttPublish(_topicBuff, (char*)mode);
	}
	
	if (CHECK_ENUM(deviceData, DeviceIsReady))
	{
		mqttPublish(_settings.BaseTopic, (char*)PAYLOAD_READY);
	}

	if (CHECK_ENUM(deviceData, DevicePing))
	{
		mqttPublish(_settings.BaseTopic, (char*)PAYLOAD_PING);
	}
}

/**
* @brief Callback method. It is fire when has information in subscribed topics.
*
* @return void
*/
void callback(char* topic, byte* payload, unsigned int length) {
#ifdef WIFIFCMM_DEBUG
	printTopicAndPayload("Call back", topic, (char*)payload, length);
#endif

	size_t baseTopicLen = strlen(_settings.BaseTopic);

	if (!startsWith(topic, _settings.BaseTopic))
	{
		return;
	}

	// Processing base topic - command sends all data from the device.
	if (strlen(topic) == baseTopicLen && length == 0)
	{
		publishAllData();
		return;
	}

	// Remove prefix basetopic/
	removeStart(topic, baseTopicLen + 1);

	// All other topics finished with /set
	strConcatenate(_topicBuff, 2, TOPIC_SEPARATOR, TOPIC_SET);

	if (!endsWith(topic, _topicBuff))
	{
		return;
	}

	// Remove /set
	removeEnd(topic, strlen(_topicBuff));

	// Processing topic basetopic/doorstate/set: on, off
	if (isEqual(topic, TOPIC_DOOR_STATE))
	{
		if (isEqual((char*)payload, PAYLOAD_ON, length))
		{
			setDoorState(On);
		}

		if (isEqual((char*)payload, PAYLOAD_OFF, length))
		{
			setDoorState(Off);
		}

		return;
	}
}

/**
* @brief Execute first after start the device. Initialize hardware.
*
* @return void
*/
void setup(void)
{
	// You can open the Arduino IDE Serial Monitor window to see what the code is doing
	DEBUG_FC.begin(115200);
	// Init KMP ProDino WiFi-ESP board.
	KMPDinoWiFiESP.init();
	KMPDinoWiFiESP.SetAllRelaysOff();

	DEBUG_FC_PRINTLN(F("KMP flat door opener management with Mqtt.\r\n"));

	//WiFiManager
	//Local initialization. Once it's business is done, there is no need to keep it around.
	WiFiManager wifiManager;

	// Is OptoIn 4 is On the board is resetting WiFi configuration.
	if (KMPDinoWiFiESP.GetOptoInState(OptoIn4))
	{
		DEBUG_FC_PRINTLN(F("Resetting WiFi configuration..."));
		wifiManager.resetSettings();
		DEBUG_FC_PRINTLN(F("WiFi configuration was reseted."));
	}

	// Set save configuration callback.
	wifiManager.setSaveConfigCallback(saveConfigCallback);

	DEBUG_FC_PRINTLN(F("Waiting WiFi up..."));
	// If the Fan coil (device) starts together with WiFi, need time to initialize WiFi router.
	// During this time (60 seconds) device trying to connect to WiFi.
	int i = 0;
	while (!connectWiFi() && i++ < 12)
	{
		// Wait for 5 second before try again.
		delay(5000);
	}
	
	if (!mangeConnectParamers(&wifiManager, &_settings))
	{
		return;
	}

	// Initialize MQTT.
	_mqttClient.setClient(_wifiClient);
}

/**
* @brief Main method.
*
* @return void
*/
void loop(void)
{
	// For a normal work on device, need it be connected to WiFi and MQTT server.
	_isConnected = connectWiFi() && connectMqtt();

	if (!_isConnected)
	{
		return;
	}

	_mqttClient.loop();

	if (millis() > _checkPingInterval)
	{
		publishData(DevicePing);
	}

	if (!_isStarted)
	{
		_isStarted = true;
		publishData(DeviceIsReady);
	}
}

void setDoorState(DoorAction doorAction)
{
	if (KMPDinoWiFiESP.GetRelayState(DOOR_RELAY) != doorAction)
	{
		KMPDinoWiFiESP.SetRelayState(DOOR_RELAY, doorAction);
		publishData(DoorState);
	}
}

/**
* @brief Publish topic.
* @param topic A topic title.
* @param payload Data to send.
*
* @return void
*/
void mqttPublish(const char* topic, char* payload)
{
#ifdef WIFIFCMM_DEBUG
	printTopicAndPayload("Publish", topic, payload, strlen(payload));
#endif
	_mqttClient.publish(topic, (const char*)payload);
}

/**
* @brief Connect to MQTT server.
*
* @return bool true - success.
*/
bool connectMqtt()
{
	if (!_mqttClient.connected())
	{
		DEBUG_FC_PRINTLN(F("Trying to MQTT connect..."));

		uint16_t port = atoi(_settings.MqttPort);
		_mqttClient.setServer(_settings.MqttServer, port);
		_mqttClient.setCallback(callback);

		DEBUG_FC_PRINT(F("Server: \""));
		DEBUG_FC_PRINT(_settings.MqttServer);
		DEBUG_FC_PRINT(F("\"\r\nPort:\""));
		DEBUG_FC_PRINT(_settings.MqttPort);
		DEBUG_FC_PRINT(F("\"\r\nClientId:\""));
		DEBUG_FC_PRINT(_settings.MqttClientId);
		DEBUG_FC_PRINT(F("\"\r\nUser:\""));
		DEBUG_FC_PRINT(_settings.MqttUser);
		DEBUG_FC_PRINT(F("\"\r\nPassword:\""));
		DEBUG_FC_PRINT(_settings.MqttPass);
		DEBUG_FC_PRINTLN(F("\""));

		if (_mqttClient.connect(_settings.MqttClientId, _settings.MqttUser, _settings.MqttPass))
		{
			DEBUG_FC_PRINTLN(F("MQTT connected. Subscribe for topics:"));
			// Subscribe for topics:
			//  basetopic
			_mqttClient.subscribe(_settings.BaseTopic);
			DEBUG_FC_PRINTLN(_settings.BaseTopic);

			//  basetopic/+/set. This pattern include:  basetopic/mode/set, basetopic/desiredtemp/set, basetopic/state/set
			strConcatenate(_topicBuff, 5, _settings.BaseTopic, TOPIC_SEPARATOR, EVERY_ONE_LEVEL_TOPIC, TOPIC_SEPARATOR, TOPIC_SET);
			_mqttClient.subscribe(_topicBuff);
			DEBUG_FC_PRINTLN(_topicBuff);
		}
		else
		{
			DEBUG_FC_PRINT(F("failed, rc="));
			DEBUG_FC_PRINT(_mqttClient.state());
			DEBUG_FC_PRINTLN(F(" try again after 5 seconds"));
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}

	return _mqttClient.connected();
}

void publishAllData()
{
	DeviceData deviceData = (DeviceData)
		(DoorState);
	publishData(deviceData, false);
}