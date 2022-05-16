#include "BaseInternet.h"

BaseInternet::BaseInternet(BaseEthernet *baseEthernet, BaseWifi *baseWifi, BaseBluetooth *baseBluetooth, BasePreferences *basePreferences) : baseEthernet(baseEthernet), baseWifi(baseWifi), baseBluetooth(baseBluetooth), basePreferences(basePreferences) {

}

bool BaseInternet::getTimeFromServer(BaseInternet::Source source, int retry) {
#ifdef DEBUG_ETHERNET
	Serial.println("Getting time from server...");
#endif
	vTaskDelay(pdMS_TO_TICKS(1000));
	String resultStr;
	if(source == EthernetSource) {
		String serverResponse = baseEthernet->ethernetHttpGet("/api/time");
		HttpParser timeResponse;
		if (serverResponse == "") {
			if(retry > 0){
				vTaskDelay(pdMS_TO_TICKS(5000));
				return getTimeFromServer(source, retry-1);
			}
			return false;
		}
		timeResponse.parseResponse(serverResponse);
		if (timeResponse.getStatusCode() != 200) {
#ifdef DEBUG_ETHERNET
			Serial.println("Wrong status code " + String(timeResponse.getStatusCode()) + " !");
#endif
			if(retry > 0){
				vTaskDelay(pdMS_TO_TICKS(5000));
				return getTimeFromServer(source, retry-1);
			}
			return false;
		}
		resultStr = timeResponse.getBody();
	}
	else if(source == WifiSource){
		resultStr = baseWifi->wifiHttpGet("/api/time");
		if (resultStr == "") {
			if(retry > 0){
				vTaskDelay(pdMS_TO_TICKS(5000));
				return getTimeFromServer(source, retry-1);
			}
			return false;
		}
	}

	unsigned long long result = 0;
	for (int i = 0; i < resultStr.length() && resultStr[i] >= '0' && resultStr[i] <= '9'; ++i) {
		result = result * 10 + resultStr[i] - '0';
	}

	if(result > 0) {
		struct timeval tv{};
		tv.tv_sec = result / 1000;
		tv.tv_usec = 0;
		settimeofday(&tv, nullptr);
		setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/ 3", 1);
		tzset();
		return true;
	}
	else if(retry > 0){
		vTaskDelay(pdMS_TO_TICKS(5000));
		return getTimeFromServer(source, retry-1);
	}
	return false;
}

bool BaseInternet::getDeviceKeyFromServer(BaseInternet::Source source, int retry) {
	if(source == EthernetSource){
		String serverResponse = baseEthernet->ethernetHttpGet(String("/api/devices/activate?activationKey=") + basePreferences->getDeviceActivationKey());
		if (serverResponse == "") {
			if(retry > 0){
				vTaskDelay(pdMS_TO_TICKS(5000));
				return getDeviceKeyFromServer(source, retry-1);
			}
			return false;
		}
		HttpParser deviceKeyResponse;
		deviceKeyResponse.parseResponse(serverResponse);
		if(deviceKeyResponse.getStatusCode() == 200){
#ifdef DEBUG_ETHERNET
			Serial.println("Got device key : "+deviceKeyResponse.getBody());
#endif
			basePreferences->setDeviceKey(deviceKeyResponse.getBody());
			return true;
		}
	}
	else if(source == WifiSource){
		String deviceKeyResponse = baseWifi->wifiHttpGet(String("/api/devices/activate?activationKey=") + basePreferences->getDeviceActivationKey());
		if (deviceKeyResponse != "") {
#ifdef DEBUG_WIFI
			Serial.println("Got device key : "+deviceKeyResponse);
#endif
			basePreferences->setDeviceKey(deviceKeyResponse);
		}
	}


	if(retry > 0){
		vTaskDelay(pdMS_TO_TICKS(5000));
		return getDeviceKeyFromServer(source, retry-1);
	}
	return false;
}

bool BaseInternet::checkDeviceOnServer(Source source, int retry) {
	if(source == EthernetSource){
		String serverResponse;
		serverResponse = baseEthernet->ethernetHttpGet(String("/api/devices/check?key=") + basePreferences->getDeviceKey());
		if (serverResponse == "") {
			if(retry > 0){
				vTaskDelay(pdMS_TO_TICKS(5000));
				return checkDeviceOnServer(source, retry-1);
			}
			return false;
		}
		HttpParser deviceKeyResponse;
		deviceKeyResponse.parseResponse(serverResponse);
		if(deviceKeyResponse.getStatusCode() == 200){
#ifdef DEBUG_ETHERNET
			Serial.println("Device check OK");
#endif
			return true;
		}
	}
	else if(source == WifiSource){
		String deviceKeyResponse = baseWifi->wifiHttpGet(String("/api/devices/check?key=") + basePreferences->getDeviceKey());
		if (deviceKeyResponse != "") {
#ifdef DEBUG_WIFI
			Serial.println("Device check OK");
#endif
			return true;
		}
	}

	if(retry > 0){
		vTaskDelay(pdMS_TO_TICKS(5000));
		return checkDeviceOnServer(source, retry-1);
	}
	return false;
}

void BaseInternet::syncLocalFiles(BaseInternet::Source source) {
	if(source == EthernetSource){
		File root = SPIFFS.open("/", FILE_READ);
		File file = root.openNextFile();
		String filePath;
		while(file){
			if(baseEthernet->sendFileHTTPPost("/api/devices/uploadData", &file)){
#ifdef DEBUG_WIFI
				Serial.println("Successfully sent file over ethernet : "+String(file.path()));
#endif
				filePath = file.path();
				file.close();
				SPIFFS.remove(filePath.c_str());
			}
			else{
				file.close();
			}
			file = root.openNextFile();
		}
	}
	else if(source == WifiSource){
		File root = SPIFFS.open("/", FILE_READ);
		File file = root.openNextFile();
		String filePath;

		while(file){
			if(baseWifi->sendFileHTTPPost("/api/devices/uploadData", &file)){
#ifdef DEBUG_WIFI
				Serial.println("Successfully sent file over wifi : "+String(file.path()));
#endif
				filePath = file.path();
				file.close();
				SPIFFS.remove(filePath.c_str());
			}
			else{
				file.close();
			}
			file = root.openNextFile();
		}
	}
}