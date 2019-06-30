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
unsigned long _windowCloseIfNotConnectedInterval;

/**
* @brief This method publishes all data per device.
* @dataType Type of data which will be publish.
* @num device number, if need for publish this topic.
* @isPrintPublish is print Publish.
*
* @return void
*/
void publishTopic(DeviceData deviceData, int num = 0, bool isPrintPublish = true)
{
	if (!_isConnected)
	{
		return;
	}

	if (isPrintPublish)
	{
		DEBUG_FC_PRINTLN(F("Publish"));
	}

	const char * topic = NULL;
	const char * payload = NULL;
	char numBuff[8];

	switch (deviceData)
	{
	case AllData:
		// base_topic/device_name:NULL
		publishTopic(DeviceIsReady, 0, false);
		publishTopic(WindowState, 0, false);
		publishTopic(AllGatesState, 0, false);
		break;
	case BroadcastDevice:
		// base_topic:NULL
		topic = MqttTopicHelper.getMainTopic();
		payload = W_OK;
		break;
	case WindowState:
		// base_topic/device_name/window:NULL
		MqttTopicHelper.buildTopicWithMT(_topicBuff, 1, WINDOW_TOPIC);
		topic = _topicBuff;
		IntToChars(WindowOpener.getState(), numBuff);
		payload = numBuff;
		break;
	case GateState:
		IntToChars(num + 1, numBuff);
		// base_topic/device_name/gate/1:[open | close]
		MqttTopicHelper.buildTopicWithMT(_topicBuff, 2, GATE_TOPIC, numBuff);
		topic = _topicBuff;
		payload = KMPDinoWiFiESP.GetOptoInState(num) ? PAYLOAD_OPEN : PAYLOAD_CLOSE;
		break;
	case AllGatesState:
		for (size_t i = 0; i < OPTOIN_COUNT; i++)
			publishTopic(GateState, i, false);
		break;
	case DeviceIsReady:
		topic = MqttTopicHelper.getIsReadyTopic();
		payload = PAYLOAD_READY;
		break;
	default:
		break;
	}

	if (topic != NULL)
	{
		MqttTopicHelper.printTopicAndPayload(topic, payload);
		_mqttClient.publish(topic, payload);
	}
}

/**
* @brief Print in debug console Subscribed topic and payload.
*
* @return void
*/
void printSubscribeTopic(char* topic, byte* payload, unsigned int length)
{
	DEBUG_FC_PRINTLN(F("Subscribe"));
	MqttTopicHelper.printTopicAndPayload(topic, payload, length);
}

/**
* @brief Callback method. It executes when has information from subscribed topics: kmp and kmp/prodinomkrzero/#
*
* @return void
*/
void callback(char* topics, byte* payload, unsigned int payloadLen)
{
	bool payloadEmpty = payloadLen == 0;
	char* payloadCh = (char*)payload;
	payloadCh[payloadLen] = CH_NONE;

	// Broadcasting device: base_topic:NULL || base_topic/device_name:NULL
	if ((MqttTopicHelper.isBaseTopic(topics) || MqttTopicHelper.isMainTopic(topics)) && payloadEmpty)
	{
		printSubscribeTopic(topics, payload, payloadLen);
		publishTopic(BroadcastDevice);
		return;
	}

	// If the topic doesn't start with base_topic/device_name it doesn't need to do.
	if (!MqttTopicHelper.startsWithMainTopic(topics))
		return;

	// Publishing all information: base_topic/device_name:all
	if (MqttTopicHelper.isMainTopic(topics) && isEqual(payloadCh, PAYLOAD_ALL))
	{
		printSubscribeTopic(topics, payload, payloadLen);
		publishTopic(AllData);
		return;
	}

	char nextTopic[32];
	char* otherTopics = nullptr;
	// Get topic after  base_topic/device_name/...
	if (!MqttTopicHelper.getNextTopic(topics, nextTopic, &otherTopics, true))
		return;

	// window...
	if (isEqual(nextTopic, WINDOW_TOPIC))
	{
		// [window]:null
		if (otherTopics[0] == CH_NONE && payloadEmpty)
		{
			printSubscribeTopic(topics, payload, payloadLen);
			publishTopic(WindowState);
			return;
		}

		// window/set [0 (Closed), 1, 2, 3, 4 (Full opened)]
		if (MqttTopicHelper.isTopicSet(otherTopics) && !payloadEmpty)
		{
			int windowPos = atoi(payloadCh);
			if (windowPos >= CloseWindow && windowPos <= FullOpen)
			{
				printSubscribeTopic(topics, payload, payloadLen);
				WindowOpener.setState((WindowStateType)windowPos, windowPos == CloseWindow);
			}
		}
		return;
	}

	// gates:null
	if (isEqual(nextTopic, GATES_TOPIC) && otherTopics[0] == CH_NONE && payloadEmpty)
	{
		printSubscribeTopic(topics, payload, payloadLen);
		publishTopic(AllGatesState);
		return;
	}

	// gate/...
	if (isEqual(nextTopic, GATE_TOPIC) && payloadEmpty)
	{
		uint8_t inNum;
		// [gate] /1
		if (!MqttTopicHelper.getNextTopic(otherTopics, nextTopic, &otherTopics) || !atoUint8(nextTopic, inNum) || inNum <= 0 || inNum > OPTOIN_COUNT)
			return;

		if (otherTopics[0] == CH_NONE && payloadEmpty)
		{
			printSubscribeTopic(topics, payload, payloadLen);
			publishTopic(GateState, --inNum);
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
	DEBUG_FC.begin(115200);

	DEBUG_FC_PRINTLN(F(""));
	DEBUG_FC_PRINTLN(F("KMP Ventilation room Mqtt application starting."));
	DEBUG_FC_PRINTLN(F(""));

	// Init KMP ProDino WiFi-ESP board.
	KMPDinoWiFiESP.init();
	KMPDinoWiFiESP.SetAllRelaysOff();

	// You can open the Arduino IDE Serial Monitor window to see what the code is doing

	// Init bypass.
	WindowOpener.init(OptoIn1, Relay1, Relay2, &publishTopic);

	DEBUG_FC_PRINTLN(F("WiFiManager starting..."));
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

	DEBUG_FC_PRINTLN(F("Manage connect and settings..."));
	if (!manageConnectAndSettings(&wifiManager, &_settings))
	{
		return;
	}

	// Initialize MQTT helper
	MqttTopicHelper.init(_settings.BaseTopic, _settings.DeviceTopic, &Serial);

	// Initialize MQTT.
	_mqttClient.setClient(_wifiClient);
	uint16_t port = atoi(_settings.MqttPort);
	_mqttClient.setServer(_settings.MqttServer, port);
	_mqttClient.setCallback(callback);

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

	_windowCloseIfNotConnectedInterval = 0;

	_mqttClient.loop();
}

/**
* @brief Connect to MQTT server.
*
* @return bool true - success.
*/
bool connectMqtt()
{
	if (_mqttClient.connected())
	{
		return true;
	}

	DEBUG_FC_PRINTLN(F("Attempting to connect MQTT..."));

	DEBUG_FC_PRINT("MqttClientId: \"");
	DEBUG_FC_PRINT(_settings.MqttClientId);
	DEBUG_FC_PRINT("\", MqttUser: \"");
	DEBUG_FC_PRINT(_settings.MqttUser);
	DEBUG_FC_PRINT("\", MqttPass: ");
	DEBUG_FC_PRINT(_settings.MqttPass);
	DEBUG_FC_PRINT("\"");

	if (_mqttClient.connect(_settings.MqttClientId, _settings.MqttUser, _settings.MqttPass))
	{
		DEBUG_FC_PRINTLN(F("MQTT connected. Subscribe for topics:"));
		// Subscribe for topics:
		//  base_topic
		_mqttClient.subscribe(_settings.BaseTopic);
		DEBUG_FC_PRINTLN(_settings.BaseTopic);

		// Building topic with wildcard symbol: base_topic/device_name/#
		// With this topic we are going to subscribe for all topics per device. All topics started with: base_topic/device_name
		MqttTopicHelper.buildTopicWithMT(_topicBuff, 1, "#");
		_mqttClient.subscribe(_topicBuff);
		DEBUG_FC_PRINTLN(_topicBuff);

		DEBUG_FC_PRINTLN(F("Connected."));
		publishTopic(DeviceIsReady);

		return true;
	}

	DEBUG_FC_PRINT(F("failed, rc="));
	DEBUG_FC_PRINT(_mqttClient.state());
	DEBUG_FC_PRINTLN(F(" try again after 5 seconds"));
	// Wait 5 seconds before retrying
	delay(5000);

	return false;
}
