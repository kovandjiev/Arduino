// Discharger.ino
// Company: KMP Electronics Ltd, Bulgaria
// Web: http://kmpelectronics.eu/
// Supported boards:
//    KMP ProDino WiFi-ESP WROOM-02 (http://www.kmpelectronics.eu/en-us/products/prodinowifi-esp.aspx)
// Description:
// Example link: 
// Prerequisites: 
// Version: 1.0.0
// Start date: 25.02.2019
// Last version date: 10.10.2017
// Author: Plamen Kovandjiev <p.kovandiev@kmpelectronics.eu>
// --------------------------------------------------------------------------------
// Prerequisites:
//	Before start this example you need to install (Sketch\Include library\Menage Libraries... find ... and click Install):
//         - PubSubClient by Nick O'Leary
//		Connect maesuring cable to internal GROVE connector:
//			- INT_GROVE_A0 (+), Gnd(-);
//  You should have account in https://www.cloudmqtt.com/ or https://www.cloudamqp.com/ or other MQTT server (your RaspberryPI for example)
// --------------------------------------------------------------------------------
// Topics* (these topics we send to MQTT server):
//   kmp/prodinomkrzero:[] - the device publishes all data from device:
//     kmp/prodinomkrzero/isready:[OK]
//     kmp/prodinomkrzero/relay/1:[off] kmp/prodinomkrzero/relay/2:[off] kmp/prodinomkrzero/relay/3:[off] kmp/prodinomkrzero/relay/4:[off]
//     kmp/prodinomkrzero/input/1:[off] kmp/prodinomkrzero/input/2:[off] kmp/prodinomkrzero/input/3:[off] kmp/prodinomkrzero/input/4:[off]
//     kmp/prodinomkrzero/temperature/1[22.0] kmp/prodinomkrzero/humidity/1[50]
//   kmp/prodinomkrzero/relay/1:[] - the device publishes data per relay 1:
//     kmp/prodinomkrzero/relay/1:[]
//   kmp/prodinomkrzero/relay/1/set:[on] - we use this topic to set relay 1..4 status payload could be "on" or "off":
//     kmp/prodinomkrzero/relay/1:[on]
//   kmp/prodinomkrzero/temperature:[] - the device publishes data from first temperature sensor:
//     kmp/prodinomkrzero/temperature/1:[22.0]
//   kmp/prodinomkrzero/humidity:[] - the device publishes data from first humidity sensor:
//     kmp/prodinomkrzero/humidity/1:[50]
// *Legend: every message includes topic (as string) and payload (as binary array). 
//  By easy describe them we use following pattern: "topic:[payload]". If payload was empty we use [].

#include <KMPDinoWiFiESP.h>       // Our library. https://www.kmpelectronics.eu/en-us/examples/prodinowifi-esp/howtoinstall.aspx
#include <KMPCommon.h>
#include <ESP8266WiFi.h>
#include <MqttTopicHelper.h>
#include <PubSubClient.h>


enum DataType {
	AllData, DischargeState, Voltage, CutOffVoltage, DischargeTime
};

// Wifi authentication settings.
const char* SSID = "SweetHome2F";
const char* SSID_PASSWORD = "kS3#%[h?g;U";

// !! ------------- Change number -------------------- !!
const char* MQTT_CLIENT_ID = "DischargeClient2";
const char* DEVICE_TOPIC = "discharger2"; // Current device name

// MQTT server settings. 
const char* MQTT_SERVER = "192.168.1.6"; // Change it with yours data.
const int MQTT_PORT = 1883; // Change it with yours data.
const char* MQTT_USER = "lqnxfvdl"; // Change it with yours data.
const char* MQTT_PASS = "p4IT?P;n#3fn"; // Change it with yours data.

// Calibration - Max voltage to show 1024.
const double MaxVoltage = 14.7;
const double VoltageCoeficient = MaxVoltage / 1024;
const double CUTOFF_VOLTAGE_CORRECTION = 0.2;
const double CUTOFF_VOLTAGE = 6.0;

const char* BASE_TOPIC = "kmp"; // Base topic for all devices in this network. It can use for broadcast devices
const char* DISCHARGE_STATE_TOPIC = "state";
const char* DISCHARGE_TIME_TOPIC = "time";
const char* CURRENT_VOLTAGE_TOPIC = "currentvoltage";
const char* CUTOFF_VOLTAGE_TOPIC = "cutoffvoltage";

// Declares a ESP8266WiFi client.
WiFiClient _wifiClient;
// Declare a MQTT client.
PubSubClient _mqttClient(MQTT_SERVER, MQTT_PORT, _wifiClient);

// There arrays store last states by relay and optical isolated inputs.
bool _lastRelayStatus[4] = { false };

const long PROCESS_INTERVAL_MS = 500;
unsigned long _processTimeout = 0;
const long PUBLISH_CURRENT_INTERVAL_MS = 5000;
unsigned long _publishCurrentTimeout = 0;
const long WAIT_BEFORE_PROCESS_INTERVAL_MS = 1500;

// A buffer to send output information.
char _topicBuff[128];

unsigned long _startDischarge = 0;
unsigned long _stopDischarge = 0;
double _currentVoltage = 0;
double _cutOffVoltage = 0;
double _lastVoltage = 0;
bool _isDischarge = false;
bool _lastDischargeStatus = false;

/**
* @brief Execute first after start the device. Initialize hardware.
*
* @return void
*/
void setup(void)
{
	delay(5000);
	Serial.begin(115200);
	// Init KMP ProDino WiFi-ESP board.
	KMPDinoWiFiESP.init();
	KMPDinoWiFiESP.SetAllRelaysOff();

	// Initialize MQTT helper
	MqttTopicHelper.init(BASE_TOPIC, DEVICE_TOPIC, &Serial);

	// Set MQTT callback method
	_mqttClient.setCallback(callback);

	Serial.println(F("KMP discharger through Mqtt.\r\n"));
}

/**
* @brief Main method.
*
* @return void
*/
void loop(void)
{
	Process();

	// Checking is device connected to MQTT server.
	if (!ConnectWiFi() || !ConnectMqtt())
	{
		return;
	}

	_mqttClient.loop();

	PublishChangedData();
}

/**
* @brief This method publishes all data per device.
* @dataType Type of data which will be publish.
* @num device number, if need for publish this topic.
* @isPrintPublish is print Publish.
*
* @return void
*/
void publishTopic(DataType dataType, bool isPrintPublish = true)
{
	if (isPrintPublish)
	{
		Serial.println("Publish");
	}

	const char * topic = NULL;
	const char * payload = NULL;
	char payloadBuff[32];
	unsigned long time = 0;

	switch (dataType)
	{
		case AllData:
			// kmp/discharger:NULL
			publishTopic(DischargeState, false);
			publishTopic(Voltage, false);
			publishTopic(DischargeTime, false);
			publishTopic(CutOffVoltage, false);
			break;
		case DischargeState:
			// kmp/discharger/state:On
			MqttTopicHelper.buildTopicWithMT(_topicBuff, 1, DISCHARGE_STATE_TOPIC);
			topic = _topicBuff;
			payload = _isDischarge ? W_ON_S : W_OFF_S;
			break;
		case Voltage:
			// kmp/discharger/currentvoltage:7.2
			MqttTopicHelper.buildTopicWithMT(_topicBuff, 1, CURRENT_VOLTAGE_TOPIC);
			topic = _topicBuff;
			FloatToChars(_currentVoltage, 1, payloadBuff);
			payload = payloadBuff;
			break;
		case CutOffVoltage:
			// kmp/discharger/cutoffvoltage:7.2
			MqttTopicHelper.buildTopicWithMT(_topicBuff, 1, CUTOFF_VOLTAGE_TOPIC);
			topic = _topicBuff;
			FloatToChars(_cutOffVoltage, 1, payloadBuff);
			payload = payloadBuff;
			break;
		case DischargeTime:
			// kmp/prodinomkrzero/time:1234
			MqttTopicHelper.buildTopicWithMT(_topicBuff, 1, DISCHARGE_TIME_TOPIC);
			topic = _topicBuff;
			if (_isDischarge)
				time = millis() - _startDischarge;
			else
				time = _stopDischarge - _startDischarge;
			// Convert in seconds
			time = time / 1000;
			ultoa(time, payloadBuff, 10);
			payload = payloadBuff;
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

void Process()
{
	if (millis() > _processTimeout)
	{
		int value = analogRead(INT_GROVE_A0);
		_currentVoltage = roundF(value * VoltageCoeficient, 2);

		if (_currentVoltage <= (CUTOFF_VOLTAGE - CUTOFF_VOLTAGE_CORRECTION))
		{
			if (_isDischarge)
			{
				_isDischarge = false;
			}
		}

		Serial.print("ADC: ");
		Serial.print(value);
		Serial.print(" Voltage: ");
		Serial.println(_currentVoltage);

		_processTimeout = millis() + PROCESS_INTERVAL_MS;
	}
}

/**
* @brief Publishing information for sensors if they changed.
*
* @return void
*/
void PublishChangedData()
{
	if (millis() > _publishCurrentTimeout)
	{
		if(_lastVoltage != _currentVoltage)
		{
			_lastVoltage = _currentVoltage;
			publishTopic(Voltage);
		}

		// Set next time to read data.
		_publishCurrentTimeout = millis() + PUBLISH_CURRENT_INTERVAL_MS;
	}
	
	if (_lastDischargeStatus != _isDischarge)
	{
		_lastDischargeStatus = _isDischarge;

		if (_isDischarge)
		{ 
			_startDischarge = millis();
			_cutOffVoltage = 0;

			_processTimeout = millis() + WAIT_BEFORE_PROCESS_INTERVAL_MS;
		}
		else
		{
			_stopDischarge = millis();
			_cutOffVoltage = _currentVoltage;
		}

		KMPDinoWiFiESP.SetRelayState(Relay1, _isDischarge);
		KMPDinoWiFiESP.SetRelayState(Relay2, _isDischarge);

		publishTopic(AllData);
	}
}

/**
* @brief Print in debug console Subscribed topic and payload.
*
* @return void
*/
void printSubscribeTopic(char* topic, byte* payload, unsigned int length)
{
	Serial.println("Subscribe");
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

	// If the topic doesn't start with kmp/discharger it doesn't need to do.
	if (!MqttTopicHelper.startsWithMainTopic(topics))
		return;

	// Publishing all information: kmp/discharger:[]
	if (MqttTopicHelper.isMainTopic(topics) && payloadEmpty)
	{
		printSubscribeTopic(topics, payload, payloadLen);
		publishTopic(AllData);
		return;
	}

	char nextTopic[32];
	char* otherTopics = nullptr;
	// Get topic after  kmp/discharger/...
	if (!MqttTopicHelper.getNextTopic(topics, nextTopic, &otherTopics, true))
		return;

	// state
	if (isEqual(nextTopic, DISCHARGE_STATE_TOPIC))
	{
		// state/set
		if (MqttTopicHelper.isTopicSet(otherTopics) && !payloadEmpty)
		{
			char payloadStr[128];
			strNCopy(payloadStr, (const char*)payload, payloadLen > 128 ? 128 : payloadLen);

			// Checking payload is On or Off.
			bool isOn = isEqual(payloadStr, W_ON_S);
			if (isOn || isEqual(payloadStr, W_OFF_S))
			{
				printSubscribeTopic(topics, payload, payloadLen);

				_isDischarge = isOn;

				publishTopic(DischargeState);
			}
		}
		return;
	}
}

/**
* @brief Connect to WiFi access point.
*
* @return bool true - success.
*/
bool ConnectWiFi()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.print("Connecting [");
		Serial.print(SSID);
		Serial.println("]...");

		WiFi.begin(SSID, SSID_PASSWORD);

		if (WiFi.waitForConnectResult() != WL_CONNECTED)
		{
			return false;
		}

		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
	}

	return true;
}

/**
* @brief Checking if device connected to MQTT server. When it connects add subscribe topics per device.
*
* @return void
*/
bool ConnectMqtt()
{
	if (_mqttClient.connected())
	{
		return true;
	}

	Serial.println("Attempting to connect MQTT...");

	if (_mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS))
	{
		// It is broadcast topic: kmp
		_mqttClient.subscribe(BASE_TOPIC);
		// Building topic with wildcard symbol: kmp/prodinomkrzero/#
		// With this topic we are going to subscribe for all topics per device. All topics started with: kmp/prodinomkrzero
		MqttTopicHelper.buildTopicWithMT(_topicBuff, 1, "#");
		_mqttClient.subscribe(_topicBuff);

		Serial.println("Connected.");

		return true;
	}

	Serial.print("failed, rc=");
	Serial.print(_mqttClient.state());
	Serial.println(" try again after 5 seconds...");
	// Wait 5 seconds before retrying
	delay(5000);

	return false;
}