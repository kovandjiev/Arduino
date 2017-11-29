/*
HallLight.ino
Hall light project is for fade switch on or off lights in my hall.
*/
#define CHECK_INPUTS_LEN 10

// These pins check for change state low, high.
const byte CHECK_INPUTS[] = { A0, 15, 14, 16, 10, 2, 3, 4, 5, 6 };
const byte NO_POWER_INPUT = 7;
const byte NIGHT_INPUT = 8;
const byte DIMMER_OUTPUT = 9;           // the PWM pin the LED is attached to

int _brightness = 0;    // how bright the LED is
int _fadeAmount = 5;    // how many points to fade the LED by

					   // the setup routine runs once when you press reset:
void setup() 
{
	for (byte i = 0; i < CHECK_INPUTS_LEN; i++)
	{
		pinMode(CHECK_INPUTS[i], OUTPUT);
	}
	
	pinMode(NO_POWER_INPUT, OUTPUT);
	pinMode(NIGHT_INPUT, OUTPUT);

	pinMode(led, OUTPUT);
}

// the loop routine runs over and over again forever:
void loop() {
	// set the brightness of pin 9:
	analogWrite(led, brightness);

	// change the brightness for next time through the loop:
	brightness = brightness + fadeAmount;

	// reverse the direction of the fading at the ends of the fade:
	if (brightness <= 0 || brightness >= 255) {
		fadeAmount = -fadeAmount;
	}
	// wait for 30 milliseconds to see the dimming effect
	delay(30);
}