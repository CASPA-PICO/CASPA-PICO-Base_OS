#include "BaseInternet.h"
#include "BaseDisplay.h"
#include "BaseWifi.h"
#include <esp_task_wdt.h>

[[noreturn]] static void ethernetTask(void *pvParameters);
[[noreturn]] static void displayTask(void *pvParameters);
[[noreturn]] static void internetTask(void *pvParameters);
[[noreturn]] static void wifiTask(void *pvParameters);

TaskHandle_t displayTask_handle, ethernetTask_handle, internetTask_handle, wifiTask_handle;

BaseEthernet *baseEthernet = nullptr;
BaseWifi *baseWifi = nullptr;
BaseDisplay *baseDisplay = nullptr;
BasePreferences *basePreferences = nullptr;
BaseInternet *baseInternet = nullptr;
BaseInternet::Source internetSource = BaseInternet::NoSource;


void setup() {
	randomSeed(analogRead(0));
	Serial.begin(115200);
	Serial.println("Demarrage de la base !");


	basePreferences = new BasePreferences();


	xTaskCreatePinnedToCore(ethernetTask, "Ethernet task", 10240, nullptr, 1, &ethernetTask_handle, 1);
	while(baseEthernet == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(wifiTask, "Wifi task", 10240, nullptr, 2, &wifiTask_handle, 0);
	while(baseWifi == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(displayTask, "Display task", 10240, nullptr, 4, &displayTask_handle, 0);
	while(baseDisplay == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(internetTask, "Internet task", 10240, nullptr, 3, &internetTask_handle, 0);
	while(baseInternet == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}

	esp_task_wdt_init(20, true);
	esp_task_wdt_add(ethernetTask_handle);
	esp_task_wdt_add(internetTask_handle);
	esp_task_wdt_add(displayTask_handle);
	esp_task_wdt_add(wifiTask_handle);
}

void loop() {
	vTaskDelay(pdMS_TO_TICKS(10*1000));
	esp_task_wdt_reset();
}

[[noreturn]] static void ethernetTask(void *pvParameters) {
	baseEthernet = new BaseEthernet(basePreferences);
	int count = 0;
	while (true) {
		esp_task_wdt_reset();
		if (baseEthernet->initEthernet()) {
			while(baseEthernet->getStatus() == BaseEthernet::Connected){
				esp_task_wdt_reset();
				if(count >= 120 || Ethernet.linkStatus() != LinkOFF) {
					count = 0;
					if (!baseEthernet->keepAlive()) {
						break;
					}
				}
				vTaskDelay(pdMS_TO_TICKS(10*1000));
				count += 10;
			}
		}
		if(internetSource == BaseInternet::EthernetSource){
			internetSource = BaseInternet::NoSource;
		}
		vTaskDelay(pdMS_TO_TICKS(10*1000));
	}
}

[[noreturn]] static void displayTask(void *pvParameters){
	baseDisplay = new BaseDisplay(basePreferences);
	while(true){
		esp_task_wdt_reset();
		baseDisplay->setEthernetStatus(baseEthernet->getStatus());
		baseDisplay->setWifiStatus(baseWifi->getWifiStatus());
		baseDisplay->updateDisplay();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

[[noreturn]] static void internetTask(void *pvParameters){
	baseInternet = new BaseInternet(baseEthernet, basePreferences);
	while(true){
		esp_task_wdt_reset();
		baseDisplay->setState(BaseDisplay::Booting);
		internetSource = BaseInternet::NoSource;
		vTaskDelay(pdMS_TO_TICKS(1*1000));

		if(baseEthernet->getStatus() == BaseEthernet::Connected){
			internetSource = BaseInternet::EthernetSource;
		}
		else{
			internetSource = BaseInternet::NoSource;
		}

		if(internetSource != BaseInternet::NoSource){
			if(!baseInternet->getTimeFromServer(internetSource, 3)) {
				//TODO : couldn't get time from server
				vTaskDelay(pdMS_TO_TICKS(30*1000));
				continue;
			}

			//Web authentication
			//If device is not activated (got no device key) -> loop while it gets registered on the server
			while(internetSource!=BaseInternet::NoSource && basePreferences->getDeviceKey() == ""){
				baseDisplay->setState(BaseDisplay::WebAuthenticationCode);
				esp_task_wdt_reset();
				vTaskDelay(pdMS_TO_TICKS(5*1000));
				baseInternet->getDeviceKeyFromServer(internetSource, 3);
			}
			if(internetSource == BaseInternet::NoSource){
				continue;
			}

			baseDisplay->setState(BaseDisplay::WebAuthentication);
			if(!baseInternet->checkDeviceOnServer(internetSource, 3)){
				baseDisplay->setState(BaseDisplay::WebAuthenticationFailed);
				vTaskDelay(pdMS_TO_TICKS(30*1000));
				basePreferences->removeDeviceKeys();
				continue;
			}

			baseDisplay->setState(BaseDisplay::Idle);
			while(internetSource != BaseInternet::NoSource){
				esp_task_wdt_reset();
				if(xTaskNotifyWait(UINT32_MAX, UINT32_MAX, nullptr, pdMS_TO_TICKS(60*1000)) == pdTRUE && internetSource != BaseInternet::NoSource){

				}
			}
		}
	}
}

[[noreturn]] static void wifiTask(void *pvParameters){
	baseWifi = new BaseWifi(basePreferences);
	baseWifi->startAccessPoint();
	unsigned long lastSavedAttemp = 0;
	while(true){
		esp_task_wdt_reset();

		if(baseWifi->getWifiStatus() == BaseWifi::AccessPointWaitingCredentials
				&& baseWifi->getRouterSSID() != "" && baseWifi->getRouterPassword() != ""){

			baseWifi->setWifiStatus(BaseWifi::Connecting);
			vTaskDelay(pdMS_TO_TICKS(1000));
			lastSavedAttemp = millis();
			WiFi.begin(baseWifi->getRouterSSID().c_str(), baseWifi->getRouterPassword().c_str());
			while(WiFi.status() != WL_CONNECTED && millis()-lastSavedAttemp < 20*1000){
				esp_task_wdt_reset();
				switch(baseWifi->getDnsStatus()){
					case BaseWifi::Running:
						baseWifi->processDNS();
						vTaskDelay(pdMS_TO_TICKS(20));
						break;
					case BaseWifi::Stopped:
						vTaskDelay(pdMS_TO_TICKS(1000));
						break;
				}
			}

			if(WiFi.status() == WL_CONNECTED){
				baseWifi->setWifiStatus(BaseWifi::Connected);
				basePreferences->setRouterSSIDandPassword(baseWifi->getRouterSSID(), baseWifi->getRouterPassword());
			}
			else{
				baseWifi->failedConnection();
			}
		}
		else if(baseWifi->getWifiStatus() == BaseWifi::AccessPointWaitingCredentials
		&& basePreferences->getRouterSSID() != "" && basePreferences->getRouterPassword() != "" && millis()-lastSavedAttemp > 80*1000){
			baseWifi->loadRouterSettingsFromPreferences();
		}
		else if(baseWifi->getWifiStatus() == BaseWifi::Connected && !WiFi.isConnected()){
			WiFi.disconnect();
			baseWifi->setWifiStatus(BaseWifi::AccessPointWaitingCredentials);
		}

		switch(baseWifi->getDnsStatus()){
			case BaseWifi::Running:
				baseWifi->processDNS();
				vTaskDelay(pdMS_TO_TICKS(20));
				break;
			case BaseWifi::Stopped:
				vTaskDelay(pdMS_TO_TICKS(1000));
				break;
		}
	}
}