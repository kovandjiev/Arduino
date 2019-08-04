#include "VentilationHelper.h"
#include "KMPCommon.h"
#include <ArduinoJson.h>          // Install with Library Manager. "ArduinoJson by Benoit Blanchon" https://github.com/bblanchon/ArduinoJson

bool _shouldSaveConfig = false;

/**
* @brief Connect to WiFi access point.
*
* @return bool true - success.
*/
bool connectWiFi()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		DEBUG_FC_PRINT(F("Connecting ["));
		DEBUG_FC_PRINT(WiFi.SSID());
		DEBUG_FC_PRINTLN(F("]..."));

		WiFi.begin();

		if (WiFi.waitForConnectResult() != WL_CONNECTED)
		{
			return false;
		}

		DEBUG_FC_PRINT(F("IP address: "));
		DEBUG_FC_PRINTLN(WiFi.localIP());
	}

	return true;
}

void copyJsonValue(char* s, const char* jsonValue)
{
	if (jsonValue == NULL)
	{
		s[0] = '\0';
	}

	size_t len = strlen(jsonValue);

	strncpy(s, jsonValue, len);

	s[len] = '\0';
}

void ReadConfiguration(DeviceSettings* settings)
{
	if (!SPIFFS.begin())
	{
		DEBUG_FC_PRINTLN("Failed to mount FS");
	}
	else
	{
		DEBUG_FC_PRINTLN("The file system is mounted.");

		if (SPIFFS.exists(CONFIG_FILE_NAME))
		{
			//file exists, reading and loading
			DEBUG_FC_PRINTLN("Reading configuration file");
			File configFile = SPIFFS.open(CONFIG_FILE_NAME, "r");
			if (configFile)
			{
				DEBUG_FC_PRINTLN("Opening configuration file");
				size_t size = configFile.size();
				// Allocate a buffer to store contents of the file.
				std::unique_ptr<char[]> buf(new char[size]);

				configFile.readBytes(buf.get(), size);
				DynamicJsonDocument json(1024);

				DeserializationError error = deserializeJson(json, buf.get());

				if (!error)
				{
#ifdef WIFIFCMM_DEBUG
					serializeJsonPretty(json, DEBUG_FC);
#endif

					DEBUG_FC_PRINTLN("\nJson is parsed");

					copyJsonValue(settings->MqttServer, json[MQTT_SERVER_KEY]);
					copyJsonValue(settings->MqttPort, json[MQTT_PORT_KEY]);
					copyJsonValue(settings->MqttClientId, json[MQTT_CLIENT_ID_KEY]);
					copyJsonValue(settings->MqttUser, json[MQTT_USER_KEY]);
					copyJsonValue(settings->MqttPass, json[MQTT_PASS_KEY]);
					copyJsonValue(settings->BaseTopic, json[BASE_TOPIC_KEY]);
					copyJsonValue(settings->DeviceTopic, json[DEVICE_TOPIC_KEY]);
				}
				else
				{
					DEBUG_FC_PRINTLN(F("Loading json configuration is failed"));
				}
			}
		}
	}
}

/**
* @brief Setting information for connect WiFi and MQTT server. After successful connected this method save them.
* @param wifiManager.
*
* @return bool if successful connected - true else false.
*/
bool manageConnectAndSettings(WiFiManager* wifiManager, DeviceSettings* settings)
{
	//read configuration from FS json
	DEBUG_FC_PRINTLN(F("Mounting FS..."));

	ReadConfiguration(settings);

	// The extra parameters to be configured (can be either global or just in the setup)
	// After connecting, parameter.getValue() will get you the configured value
	// id/name placeholder/prompt default length
	WiFiManagerParameter customMqttServer("server", "MQTT server", settings->MqttServer, MQTT_SERVER_LEN);
	WiFiManagerParameter customMqttPort("port", "MQTT port", settings->MqttPort, MQTT_PORT_LEN);
	WiFiManagerParameter customClientName("clientName", "Client name", settings->MqttClientId, MQTT_CLIENT_ID_LEN);
	WiFiManagerParameter customMqttUser("user", "MQTT user", settings->MqttUser, MQTT_USER_LEN);
	WiFiManagerParameter customMqttPass("password", "MQTT pass", settings->MqttPass, MQTT_PASS_LEN);
	WiFiManagerParameter customBaseTopic("baseTopic", "Base topic", settings->BaseTopic, BASE_TOPIC_LEN);
	WiFiManagerParameter customDevicTopic("deviceTopic", "Device topic", settings->DeviceTopic, DEVICE_TOPIC_LEN);

	// add all your parameters here
	wifiManager->addParameter(&customMqttServer);
	wifiManager->addParameter(&customMqttPort);
	wifiManager->addParameter(&customClientName);
	wifiManager->addParameter(&customMqttUser);
	wifiManager->addParameter(&customMqttPass);
	wifiManager->addParameter(&customBaseTopic);
	wifiManager->addParameter(&customDevicTopic);

	DEBUG_FC_PRINTLN(F("Waiting WiFi up..."));

	// If the Fan coil (device) starts together with WiFi, need time to initialize WiFi router.
	// During this time (60 seconds) device trying to connect to WiFi.
	wifiManager->setTimeout(60);

	// fetches ssid and pass from eeprom and tries to connect
	// if it does not connect it starts an access point with the specified name
	// auto generated name ESP + ChipID
	if (!wifiManager->autoConnect())
	{
		DEBUG_FC_PRINTLN(F("Failed to connect and hit timeout"));
		return false;
	}

	//if you get here you have connected to the WiFi
	DEBUG_FC_PRINTLN("WiFi is connected.");

	if (_shouldSaveConfig)
	{
		//read updated parameters
		strcpy(settings->MqttServer, customMqttServer.getValue());
		strcpy(settings->MqttPort, customMqttPort.getValue());
		strcpy(settings->MqttClientId, customClientName.getValue());
		strcpy(settings->MqttUser, customMqttUser.getValue());
		strcpy(settings->MqttPass, customMqttPass.getValue());
		strcpy(settings->BaseTopic, customBaseTopic.getValue());
		strcpy(settings->DeviceTopic, customDevicTopic.getValue());

		SaveConfiguration(settings);
	}

	return true;
}

void SaveConfiguration(DeviceSettings* settings)
{
	DEBUG_FC_PRINTLN(F("Saving configuration..."));

	DynamicJsonDocument json(1024);

	json[MQTT_SERVER_KEY] = settings->MqttServer;
	json[MQTT_PORT_KEY] = settings->MqttPort;
	json[MQTT_CLIENT_ID_KEY] = settings->MqttClientId;
	json[MQTT_USER_KEY] = settings->MqttUser;
	json[MQTT_PASS_KEY] = settings->MqttPass;
	json[BASE_TOPIC_KEY] = settings->BaseTopic;
	json[DEVICE_TOPIC_KEY] = settings->DeviceTopic;

	File configFile = SPIFFS.open(CONFIG_FILE_NAME, "w");
	if (!configFile) 
	{
		DEBUG_FC_PRINTLN(F("Failed to open a configuration file for writing."));
		return;
	}

#ifdef WIFIFCMM_DEBUG
	serializeJson(json, DEBUG_FC);
#endif

	serializeJson(json, configFile);
	configFile.close();

	DEBUG_FC_PRINTLN(F("Configuration is saved."));
}

/**
* @brief Callback notifying us of the need to save configuration set from WiFiManager.
*
* @return void.
*/
void saveConfigCallback()
{
	DEBUG_FC_PRINTLN("Should save configuration");
	_shouldSaveConfig = true;
}