// 
// 
// 

#include "WindowOpener.h"

WindowStateType _windowState;
WindowStateType _windowNewState;
bool _windowStateIsChanging;
//bool _windowCloseAdditionalTime;
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

	_windowState = FullOpen;
	_windowStateIsChanging = false;
}

WindowStateType WindowOpenerClass::getState()
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
void WindowOpenerClass::setState(WindowStateType state, bool forceState)
{
	if (!forceState)
	{
		if (state == _windowState || _windowStateIsChanging)
		{
			publishState();
			return;
		}
	}

	_windowNewState = state;

	_windowStateIsChanging = true;

	int openDifference = _windowNewState - _windowState;

	//DEBUG_FC_PRINT("openDifference: ");
	//DEBUG_FC_PRINTLN(openDifference);
	// The window needs to be open more.
	if (openDifference > 0)
	{
		if (_windowNewState == FullOpen)
		{
			_windowChangeStateInterval = OPEN_WINDOW_DURATION_MS + OPEN_WINDOW_SINGLE_STEP_DURATION_MS;
		}
		else
		{
			_windowChangeStateInterval = OPEN_WINDOW_SINGLE_STEP_DURATION_MS * openDifference;
		}
		KMPDinoWiFiESP.SetRelayState(_openRelay, true);
	}
	else
	{
		openDifference = abs(openDifference);
		openDifference = openDifference == 0 ? 1 : openDifference;
		if (_windowNewState == CloseWindow)
		{
			_windowChangeStateInterval = CLOSE_WINDOW_DURATION_MS + CLOSE_WINDOW_SINGLE_STEP_DURATION_MS;

			// Close window we will process together with sensorIn.
			//_windowCloseAdditionalTime = false;
		}
		else
		{
			_windowChangeStateInterval = CLOSE_WINDOW_SINGLE_STEP_DURATION_MS * openDifference;
		}
		KMPDinoWiFiESP.SetRelayState(_closeRelay, true);
	}

	_windowChangeStateInterval += millis();
}

void WindowOpenerClass::process()
{
	if (!_windowStateIsChanging)
	{
		return;
	}

	// Window state is changing.
	if (_windowChangeStateInterval > millis())
	{
		// TODO: We will skip this logics
		//// Waiting for sensor and add sometime. 0 - window is closed. 1 - is opened.
		//// If window is closed (sensor 0) add additional time.
		//if (_windowNewState == CloseWindow && !KMPDinoWiFiESP.GetOptoInState(_sensor) && !_windowCloseAdditionalTime)
		//{
		//	_windowCloseAdditionalTime = true;

		//	_windowChangeStateInterval = millis() + CLOSE_WINDOW_SINGLE_STEP_DURATION_MS;
		//}

		return;
	}

	//_windowCloseAdditionalTime = false;

	_windowStateIsChanging = false;
	_windowState = _windowNewState;

	KMPDinoWiFiESP.SetRelayState(_openRelay, false);
	KMPDinoWiFiESP.SetRelayState(_closeRelay, false);

	publishState();
}

void WindowOpenerClass::publishState()
{
	if (_publishDataCallBack != NULL)
	{
		_publishDataCallBack(WindowState, 0, true);
	}
}

WindowOpenerClass WindowOpener;
