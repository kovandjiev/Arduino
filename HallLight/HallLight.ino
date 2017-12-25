/*
HallLight.ino
Hall light project is for fade switch on or off lights in my hall.
*/
#define CHECK_INPUTS_LEN 10
#define DEBUG

// These pins check for change state low, high.
const byte CHECK_INPUTS[CHECK_INPUTS_LEN] = { A0, 15, 14, 16, 10, 2, 3, 4, 5, 6 };
const byte NO_POWER_INPUT = 7;
const byte NIGHT_INPUT = 8;
const byte DIMMER_OUTPUT = 9;
const byte LIGHTS_IS_ON = A1;

const int MAX_BRIGHTNESS = 255;
const int MAX_BRIGHTNESS_NIGHT = 85;
const int MAX_BRIGHTNESS_NO_POWER = 127;
const unsigned long DELAY_LIGHTS_ON_MS = 60000; //5000; // 60000 One minute is on
const unsigned long FADE_DELAY_ON_MS = 6; // 30
const unsigned long FADE_DELAY_OFF_MS = 100; // 100
const int FADE_POINTS = 1;    // 5 how many points to fade the LED by

bool _inputState[CHECK_INPUTS_LEN];

int _brightness = 0;    // how bright the LED is
unsigned long _lightDelay = 0;
unsigned long _fadeDelay = 0;

void setup() 
{
	Serial.begin(57600);

#ifdef DEBUG
	Serial.println("Hall light project is started");
#endif // DEBUG

	for (byte i = 0; i < CHECK_INPUTS_LEN; i++)
	{
		pinMode(CHECK_INPUTS[i], INPUT);

		// Initialize compare array.
		_inputState[i] = digitalRead(CHECK_INPUTS[i]);
	}
	
	pinMode(NO_POWER_INPUT, INPUT);
	pinMode(NIGHT_INPUT, INPUT);

	pinMode(DIMMER_OUTPUT, OUTPUT);
	pinMode(LIGHTS_IS_ON, OUTPUT);
}

void loop() 
{
	bool inState = processInputs();

	bool lighState = processState(inState);

	int maxBrightness = getMaxBrightness();

	int newBrightness = processBrightness(lighState, _brightness, maxBrightness);

	setBrightness(newBrightness);

	digitalWrite(LIGHTS_IS_ON, _brightness > 0 ? HIGH : LOW);
}

void setBrightness(int brightness)
{
	if (_brightness != brightness)
	{
		analogWrite(DIMMER_OUTPUT, brightness);

		_brightness = brightness;
#ifdef DEBUG
		Serial.println(brightness);
#endif // DEBUG
	}
}

int processBrightness(bool lighState, int currentBrightness, int maxBrightness)
{
	unsigned long time = millis();

	if (_fadeDelay > time)
	{
		return currentBrightness;
	}

	int newBrightness = currentBrightness;

	if (newBrightness > maxBrightness)
	{
		newBrightness = maxBrightness;
	}
	
	if (lighState) // On
	{
		if (newBrightness < maxBrightness)
		{
			newBrightness += FADE_POINTS;
			if (newBrightness > maxBrightness)
			{
				newBrightness = maxBrightness;
			}

			_fadeDelay = time + FADE_DELAY_ON_MS;
			_lightDelay = time + DELAY_LIGHTS_ON_MS;
		}
	}
	else // Off
	{
		if (newBrightness > 0)
		{
			newBrightness -= FADE_POINTS;
			if (newBrightness < 0)
			{
				newBrightness = 0;
			}

			_fadeDelay = time + FADE_DELAY_OFF_MS;
		}
	}

	return newBrightness;
}

bool processState(bool inState)
{
	unsigned long time = millis();

	if (inState)
	{
		_lightDelay = time + DELAY_LIGHTS_ON_MS;
		_fadeDelay = 0;
	}

	return _lightDelay > time;
}

int getMaxBrightness()
{
	// Night arrived - active High
	if (digitalRead(NIGHT_INPUT) == HIGH)
	{
		return MAX_BRIGHTNESS_NIGHT;
	}

	// No main power - active Low
	if (digitalRead(NO_POWER_INPUT) == LOW)
	{
		return MAX_BRIGHTNESS_NO_POWER;
	}

	return MAX_BRIGHTNESS;
}

bool processInputs()
{
	bool result = false;

	for (byte i = 0; i < CHECK_INPUTS_LEN; i++)
	{
		bool inputState = digitalRead(CHECK_INPUTS[i]);
		
		if (inputState != _inputState[i])
		{
#ifdef DEBUG
			Serial.print(CHECK_INPUTS[i]);
			Serial.print(" Input is changed: ");
			Serial.println(_inputState[i]);
#endif // DEBUG
			_inputState[i] = inputState;
			result = true;
		}
	}

	return result;
}