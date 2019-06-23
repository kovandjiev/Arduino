// 
// 
// 

#include "WindowOpener.h"

WindowState _windowNewState = CloseWindow;
bool _windowStateIsChanging = false;
bool _windowCloseAdditionalTime;
unsigned long _windowChangeStateInterval;
OptoIn _sensor;
Relay _openRelay;
Relay _closeRelay;
callBackPublishData _publishDataCallBack;

void WindowOpenerClass::init(OptoIn sensor, Relay openRelay, Relay closeRelay, callBackPublishData publishData)
{
	_sensor = sensor;
	_openRelay = openRelay;
	_closeRelay = closeRelay;
	_publishDataCallBack = publishData;
}

WindowState WindowOpenerClass::getState()
{
	return _windowState;
}

/**
* @brief Set window new state.
* @param state new state of the window
* @desc Open and Close the window.
*
* @return void
**/
void WindowOpenerClass::setState(WindowState state, bool forceState)
{
	if (!forceState)
	{
		if (state == _windowState || _windowStateIsChanging)
		{
			return;
		}
	}

	_windowNewState = state;

	_windowStateIsChanging = true;

	// Close window we will process together with sensorIn.
	if (_windowNewState == CloseWindow)
	{
		_windowChangeStateInterval = CLOSE_WINDOW_DURATION_MS + ADDITIONAL_OPERATION_TIME_MS;
		KMPDinoWiFiESP.SetRelayState(_closeRelay, true);
		_windowCloseAdditionalTime = false;
		return;
	}

	int openDifference = _windowNewState - _windowState;

	// The window needs to be open more.
	if (openDifference > 0)
	{
		_windowChangeStateInterval = OPEN_WINDOW_SINGLE_STEP_DURATION_MS * openDifference;
		KMPDinoWiFiESP.SetRelayState(_openRelay, true);
	}
	else
	{
		_windowChangeStateInterval = CLOSE_WINDOW_SINGLE_STEP_DURATION_MS * (openDifference * -1);
		KMPDinoWiFiESP.SetRelayState(_closeRelay, true);
	}

	_windowChangeStateInterval += millis();
}

void WindowOpenerClass::processWindowState()
{
	if (!_windowStateIsChanging)
	{
		return;
	}

	// End window state changing.
	if (_windowChangeStateInterval > millis())
	{
		// Waiting for sensor and add sometime
		if (_windowNewState == CloseWindow && !KMPDinoWiFiESP.GetOptoInState(_sensor) && !_windowCloseAdditionalTime)
		{
			_windowCloseAdditionalTime = true;

			_windowChangeStateInterval = millis() + ADDITIONAL_CLOSE_WINDOW_TIME_MS;
		}
		
		return;
	}

	_windowStateIsChanging = false;
	_windowState = _windowNewState;

	if (_publishDataCallBack != NULL)
	{
		_publishDataCallBack(WindowCurrentState, false);
	}
}

WindowOpenerClass WindowOpener;