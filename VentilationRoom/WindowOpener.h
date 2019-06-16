// WindowOpener.h

#ifndef _WINDOWOPENER_H
#define _WINDOWOPENER_H

#include "Arduino.h"
#include "VentilationHelper.h"
#include <KMPDinoWiFiESP.h>

#define CLOSE_WINDOW_STATE 0
#define OPEN_WINDOW_STATE 3
#define OPEN_WINDOW_DURATION_MS 23  * 1000
#define OPEN_WINDOW_SINGLE_STEP_DURATION_MS (OPEN_WINDOW_DURATION_MS / OPEN_WINDOW_STATE + 1)
#define CLOSE_WINDOW_DURATION_MS 25 * 1000
#define CLOSE_WINDOW_SINGLE_STEP_DURATION_MS (CLOSE_WINDOW_DURATION_MS / OPEN_WINDOW_STATE + 1)
#define ADDITIONAL_OPERATION_TIME_MS 5 * 1000

enum WindowState
{
	Close = 0,
	QuarterOpen = 1,
	FalfOpen = 2,
	FullOpen = 3
};

typedef void(* callBackPublishData) (DeviceData deviceData, bool sendCurrent);

class WindowOpenerClass : private KMPDinoWiFiESPClass
{
private:
	WindowState _windowState = Close;
public:
	void init(int windowSensorPin, callBackPublishData publishData);

	WindowState getState();

	void setState(WindowState state, bool forceState = false);

	void processWinodwState();
};

extern WindowOpenerClass WindowOpener;

#endif

