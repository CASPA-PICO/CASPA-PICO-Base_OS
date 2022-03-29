#ifndef BASE_OS_BASEWIFI_H
#define BASE_OS_BASEWIFI_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <esp_task_wdt.h>

#include "BasePreferences.h"
#include "favicon.h"

#define PORT_DNS 53
#define PORT_SERVER 80
#define WIFI_SSID "Caspa-PICO"
#define WIFI_PASSWORD "Caspa123"

#define DEBUG_WIFI

class BaseWifi : public AsyncWebHandler{
public:
	enum DNS_Status {Running, Stopped};
	enum Wifi_Status {AccessPointWaitingCredentials, Connecting, Connected};

	BaseWifi(BasePreferences *basePreferences);
	void startAccessPoint();

	DNS_Status getDnsStatus() const;
	void startDNSServer();
	void stopDNSServer();
	void processDNS();

	Wifi_Status getWifiStatus() const;

	bool canHandle(AsyncWebServerRequest *request) override;

	void handleRequest(AsyncWebServerRequest *request) override;
	String generatePortal() const;
	String generateWaitingPage() const;
	String generateConnectedPage() const;

	void setWifiStatus(Wifi_Status newWifistatus){wifiStatus = newWifistatus;};
	void failedConnection();
	String getRouterSSID(){return routerSSID;}
	String getRouterPassword(){return routerPassword;}

	void loadRouterSettingsFromPreferences();

private:
	IPAddress apIP = IPAddress(192,168,0,1);
	IPAddress subnet = IPAddress(255, 255, 255, 0);

	DNS_Status dnsStatus = Stopped;
	Wifi_Status wifiStatus = AccessPointWaitingCredentials;

	const char *ssid = WIFI_SSID;
	const char *password = WIFI_PASSWORD;

	AsyncWebServer webServer;
	DNSServer dnsServer;

	String routerSSID;
	String routerPassword;

	BasePreferences *preferences;
	bool requestNetworkRefresh = false;
};

#endif //BASE_OS_BASEWIFI_H
