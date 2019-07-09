// 
// 
// 

#include "VentilateProcess.h"

#define GATE_CHANGE_STATE_MORE_THAN_MS  20 * 1000
#define CHECK_INTERVAL_MS                2 * 1000

unsigned long _changeStateInterval;
unsigned long _checkInterval;
OptoIn* _gates;
int _gateCount;
bool _currentState;
bool _waitingForChange;
callBackPublishVentilation _publishVentilationCallBack;

void VentilateProcessClass::init(OptoIn* gates, int gateCount, callBackPublishVentilation publishData)
{
	_gates = gates;
	_gateCount = gateCount;
	_publishVentilationCallBack = publishData;

	_waitingForChange = false;
}

bool VentilateProcessClass::getState()
{
	return _currentState;
}

void VentilateProcessClass::process()
{
	if (_checkInterval > millis())
	{
		return;
	}
	_checkInterval = CHECK_INTERVAL_MS + millis();

	bool stateNow = false;
	for (int i = 0; i < _gateCount; i++)
	{
		if (KMPDinoWiFiESP.GetOptoInState(_gates[i]))
		{
			stateNow = true;
			break;
		}
	}

	if (stateNow == _currentState)
	{
		_waitingForChange = false;
		return;
	}

	if (!_waitingForChange)
	{
		_waitingForChange = true;
		_changeStateInterval = GATE_CHANGE_STATE_MORE_THAN_MS + millis();
	}

	if (_changeStateInterval < millis())
	{
		_currentState = stateNow;

		publishState();
	}
}

void VentilateProcessClass::publishState()
{
	if (_publishVentilationCallBack != NULL)
	{
		_publishVentilationCallBack(Ventilate, 0, true);
	}
}

VentilateProcessClass VentilateProcess;
