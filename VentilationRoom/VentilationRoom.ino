// VentilationRoom.ino
// Company: KMP Electronics Ltd, Bulgaria
// Web: http://kmpelectronics.eu/
// Supported boards:
//    KMP ProDino WiFi-ESP WROOM-02 (http://www.kmpelectronics.eu/en-us/products/prodinowifi-esp.aspx)
// Description:
// Example link: 
// Prerequisites: 
//    You should install following libraries:
//		Install with Library Manager. "ArduinoJson by Benoit Blanchon" https://github.com/bblanchon/ArduinoJson
// Version: 0.1.0
// Start date: 16.06.2019
// Last version date: 16.06.2019
// Author: Plamen Kovandjiev <p.kovandiev@kmpelectronics.eu>

#include "VentilationHelper.h"
#include "WindowOpener.h"
#include "MqttTopicHelper.h"
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

bool _isConnected = false;
bool _isStarted = false;
unsigned long _windowCloseIfNotConnectedInterval;

void publishData(DeviceData deviceData, bool sendCurrent = false)
{
	if (!_isConnected || !_isStarted)
	{
		return;
	}
/*
	if (CHECK_ENUM(deviceData, Temperature))
	{
		strConcatenate(_topicBuff, 3, _settings.BaseTopic, TOPIC_SEPARATOR, TOPIC_TEMPERATURE);

		mqttPublish(_topicBuff, valueToStr(&TemperatureData, sendCurrent));
	}

	if (CHECK_ENUM(deviceData, Humidity))
	{
		strConcatenate(_topicBuff, 3, _settings.BaseTopic, TOPIC_SEPARATOR, TOPIC_HUMIDITY);

		mqttPublish(_topicBuff, valueToStr(&HumidityData, sendCurrent));
	}

	if (CHECK_ENUM(deviceData, InletPipe))
	{
		strConcatenate(_topicBuff, 3, _settings.BaseTopic, TOPIC_SEPARATOR, TOPIC_INLET_TEMPERATURE);

		mqttPublish(_topicBuff, valueToStr(&InletData, sendCurrent));
	}

	if (CHECK_ENUM(deviceData, FanDegree))
	{
		strConcatenate(_topicBuff, 3, _settings.BaseTopic, TOPIC_SEPARATOR, TOPIC_FAN_DEGREE);
		IntToChars(_fanDegree, _payloadBuff);

		mqttPublish(_topicBuff, _payloadBuff);
	}

	if (CHECK_ENUM(deviceData, DesiredTemp))
	{
		strConcatenate(_topicBuff, 3, _settings.BaseTopic, TOPIC_SEPARATOR, TOPIC_DESIRED_TEMPERATURE);
		FloatToChars(_desiredTemperature, TEMPERATURE_PRECISION, _payloadBuff);

		mqttPublish(_topicBuff, _payloadBuff);
	}

	if (CHECK_ENUM(deviceData, CurrentMode))
	{
		strConcatenate(_topicBuff, 3, _settings.BaseTopic, TOPIC_SEPARATOR, TOPIC_MODE);

		const char * mode = _mode == Cold ? PAYLOAD_COLD : PAYLOAD_HEAT;

		mqttPublish(_topicBuff, (char*)mode);
	}

	if (CHECK_ENUM(deviceData, CurrentDeviceState))
	{
		strConcatenate(_topicBuff, 3, _settings.BaseTopic, TOPIC_SEPARATOR, TOPIC_DEVICE_STATE);

		const char * mode = _deviceState == On ? PAYLOAD_ON : PAYLOAD_OFF;

		mqttPublish(_topicBuff, (char*)mode);
	}
*/
	if (CHECK_ENUM(deviceData, DeviceIsReady))
	{
		mqttPublish(_settings.BaseTopic, (char*)PAYLOAD_READY);
	}

	if (CHECK_ENUM(deviceData, DeviceOk))
	{
		mqttPublish(_settings.BaseTopic, (char*)PAYLOAD_OK);
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

	// Processing topic basetopic/mode/set: heat/cold
	if (isEqual(topic, TOPIC_MODE))
	{
		setDeviceMode((char*)payload, length);
		return;
	}

	// Processing topic basetopic/desiredtemp/set: 22.5
	if (isEqual(topic, TOPIC_DESIRED_TEMPERATURE))
	{
		memcpy(_topicBuff, payload, length);
		_topicBuff[length] = CH_NONE;

		float temp = atof(_topicBuff);
		setDesiredTemperature(temp);

		return;
	}

	// Processing topic basetopic/state/set: on, off
	if (isEqual(topic, TOPIC_DEVICE_STATE))
	{
		if (isEqual((char*)payload, PAYLOAD_ON, length))
		{
			if (setDeviceState(On))
			{
				_lastDeviceState = On;
			}
		}

		if (isEqual((char*)payload, PAYLOAD_OFF, length))
		{
			if (setDeviceState(Off))
			{
				_lastDeviceState = Off;
			}
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
	// Init bypass.
	WindowOpener.init(OptoIn1, Relay1, Relay2, &publishData);

	DEBUG_FC_PRINTLN(F("KMP Ventilation room Mqtt application starting."));
	DEBUG_FC_PRINTLN(F(""));

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

	if (!manageConnectAndSettings(&wifiManager, &_settings))
	{
		return;
	}

	// Initialize MQTT helper
	MqttTopicHelper.init(_settings.BaseTopic, _settings.DeviceTopic, &Serial);

	// Initialize MQTT.
	_mqttClient.setClient(_wifiClient);

	// Close window.
	WindowOpener.setState(CloseWindow, true);
}

/**
* @brief Main method.
*
* @return void
*/
void loop(void)
{
	WindowOpener.processWindowState();

	// For a normal work on device, need it be connected to WiFi and MQTT server.
	_isConnected = connectWiFi() && connectMqtt();

	if (!_isConnected)
	{
		if (_windowCloseIfNotConnectedInterval == 0)
		{
			_windowCloseIfNotConnectedInterval = millis() + WAIT_FOR_CONNECTION_TIMEOUT_MS;
		}

		if (_windowCloseIfNotConnectedInterval > millis())
		{
			WindowOpener.setState(CloseWindow, true);
		}

		return;
	}

	_mqttClient.loop();

	if (!_isStarted)
	{
		_isStarted = true;
		publishData(DeviceIsReady);
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
		(CurrentDeviceState | WindowCurrentState);
	publishData(deviceData, false);
}