#ifndef BASE_OS_BASEBLUETOOTH_H
#define BASE_OS_BASEBLUETOOTH_H

//Pin du microcontroleur sur lequel est branché le reed switch
#define PIN_SENSOR_DETECT 17
//Affichage des informations de débogage dans la console
#define DEBUG_BLUETOOTH_SYNC

#include "PLTP.h"
#include <SPIFFS.h>
#include <FS.h>
#include <esp_task_wdt.h>

class BaseBluetooth {
public:
	enum Status {Off, Connecting, Connected};
	typedef struct{char *filename; int totalSize; int totalReceived;} ReceivedFile;

	BaseBluetooth();
	void startSync();
	Status getStatus(){return status;}
	int getBytesReceived();
	int getTotalBytes();

private:
	PLTP pltp = PLTP(true);
	Status status = Off;
	ReceivedFile lastReceivedFile;
};


#endif //BASE_OS_BASEBLUETOOTH_H
