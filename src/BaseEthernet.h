#ifndef BASEETHERNET_H
#define BASEETHERNET_H

#define ETHERNET_MAC {0x00, 0x08, 0xDC, 0x68, 0x4E, 0x2C}
#define ETHERNET_CS_PIN 25
#define ETHERNET_RESET_PIN 26
#define ETHERNET_INT_PIN 0

#define SYNC_SERVER_URL "caspa.icare.univ-lille.fr"
#define SYNC_SERVER_PORT 443
#define SYNC_SERVER_USE_HTTPS

#define DEBUG_ETHERNET

#include <SPI.h>
#include <Ethernet.h>
#include <Arduino.h>
#include <FS.h>
#include <esp_task_wdt.h>
#include "HttpParser.h"
#include "BasePreferences.h"
#include <sys/time.h>

#include <SSLClient.h>
#include "ta.h"

class BaseEthernet {
public:
	enum Status {NoHardware, NoLink, Connecting, ConnectingNoHardware, Connected};

	BaseEthernet(BasePreferences *basePreferences);
	bool initEthernet();
	String ethernetHttpGet(const String& path);
	bool sendFileHTTPPost(const String& path, File *file);

	bool keepAlive();
	Status getStatus();
	void setStatus(Status newStatus);
	static void printStatus(Status status);

	int getSizePostTotal() const;

	int getSizePostSent() const;

private:
	Status status;
	BasePreferences *basePreferences;

	int sizePOSTTotal = -1;
	int sizePOSTSent = -1;
};

#endif
