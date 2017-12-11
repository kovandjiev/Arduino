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

const int MAX_BRIGHTNESS = 255;
const int MAX_BRIGHTNESS_NIGHT = 85;
const int MAX_BRIGHTNESS_NO_POWER = 127;
const unsigned long DELAY_LIGHTS_ON_MS = 5000; // 60000 One minute is on
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

	for (byte i = 0; i < CHECK_INPUTS_LEN; i++)
	{
		pinMode(CHECK_INPUTS[i], INPUT);

		// Initialize compare array.
		_inputState[i] = digitalRead(CHECK_INPUTS[i]);
	}
	
	pinMode(NO_POWER_INPUT, INPUT);
	pinMode(NIGHT_INPUT, INPUT);

	pinMode(DIMMER_OUTPUT, OUTPUT);
}

void loop() 
{
	bool inState = processInputs();

	bool lighState = processState(inState);
	int maxBrightness = getMaxBrightness();

	int brightness = processBrightness(lighState, maxBrightness);

	setBrightness(brightness);
	
	//analogWrite(DIMMER_OUTPUT, _brightness);

	// change the brightness for next time through the loop:
	//_brightness += _fadeAmount;

	//// reverse the direction of the fading at the ends of the fade:
	//if (brightness <= 0 || brightness >= 255) {
	//	fadeAmount = -fadeAmount;
	//}
	//// wait for 30 milliseconds to see the dimming effect
	//delay(30);
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

int processBrightness(bool lighState, int maxBrightness)
{
	unsigned long time = millis();

	if (_fadeDelay > time)
	{
		return;
	}

	int brightness = _brightness;

	if (lighState) // On
	{
		if (brightness < maxBrightness)
		{
			brightness += FADE_POINTS;
			if (brightness > maxBrightness)
			{
				brightness = maxBrightness;
			}

			_fadeDelay = time + FADE_DELAY_ON_MS;
			_lightDelay = time + DELAY_LIGHTS_ON_MS;
		}
	}
	else // Off
	{
		if (brightness > 0)
		{
			brightness -= FADE_POINTS;
			if (brightness < 0)
			{
				brightness = 0;
			}

			_fadeDelay = time + FADE_DELAY_OFF_MS;
		}
	}

	return brightness;
}

bool processState(bool inState)
{
	unsigned long time = millis();

	if (inState)
	{
#ifdef DEBUG
		Serial.println("State changed");
#endif // DEBUG
		_lightDelay = time + DELAY_LIGHTS_ON_MS;
		_fadeDelay = 0;
	}

	return _lightDelay > time;
}

int getMaxBrightness()
{
	int maxBrightnes = MAX_BRIGHTNESS;

	// Night arrived - active High
	if (digitalRead(NIGHT_INPUT) == HIGH)
	{
		maxBrightnes = MAX_BRIGHTNESS_NIGHT;
		return;
	}

	// No main power - active Low
	if (digitalRead(NO_POWER_INPUT) == LOW)
	{
		maxBrightnes = MAX_BRIGHTNESS_NO_POWER;
		return;
	}

	return maxBrightnes;
}

bool processInputs()
{
	bool result = false;

	for (byte i = 0; i < CHECK_INPUTS_LEN; i++)
	{
		bool inputState = digitalRead(CHECK_INPUTS[i]);
		
		if (inputState != _inputState[i])
		{
			_inputState[i] = inputState;
			result = true;
		}
	}

	return result;
}