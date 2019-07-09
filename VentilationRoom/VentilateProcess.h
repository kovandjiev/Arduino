// VentilateProcess.h

#ifndef _VENTILATEPROCESS_H
#define _VENTILATEPROCESS_H

#include "Arduino.h"
#include "VentilationHelper.h"
#include <KMPDinoWiFiESP.h>

typedef void(* callBackPublishVentilation) (DeviceData deviceData, int num, bool isPrintPublish);

class VentilateProcessClass : private KMPDinoWiFiESPClass
{
private:
	void publishState();
public:
	void init(OptoIn* gates, int gateCount, callBackPublishVentilation publishData);

	bool getState();

	void process();
};

extern VentilateProcessClass VentilateProcess;

#endif
