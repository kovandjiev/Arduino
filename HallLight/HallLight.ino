/*
HallLight.ino
Hall light project is for fade switch on or off lights in my hall.
*/
#include <avr/wdt.h>

#define CHECK_INPUTS_LEN 10
#define DEBUG

// These pins check for change state low, high.
const byte CHECK_INPUTS[CHECK_INPUTS_LEN] = { A0, 15, 14, 16, 10, 2, 3, 4, 5, 6 };
const byte NO_POWER_INPUT = 7; // Pull down. 1 - has power, 0 - hasn't power
const byte IS_NIGHT_INPUT = 8; // Pull down. 0 - day, 1 - night
const byte DIMMER_OUTPUT = 9;
const byte LIGHTS_IS_ON_OUTPUT = A1; // 1 - the light is On, 0 - the lights is Off

const int MAX_BRIGHTNESS = 255;
const int MAX_BRIGHTNESS_NIGHT = 85;
const int MAX_BRIGHTNESS_NO_POWER = 127;
const unsigned long DELAY_LIGHTS_ON_MS = 60000; //5000; // 60000 One minute is on
const unsigned long FADE_DELAY_ON_MS = 8; // 30
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
}

void loop() 
{
	wdt_reset();

	bool inState = processInputs();

	bool lighState = processState(inState);

	int maxBrightness = getMaxBrightness();

	int newBrightness = processBrightness(lighState, _brightness, maxBrightness);

	setBrightness(newBrightness);

	digitalWrite(LIGHTS_IS_ON_OUTPUT, _brightness > 0 ? HIGH : LOW);
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
	if (digitalRead(IS_NIGHT_INPUT) == HIGH)
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