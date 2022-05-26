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
BaseBluetooth *baseBluetooth = nullptr;
BaseInternet::Source internetSource = BaseInternet::NoSource;


void setup() {
	Serial.begin(115200);
	Serial.println("Demarrage de la base !");

	//On génère une randomSeed pour le SSL
	randomSeed(analogRead(0));

	//On ouvre le stockage local de la base
	if(!SPIFFS.begin(true)){
		if(!SPIFFS.begin()){
			Serial.println("Echec de l'ouverture du stockage local !");
		}
	}

	basePreferences = new BasePreferences();

	//Création et démarrage des tâches
	xTaskCreatePinnedToCore(ethernetTask, "Ethernet task", 5*1024, nullptr, 1, &ethernetTask_handle, 1);
	while(baseEthernet == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(bluetoothTask, "Bluetooth task", 12*1024, nullptr, 2, &bluetoothTask_handle, 0);
	while(baseBluetooth == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(wifiTask, "Wifi task", 10*1024, nullptr, 3, &wifiTask_handle, 0);
	while(baseWifi == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(displayTask, "Display task", 2*1024, nullptr, 5, &displayTask_handle, 0);
	while(baseDisplay == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}
	xTaskCreatePinnedToCore(internetTask, "Internet task", 15*1024, nullptr, 4, &internetTask_handle, 0);
	while(baseInternet == nullptr){ vTaskDelay(pdMS_TO_TICKS(500));}

	//Configuration du WatchDog timer
	esp_task_wdt_init(2*60, true);
	esp_task_wdt_add(ethernetTask_handle);
	esp_task_wdt_add(internetTask_handle);
	esp_task_wdt_add(displayTask_handle);
	esp_task_wdt_add(wifiTask_handle);
	esp_task_wdt_add(bluetoothTask_handle);
}

void loop() {
	//La boucle principale ne fait rien -> chaque tâche s'occupe de sa partie à gérer
	vTaskDelay(pdMS_TO_TICKS(10*1000));
	esp_task_wdt_reset();
}
/**
 * Tâche Ethernet : s'occupe de la connexion du port Ethernet à la box
 * Gère l'obtention d'une adresse IP ainsi que son renouvellement avec le DHCP
 *
 * @param pvParameters : Non utilisé
 */
[[noreturn]] static void ethernetTask(void *pvParameters) {
	baseEthernet = new BaseEthernet(basePreferences);
	vTaskDelay(pdMS_TO_TICKS(2*1000));
	//Count permet de compter les secondes depuis le dernier keepAlive avec le DHCP
	int count = 0;
	while (true) {
		esp_task_wdt_reset();
		if (baseEthernet->initEthernet()) {
			//Tant que la base est connectée, on fait un keepAlive toutes les 120 secondes avec le DHCP
			while(baseEthernet->getStatus() == BaseEthernet::Connected){
				esp_task_wdt_reset();
				vTaskDelay(pdMS_TO_TICKS(10*1000));
				count += 10;
				if(count >= 120 || Ethernet.linkStatus() == LinkOFF) {
					count = 0;
					/* Si le keepAlive échoue, c'est que la connexion a été perdu, on sort de la boucle pour retenter de
					 * se connecter à nouveau
					 */
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

/**
 * Tâche d'affichage : s'occupe de l'écran OLED, le rafraichi toutes les secondes avec les informations nécessaires
 * @param pvParameters : Non utilisé
 */
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
/**
 * Tâche Internet : gère l'échange d'informations entre la base et le serveur web
 * S'occupe de récupérer l'heure depuis le serveur ainsi qu'authentifier la base ou encore
 * transférer les données réçu du Bluetooth dans le stockage local vers le serveur web
 *
 * @param pvParameters : Non utilisé
 */
[[noreturn]] static void internetTask(void *pvParameters){
	baseInternet = new BaseInternet(baseEthernet, baseWifi, baseBluetooth, basePreferences);
	while(true){
		esp_task_wdt_reset();
		baseDisplay->setState(BaseDisplay::Booting);
		internetSource = BaseInternet::NoSource;
		vTaskDelay(pdMS_TO_TICKS(1*1000));

		//On choisi le mode de communication avec le serveur (Ethernet ou Wifi)
		if(baseEthernet->getStatus() == BaseEthernet::Connected){
			internetSource = BaseInternet::EthernetSource;
		}
		else if(baseWifi->getWifiStatus() == BaseWifi::Connected){
			internetSource = BaseInternet::WifiSource;
		}
		else{
			//Si ni l'Ethernet ni le Wifi sont connecté : on ne peut pas communiquer avec le serveur
			internetSource = BaseInternet::NoSource;
		}

		if(internetSource != BaseInternet::NoSource){
			//Une fois que la base est connectée en Ethernet ou Wifi, on récupère l'heure
			if(!baseInternet->getTimeFromServer(internetSource, 3)) {
				baseDisplay->showError("Erreur #1", "Impossible d'obtenir l'heure");
				vTaskDelay(pdMS_TO_TICKS(30*1000));
				continue;
			}

			/* Une fois l'heure récupéré, on effectue l'authentification web
			 * Si la base n'a pas de token (deviceKey), alors elle affiche un code d'authentification et essaie
			 * de récupérer le token toute les 5 secondes depuis le serveur
			 * (l'utilisateur doit taper le code d'authentification sur le site pour que la base puisse obtenir le token)
			 * Si la base a déjà un token enregistré, on saute cette étape)
			*/
			while(internetSource!=BaseInternet::NoSource && basePreferences->getDeviceKey() == ""){
				baseDisplay->setState(BaseDisplay::WebAuthenticationCode);
				esp_task_wdt_reset();
				vTaskDelay(pdMS_TO_TICKS(5*1000));
				baseInternet->getDeviceKeyFromServer(internetSource, 3);
			}
			if(internetSource == BaseInternet::NoSource){
				continue;
			}

			/* On vérifie maintenant que le token existe bien sur le serveur et qu'il est valide
			 * Si ce n'est pas le cas, on supprime le token enregistré de la base et on recommence la procédure
			 * de connexion à internet
			 * Cela permettra d'obtenir un token (voir ci-dessus)
			 */
			baseDisplay->setState(BaseDisplay::WebAuthentication);
			if(!baseInternet->checkDeviceOnServer(internetSource, 3)){
				baseDisplay->showError("Erreur #2",  "Impossible d'authentifier l'appareil");
				vTaskDelay(pdMS_TO_TICKS(30*1000));
				basePreferences->removeDeviceKeys();
				continue;
			}

			/* Si la base est bien authentifiée, on passe en mode "Idle" :
			 * on regarde s'il faut transférer des fichiers depuis le stockage local
			 * Si oui, on les transfère et sinon on affiche l'heure et on attend qu'une notification indiquant qu'un
			 * transfert doit se faire arrive
			 */
			baseDisplay->setState(BaseDisplay::Idle);
			while(internetSource != BaseInternet::NoSource){
				//Ici on compte le nombre de fichiers dans le stockage local
				esp_task_wdt_reset();
				int fileCount = 0;
				File root = SPIFFS.open("/");
				File file = root.openNextFile();
				while(file){
					fileCount++;
					file.close();
					file = root.openNextFile();
				}

				/* S'il y a des fichiers à transférer depuis le stockage local, ou si la tâche à reçu une notification
				 * lui indicant qu'il faut transférer les fichiers, on lance le transfert
				 */
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
/**
 * Tâche Wifi : s'occupe de gérer la connexion du microcontroleur en Wifi
 * Afficher le portail, se connecter au réseau Wifi de l'utilisateur
 *
 * @param pvParameters : Non utilisé
 */
[[noreturn]] static void wifiTask(void *pvParameters){
	baseWifi = new BaseWifi(basePreferences);
	//On active le point d'accès (permet à un utilisateur de se connecter et de configurer son réseau Wifi)
	baseWifi->startAccessPoint();
	//lastSavedAttemp : enregistre le moment depuis la dernière tentative de connexion au réseau Wifi de l'uitlisateur
	unsigned long lastSavedAttemp = 0;
	while(true){
		esp_task_wdt_reset();

		//Si on a un réseau Wifi enregistré : on tente de s'y connecter
		if(baseWifi->getWifiStatus() == BaseWifi::AccessPointWaitingCredentials
				&& baseWifi->getRouterSSID() != "" && baseWifi->getRouterPassword() != ""){

			baseWifi->setWifiStatus(BaseWifi::Connecting);
			vTaskDelay(pdMS_TO_TICKS(1000));
			lastSavedAttemp = millis();
#ifdef DEBUG_WIFI
			Serial.println("Connecting to "+baseWifi->getRouterSSID()+" with password "+baseWifi->getRouterPassword());
#endif
			WiFi.begin(baseWifi->getRouterSSID().c_str(), baseWifi->getRouterPassword().c_str());
			while(WiFi.status() != WL_CONNECTED && millis()-lastSavedAttemp < 20*1000){
				esp_task_wdt_reset();
				//Pendant la connexion, on continue de faire tourner le DNS du microcontroleur (qui permet d'afficher le portail de connexion)
				switch(baseWifi->getDnsStatus()){
					case BaseWifi::Running:
						//On utilise la Wifi que lorsque le Bluetooth est désactivé
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

			//On on réussi à se connecter : on enregistre le réseau Wifi pour le réutiliser plus tard (après un redémarrage de la base par exemple)
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
		//Au bout d'un certain temps : timeout de la tentative de connexion, on revient en arrière sur les changements
		else if(baseWifi->getWifiStatus() == BaseWifi::AccessPointWaitingCredentials
		&& basePreferences->getRouterSSID() != "" && basePreferences->getRouterPassword() != "" && millis()-lastSavedAttemp > 80*1000 && WiFi.scanComplete() != -1){
			baseWifi->loadRouterSettingsFromPreferences();
		}
		//Si le microcontroleur est déconnecté du réseau Wifi : on réactive le point d'accès pour que l'utilisateur puisse saisir un nouveau réseau Wifi
		else if(baseWifi->getWifiStatus() == BaseWifi::Connected && !WiFi.isConnected() && baseBluetooth->getStatus() == BaseBluetooth::Off){
			WiFi.disconnect();
			baseWifi->setWifiStatus(BaseWifi::AccessPointWaitingCredentials);
			if(baseWifi->getDnsStatus() == BaseWifi::Stopped){
				baseWifi->startAccessPoint();
			}

		}
		//Si la connexion est stable depuis plus de deux minutes, on désactive le point d'accès (l'utilisateur n'a plus besoin de configurer la connexion)
		else if(baseWifi->getWifiStatus() == BaseWifi::Connected && WiFi.isConnected()
				&& millis()-lastSavedAttemp > 120*1000 && baseWifi->getDnsStatus() == BaseWifi::Running){
			baseWifi->stopAccessPoint();
		}

		//On fait tourner le DNS du microcontroleur pour afficher le portail de connexion au réseau
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

/**
 * Interruption lorsque la base détecte que le capteur a été branché sur le port USB C
 */
static void IRAM_ATTR sensor_detect_ISR(){
	/* Lorsque la base détecte que le capteur a été branché : on envoie une notification a la tâche bluetooth lui
	 * indicant de démarer le transfert des données
	 */
	BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(bluetoothTask_handle, &pxHigherPriorityTaskWoken);
	if(pxHigherPriorityTaskWoken == pdTRUE){
		portYIELD_FROM_ISR();
	}
}
/**
 * Tâche Bluetooth : gère la communication en Bluetooth de la base avec le capteur pour récupérer les données
 *
 * @param pvParameters : Non utilisé
 */
[[noreturn]] static void bluetoothTask(void *pvParameters){
	baseBluetooth = new BaseBluetooth();

	//On configure le pin qui détecte lors que le capteur est branché à la base
	pinMode(PIN_SENSOR_DETECT, INPUT_PULLDOWN);
	attachInterrupt(PIN_SENSOR_DETECT, sensor_detect_ISR, RISING);

	while(true){
		//Si la tâche reçoit une notification -> le capteur a été branché il faut lancer le transfert
		if(xTaskNotifyWait(UINT32_MAX, UINT32_MAX, nullptr, pdMS_TO_TICKS(60*1000)) == pdPASS){
			esp_task_wdt_reset();

			/* On arrête les tâches Wifi et Internet :
			 * Le microcontroleur ne peut pas fonctionner avec le Wifi et le Bleutooth activés en même temps
			 * On désactive donc le Wifi (qu'on réactivera si besoin par la suite)
			 */
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
			//On notifie la tâche internet qu'il faut transférer des données au serveur et on relance les tâches
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