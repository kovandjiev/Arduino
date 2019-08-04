// 
// 
// 

#include "VentilateProcess.h"

#define GATE_CHANGE_STATE_MORE_THAN_MS  20 * 1000
#define CHECK_GATE_INTERVAL_MS          2 * 1000

unsigned long _changeStateInterval;
unsigned long _checkGateInterval;
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
	_currentState = false;

	_waitingForChange = false;
}

bool VentilateProcessClass::getState()
{
	return _currentState;
}

void VentilateProcessClass::process()
{
	if (_checkGateInterval > millis())
	{
		return;
	}
	_checkGateInterval = CHECK_GATE_INTERVAL_MS + millis();

	bool isOpened = false;
	for (int i = 0; i < _gateCount; i++)
	{
		if (KMPDinoWiFiESP.GetOptoInState(_gates[i]))
		{
			isOpened = true;
			break;
		}
	}

	if (isOpened == _currentState)
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
		_currentState = isOpened;

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
