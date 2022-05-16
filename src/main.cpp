#include "BaseInternet.h"
#include "BaseDisplay.h"
#include "BaseWifi.h"
#include "BaseBluetooth.h"
#include <esp_task_wdt.h>
#include <esp_wifi.h>

[[noreturn]] static void ethernetTask(void *pvParameters);
[[noreturn]] static void displayTask(void *pvParameters);
[[noreturn]] static void internetTask(void *pvParameters);
[[noreturn]] static void wifiTask(void *pvParameters);
[[noreturn]] static void bluetoothTask(void *pvParameters);

TaskHandle_t displayTask_handle, ethernetTask_handle, internetTask_handle, wifiTask_handle, bluetoothTask_handle;

BaseEthernet *baseEthernet = nullptr;
BaseWifi *baseWifi = nullptr;
BaseDisplay *baseDisplay = nullptr;
BasePreferences *basePreferences = nullptr;
BaseInternet *baseInternet = nullptr;
BaseInternet::Source internetSource = BaseInternet::NoSource;
BaseBluetooth *baseBluetooth = nullptr;


void setup() {
	randomSeed(analogRead(0));
	Serial.begin(115200);
	Serial.println("Demarrage de la base !");

	if(!SPIFFS.begin(true)){
		if(!SPIFFS.begin()){
			Serial.println("Echec de l'ouverture du stockage local !");
		}
	}

	basePreferences = new BasePreferences();

	xTaskCreatePinnedToCore(ethernetTask, "Ethernet task", 5*1024, nullptr, 1, &ethernetTask_handle, 1);
	while(baseEthernet == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(bluetoothTask, "Bluetooth task", 20*1024, nullptr, 2, &bluetoothTask_handle, 0);
	while(baseBluetooth == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(wifiTask, "Wifi task", 10*1024, nullptr, 3, &wifiTask_handle, 0);
	while(baseWifi == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(displayTask, "Display task", 2*1024, nullptr, 5, &displayTask_handle, 0);
	while(baseDisplay == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(internetTask, "Internet task", 6*1024, nullptr, 4, &internetTask_handle, 0);
	while(baseInternet == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}

	esp_task_wdt_init(2*60, true);
	esp_task_wdt_add(ethernetTask_handle);
	esp_task_wdt_add(internetTask_handle);
	esp_task_wdt_add(displayTask_handle);
	esp_task_wdt_add(wifiTask_handle);
	esp_task_wdt_add(bluetoothTask_handle);
}

void loop() {
	vTaskDelay(pdMS_TO_TICKS(10*1000));
	esp_task_wdt_reset();
}

[[noreturn]] static void ethernetTask(void *pvParameters) {
	baseEthernet = new BaseEthernet(basePreferences);
	vTaskDelay(pdMS_TO_TICKS(2*1000));
	int count = 0;
	while (true) {
		esp_task_wdt_reset();
		if (baseEthernet->initEthernet()) {
			while(baseEthernet->getStatus() == BaseEthernet::Connected){
				esp_task_wdt_reset();
				vTaskDelay(pdMS_TO_TICKS(10*1000));
				count += 10;
				if(count >= 120 || Ethernet.linkStatus() == LinkOFF) {
					count = 0;
					if (!baseEthernet->keepAlive()) {
						break;
					}
				}
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
		baseDisplay->setBluetoothStatus(baseBluetooth->getStatus());
		baseDisplay->setBluetoothProgress(baseBluetooth->getBytesReceived(), baseBluetooth->getTotalBytes());
		if(internetSource == BaseInternet::EthernetSource){
			baseDisplay->setInternetProgress(baseEthernet->getSizePostSent(), baseEthernet->getSizePostTotal());
		}
		else if(internetSource == BaseInternet::WifiSource){
			baseDisplay->setInternetProgress(baseWifi->getSizePostSent(), baseWifi->getSizePostTotal());
		}
		baseDisplay->updateDisplay();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

[[noreturn]] static void internetTask(void *pvParameters){
	baseInternet = new BaseInternet(baseEthernet, baseWifi, baseBluetooth, basePreferences);
	while(true){
		esp_task_wdt_reset();
		baseDisplay->setState(BaseDisplay::Booting);
		internetSource = BaseInternet::NoSource;
		vTaskDelay(pdMS_TO_TICKS(1*1000));

		if(baseEthernet->getStatus() == BaseEthernet::Connected){
			internetSource = BaseInternet::EthernetSource;
		}
		else if(baseWifi->getWifiStatus() == BaseWifi::Connected){
			internetSource = BaseInternet::WifiSource;
		}
		else{
			internetSource = BaseInternet::NoSource;
		}

		if(internetSource != BaseInternet::NoSource){
			if(!baseInternet->getTimeFromServer(internetSource, 3)) {
				baseDisplay->showError("Erreur #1", "Impossible d'obtenir l'heure");
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
				baseDisplay->showError("Erreur #2",  "Impossible d'authentifier l'appareil");
				vTaskDelay(pdMS_TO_TICKS(30*1000));
				basePreferences->removeDeviceKeys();
				continue;
			}

			baseDisplay->setState(BaseDisplay::Idle);
			while(internetSource != BaseInternet::NoSource){
				esp_task_wdt_reset();
				int fileCount = 0;
				File root = SPIFFS.open("/");
				File file = root.openNextFile();
				while(file){
					fileCount++;
					file.close();
					file = root.openNextFile();
				}
				if(fileCount > 0 || xTaskNotifyWait(UINT32_MAX, UINT32_MAX, nullptr, pdMS_TO_TICKS(60*1000)) == pdPASS && internetSource != BaseInternet::NoSource){
					baseDisplay->setState(BaseDisplay::DataTransfer);
					baseInternet->syncLocalFiles(internetSource);
					baseDisplay->setState(BaseDisplay::Idle);
					esp_task_wdt_reset();
					vTaskDelay(60*1000);
				}

				if(baseEthernet->getStatus() == BaseEthernet::Connected){
					internetSource = BaseInternet::EthernetSource;
				}
				else if(baseWifi->getWifiStatus() == BaseWifi::Connected){
					internetSource = BaseInternet::WifiSource;
				}
				else{
					internetSource = BaseInternet::NoSource;
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
						if(baseBluetooth->getStatus() == BaseBluetooth::Off){
							baseWifi->processDNS();
						}
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
#ifdef DEBUG_WIFI
				Serial.println("Connected to wifi network !");
#endif
			}
			else{
				baseWifi->failedConnection();
			}
		}
		else if(baseWifi->getWifiStatus() == BaseWifi::AccessPointWaitingCredentials
		&& basePreferences->getRouterSSID() != "" && basePreferences->getRouterPassword() != "" && millis()-lastSavedAttemp > 80*1000 && WiFi.scanComplete() != -1){
			baseWifi->loadRouterSettingsFromPreferences();
		}
		else if(baseWifi->getWifiStatus() == BaseWifi::Connected && !WiFi.isConnected() && baseBluetooth->getStatus() == BaseBluetooth::Off){
			WiFi.disconnect();
			baseWifi->setWifiStatus(BaseWifi::AccessPointWaitingCredentials);
			if(baseWifi->getDnsStatus() == BaseWifi::Stopped){
				baseWifi->startAccessPoint();
			}
		}
		else if(baseWifi->getWifiStatus() == BaseWifi::Connected && WiFi.isConnected()
				&& millis()-lastSavedAttemp > 120*1000 && baseWifi->getDnsStatus() == BaseWifi::Running){
			baseWifi->stopAccessPoint();
		}

		switch(baseWifi->getDnsStatus()){
			case BaseWifi::Running:
				if(baseBluetooth->getStatus() == BaseBluetooth::Off){
					baseWifi->processDNS();
				}
				vTaskDelay(pdMS_TO_TICKS(20));
				break;
			case BaseWifi::Stopped:
				vTaskDelay(pdMS_TO_TICKS(1000));
				break;
		}
	}
}

static void IRAM_ATTR sensor_detect_ISR(){
	BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(bluetoothTask_handle, &pxHigherPriorityTaskWoken);
	if(pxHigherPriorityTaskWoken == pdTRUE){
		portYIELD_FROM_ISR();
	}
}

[[noreturn]] static void bluetoothTask(void *pvParameters){
	baseBluetooth = new BaseBluetooth();
	pinMode(PIN_SENSOR_DETECT, INPUT_PULLDOWN);
	attachInterrupt(PIN_SENSOR_DETECT, sensor_detect_ISR, RISING);
	while(true){
		if(xTaskNotifyWait(UINT32_MAX, UINT32_MAX, nullptr, pdMS_TO_TICKS(60*1000)) == pdPASS){
			esp_task_wdt_reset();
			vTaskSuspend(wifiTask_handle);
			vTaskSuspend(internetTask_handle);
			esp_task_wdt_delete(wifiTask_handle);
			esp_task_wdt_delete(internetTask_handle);
			wifi_mode_t previousWifiMode = WiFi.getMode();
			WiFi.mode(WIFI_OFF);
			baseBluetooth->startSync();
			WiFi.mode(previousWifiMode);
			baseWifi->setWifiStatus(BaseWifi::AccessPointWaitingCredentials);
			if(internetSource == BaseInternet::WifiSource){
				if(baseEthernet->getStatus() == BaseEthernet::Connected){
					internetSource = BaseInternet::EthernetSource;
				}
				else{
					internetSource = BaseInternet::NoSource;
				}
			}
			xTaskNotifyGive(internetTask_handle);
			vTaskResume(wifiTask_handle);
			vTaskResume(internetTask_handle);
			esp_task_wdt_add(wifiTask_handle);
			esp_task_wdt_add(internetTask_handle);
			xTaskNotifyWait(UINT32_MAX, UINT32_MAX, nullptr, pdMS_TO_TICKS(100));
		}
		esp_task_wdt_reset();
	}
}