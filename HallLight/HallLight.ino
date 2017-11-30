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
const int DELAY_LIGHTS_ON_MS = 60000; // One minute is on

bool _inputState[CHECK_INPUTS_LEN];

int _brightness = 0;    // how bright the LED is
int _fadeAmount = 5;    // how many points to fade the LED by
unsigned long _onLighRemaining = 0;

void setup() 
{
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

void loop() {
	bool inState = processInputs();

	analogWrite(DIMMER_OUTPUT, 255);
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
	if (state)
	{
		_onLighRemaining = DELAY_LIGHTS_ON_MS;
	}

	if (_onLighRemaining <= 0)
	{
		return;
	}


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