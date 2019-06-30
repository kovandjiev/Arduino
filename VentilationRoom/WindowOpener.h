// WindowOpener.h

#ifndef _WINDOWOPENER_H
#define _WINDOWOPENER_H

#include "Arduino.h"
#include "VentilationHelper.h"
#include <KMPDinoWiFiESP.h>

enum WindowStateType
{
	CloseWindow = 0,
	OneQuarterOpen = 1,
	FalfOpen = 2,
	ThreeQuartersOpen = 3,
	FullOpen = 4
};

#define CLOSE_WINDOW_STATE 0
#define MAX_WINDOW_STATE FullOpen
#define OPEN_WINDOW_DURATION_MS 23  * 1000
#define OPEN_WINDOW_SINGLE_STEP_DURATION_MS (OPEN_WINDOW_DURATION_MS / MAX_WINDOW_STATE)
#define CLOSE_WINDOW_DURATION_MS 25 * 1000
#define CLOSE_WINDOW_SINGLE_STEP_DURATION_MS (CLOSE_WINDOW_DURATION_MS / MAX_WINDOW_STATE)
#define ADDITIONAL_OPERATION_TIME_MS (CLOSE_WINDOW_DURATION_MS / 2)
#define ADDITIONAL_CLOSE_WINDOW_TIME_MS CLOSE_WINDOW_SINGLE_STEP_DURATION_MS

typedef void(* callBackPublishData) (DeviceData deviceData, int num, bool isPrintPublish);

class WindowOpenerClass : private KMPDinoWiFiESPClass
{
private:
	void publishState();
public:
	void init(OptoIn sensor, Relay openRelay, Relay closeRelay, callBackPublishData publishData);

	WindowStateType getState();

	void setState(WindowStateType state, bool forceState = false);

	void processWindowState();
};

extern WindowOpenerClass WindowOpener;

#endif
