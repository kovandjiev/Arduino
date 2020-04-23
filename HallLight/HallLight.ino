/*
HallLight.ino
Hall light project is for fade switch on or off lights in my hall.
*/
#include <avr/wdt.h>

#define CHECK_INPUTS_LEN 10
//#define DEBUG

// These pins check for change state low, high.
const byte CHECK_INPUTS[CHECK_INPUTS_LEN] = {
	A0, // PIR1 Hall short part
	15, // PIR2 Hall long part
	14, // Door Kitchen and Living room
	16, // Door Sitting room
	10, // Door Bedroom 2
	2,  // Door Toilet
	3,  // Front door
	4,  // Door bathroom
	5,  // Door wet room
	6   // Door Bedroom 1
};

const byte NO_POWER_INPUT = 7; // Power exists 1 - has power, 0 - hasn't power Pull down. Is power down, dimming all lights on 50%.
const byte IS_NIGHT_INPUT = 8; // Is day or night 0 - day, 1 - night Pull down. Is night arrives, dimming on 30% light.
const byte DIMMER_OUTPUT = 9;  // Dimmer output PWM. Connect to dimmers.
const byte LIGHTS_IS_ON_OUTPUT = A1; // 1 - the light is On, 0 - the lights is Off. Events if lights is on pin up to 1.

const int MAX_BRIGHTNESS = 255;
const int MAX_BRIGHTNESS_NIGHT = 85;
const int MAX_BRIGHTNESS_NO_POWER = 127;
const unsigned long DELAY_LIGHTS_ON_MS = 60000; //5000; // 60000 One minute is on
const unsigned long FADE_DELAY_ON_MS = 8; // Waiting before switch to next level up light. Fast up.
const unsigned long FADE_DELAY_OFF_MS = 100; // Waiting before switch to next level down light. Slow down.
const unsigned long NOISE_CLEAR_MS = 100; // Clear noise if turn on/off inputs
const unsigned long NOISE_CLEAR_LONG_MS = 1000; // Clear noise if turn on/off inputs
const int FADE_POINTS = 1;    // 5 how many points to fade the LED by

bool _inputState[CHECK_INPUTS_LEN];
unsigned long _inputNoiseClear[CHECK_INPUTS_LEN];

int _currentBrightness = 0;    // how bright the LED is
int _lastBrightness = 0;    // how bright the LED is
unsigned long _lightEndTime = 0;
unsigned long _fadeDelay = 0;
bool _isNight = false;
unsigned long _isNightNoiseClear = 0;
bool _noPower = false;
unsigned long _noPowerNoiseClear = 0;
unsigned long _millis;

void setup()
{
#ifdef DEBUG
	Serial.begin(9600);
	Serial.println("Hall light project is started");
	Serial.println("Waiting for flash...");
#endif // DEBUG

	// Wait for 30 seconds for the board has opportunity to flash a new sketch. If comment this row, it can flash only in ISP.
	delay(30000);

	for (byte i = 0; i < CHECK_INPUTS_LEN; i++)
	{
		pinMode(CHECK_INPUTS[i], INPUT);

		// Initialize compare array.
		_inputState[i] = digitalRead(CHECK_INPUTS[i]);
	}

	pinMode(NO_POWER_INPUT, INPUT);
	pinMode(IS_NIGHT_INPUT, INPUT);

	pinMode(DIMMER_OUTPUT, OUTPUT);
	pinMode(LIGHTS_IS_ON_OUTPUT, OUTPUT);

	// Start MC Watchdog timer.
	cli(); // disable all interrupts
	wdt_reset(); // reset the WDT timer    
				 /*
				 WDTCSR configuration:
				 WDIE = 1: Interrupt Enable
				 WDE = 1 :Reset Enable
				 WDP3 WDP2 WDP1 WDP0
				 0 0 0 0 2K (2048) cycles 16 ms
				 0 0 0 1 4K (4096) cycles 32 ms
				 0 0 1 0 8K (8192) cycles 64 ms
				 0 0 1 1 16K (16384) cycles 0.125 s
				 0 1 0 0 32K (32768) cycles 0.25 s
				 0 1 0 1 64K (65536) cycles 0.5 s
				 0 1 1 0 128K (131072) cycles 1.0 s
				 0 1 1 1 256K (262144) cycles 2.0 s
				 1 0 0 0 512K (524288) cycles 4.0 s
				 1 0 0 1 1024K (1048576) cycles 8.0 s    */
				 /* Start timed sequence */
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	// Set Watchdog settings - Reset Enable, cycles 1.0 s.
	WDTCSR = (1 << WDE) | (0 << WDP3) | (1 << WDP2) | (1 << WDP1) | (0 << WDP0);
	sei();
#ifdef DEBUG
	Serial.println("Start loop");
#endif // DEBUG
}

void loop()
{
	wdt_reset();

	_millis = millis();

	bool inState = processInputs();
	if (inState)
	{
		_lightEndTime = _millis + DELAY_LIGHTS_ON_MS;
	}
	bool isLigthOn = _lightEndTime > _millis;

	int maxBrightness = getMaxBrightness();

	processBrightness(isLigthOn, maxBrightness);

	if (_currentBrightness != _lastBrightness)
	{
		analogWrite(DIMMER_OUTPUT, _currentBrightness);

		// Set Light is On output state
		if (_lastBrightness == 0 || _currentBrightness == 0)
		{
			digitalWrite(LIGHTS_IS_ON_OUTPUT, _currentBrightness > 0 ? HIGH : LOW);
		}

		_lastBrightness = _currentBrightness;
	}
}

void processBrightness(bool lighState, int maxBrightness)
{
	if (_fadeDelay > _millis)
	{
		return;
	}

	int newBrightness = _currentBrightness;

	if (lighState) // On
	{
		if (newBrightness < maxBrightness)
		{
			newBrightness += FADE_POINTS;

			_fadeDelay = _millis + FADE_DELAY_ON_MS;
			_lightEndTime = _millis + DELAY_LIGHTS_ON_MS;
		}
	}
	else // Off
	{
		if (newBrightness > 0)
		{
			newBrightness -= FADE_POINTS;

			_fadeDelay = _millis + FADE_DELAY_OFF_MS;
		}
	}

	if (newBrightness > maxBrightness)
	{
		newBrightness = maxBrightness;
	}

	if (newBrightness < 0)
	{
		newBrightness = 0;
	}

	_currentBrightness = newBrightness;
}

int getMaxBrightness()
{
	// Night arrived - active High
	bool isNight = digitalRead(IS_NIGHT_INPUT) == HIGH;
	if (isNotNoise(isNight != _isNight, NOISE_CLEAR_LONG_MS, &_isNightNoiseClear))
	{
		_isNight = isNight;
	}

	if (_isNight)
	{
		return MAX_BRIGHTNESS_NIGHT;
	}

	// No main power - active Low
	bool noPower = digitalRead(NO_POWER_INPUT) == LOW;
	if (isNotNoise(noPower != _noPower, NOISE_CLEAR_LONG_MS, &_noPowerNoiseClear))
	{
		_noPower = noPower;
	}

	if (_noPower)
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

		if (isNotNoise(inputState != _inputState[i], NOISE_CLEAR_MS, &_inputNoiseClear[i]))
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

bool isNotNoise(const bool status, const unsigned long delay, unsigned long* timeout)
{
	if (status)
	{
		if (0 == *timeout)
		{
			*timeout = _millis + delay;
		}
		else
		{
			if (_millis > *timeout)
			{
				*timeout = 0;
				return true;
			}
		}
	}
	else
	{
		*timeout = 0;
	}

	return false;
}