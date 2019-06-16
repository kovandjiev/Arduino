// 
// 
// 

#include "WindowOpener.h"

WindowState _windowNewState = Close
bool _windowStateIsChanging = false;
unsigned long _windowChangeStateInterval;
int _windowSensorPin;
callBackPublishData _publishData;

void WindowOpenerClass::init(int windowSensorPin, callBackPublishData publishData)
{
	_windowSensorPin = windowSensorPin;
	_publishData = publishData;
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

	// Start bypass state changing
	_windowNewState = state;

	_windowStateIsChanging = true;
	_windowChangeStateInterval = millis() + BYPASS_CHANGE_STATE_INTERVAL_MS;

	setBypassPin(_bypassNewState, true);
}

void WindowOpenerClass::processWinodwState()
{
	if (!_windowStateIsChanging)
	{
		return;
	}

	// End bypass state changing.
	if (_windowChangeStateInterval < millis())
	{
		_windowStateIsChanging = false;
		_windowState = _windowNewState;

		if (_publishData != NULL)
		{
			_publishData(WindowCurrentState, false);
		}
	}
}

WindowOpenerClass WindowOpener;