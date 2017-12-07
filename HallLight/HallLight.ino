/*
HallLight.ino
Hall light project is for fade switch on or off lights in my hall.
*/
#define CHECK_INPUTS_LEN 10

// These pins check for change state low, high.
const byte CHECK_INPUTS[CHECK_INPUTS_LEN] = { A0, 15, 14, 16, 10, 2, 3, 4, 5, 6 };
const byte NO_POWER_INPUT = 7;
const byte NIGHT_INPUT = 8;
const byte DIMMER_OUTPUT = 9;

const int MAX_BRIGHTNESS = 255;
const int MAX_BRIGHTNESS_NIGHT = 85;
const int MAX_BRIGHTNESS_NO_POWER = 127;
const unsigned long DELAY_LIGHTS_ON_MS = 10000; // One minute is on
const unsigned long FADE_DELAY_ON_MS = 30;
const unsigned long FADE_DELAY_OFF_MS = 100;
const int FADE_POINTS = 5;    // how many points to fade the LED by

bool _inputState[CHECK_INPUTS_LEN];

int _brightness = 0;    // how bright the LED is
unsigned long _lightDelay = 0;
unsigned long _fadeDelay = 0;
int _maxBrightnes;

void setup() 
{
	Serial.begin(115200);

	for (byte i = 0; i < CHECK_INPUTS_LEN; i++)
	{
		pinMode(CHECK_INPUTS[i], INPUT_PULLUP);

		// Initialize compare array.
		_inputState[i] = digitalRead(CHECK_INPUTS[i]);
	}
	
	pinMode(NO_POWER_INPUT, INPUT_PULLUP);
	pinMode(NIGHT_INPUT, INPUT_PULLUP);

	pinMode(DIMMER_OUTPUT, OUTPUT);
}

void loop() 
{
	setMaxBrightness();

	bool inState = processInputs();

	procesBrightness(inState);
	
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

void procesBrightness(bool state)
{
	unsigned long time = millis();

	if (state)
	{
		Serial.println("State changed");
		_lightDelay = time + DELAY_LIGHTS_ON_MS;
		Serial.print("_lightDelay ");
		Serial.println(_lightDelay);
		Serial.print("time ");
		Serial.println(time);
		_fadeDelay = 0;
	}

	bool isOn = _lightDelay > time;

	if (isOn)
	{
		if (_fadeDelay < time && _brightness < _maxBrightnes)
		{
			Serial.print("On ");
			_brightness += FADE_POINTS;
			_fadeDelay = time + FADE_DELAY_ON_MS;
			Serial.println(_brightness);
		}
	}
	else
	{
		if (_fadeDelay < time && _brightness > 0)
		{
			Serial.print("Off ");
			_brightness -= FADE_POINTS;
			_fadeDelay = time + FADE_DELAY_OFF_MS;
			Serial.println(_brightness);
		}
	}

	if (_brightness > _maxBrightnes)
	{
		_brightness = _maxBrightnes;
	}

	analogWrite(DIMMER_OUTPUT, _brightness);
}

void setMaxBrightness()
{
	if (digitalRead(NIGHT_INPUT) == HIGH)
	{
		_maxBrightnes = MAX_BRIGHTNESS_NIGHT;
		
		return;
	}

	if (digitalRead(NO_POWER_INPUT) == HIGH)
	{
		_maxBrightnes = MAX_BRIGHTNESS_NO_POWER;

		return;
	}

	_maxBrightnes = MAX_BRIGHTNESS;
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