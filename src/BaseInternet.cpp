#include "BaseInternet.h"

BaseInternet::BaseInternet(BaseEthernet *baseEthernet, BasePreferences *basePreferences) : baseEthernet(baseEthernet), basePreferences(basePreferences) {

}

bool BaseInternet::getTimeFromServer(BaseInternet::Source source, int retry) {
#ifdef DEBUG_ETHERNET
	Serial.println("Getting time from server...");
#endif
	vTaskDelay(pdMS_TO_TICKS(1000));
	String serverResponse;
	if(source == EthernetSource){
		serverResponse  = baseEthernet->ethernetHttpGet("/api/time");
	}
	if (serverResponse == "") {
		if(retry > 0){
			vTaskDelay(pdMS_TO_TICKS(1000));
			return getTimeFromServer(source, retry-1);
		}
		return false;
	}

	HttpParser timeResponse;
	timeResponse.parseResponse(serverResponse);
	if (timeResponse.getStatusCode() != 200) {
#ifdef DEBUG_ETHERNET
		Serial.println("Wrong status code " + String(timeResponse.getStatusCode()) + " !");
#endif
		if(retry > 0){
			vTaskDelay(pdMS_TO_TICKS(1000));
			return getTimeFromServer(source, retry-1);
		}
		return false;
	}
	String resultStr = timeResponse.getBody();
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
		vTaskDelay(pdMS_TO_TICKS(1000));
		return getTimeFromServer(source, retry-1);
	}
	return false;
}

bool BaseInternet::getDeviceKeyFromServer(BaseInternet::Source source, int retry) {
	String serverResponse;
	if(source == EthernetSource){
		serverResponse = baseEthernet->ethernetHttpGet(String("/api/devices/activate?activationKey=") + basePreferences->getDeviceActivationKey());
	}

	if (serverResponse == "") {
		if(retry > 0){
			vTaskDelay(pdMS_TO_TICKS(1000));
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
	else if(retry > 0){
		vTaskDelay(pdMS_TO_TICKS(1000));
		return getDeviceKeyFromServer(source, retry-1);
	}
	return false;
}

bool BaseInternet::checkDeviceOnServer(Source source, int retry) {
	String serverResponse;
	if(source == EthernetSource){
		serverResponse = baseEthernet->ethernetHttpGet(String("/api/devices/check?key=") + basePreferences->getDeviceKey());
	}

	if (serverResponse == "") {
		if(retry > 0){
			vTaskDelay(pdMS_TO_TICKS(1000));
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
	else if(retry > 0){
		vTaskDelay(pdMS_TO_TICKS(1000));
		return checkDeviceOnServer(source, retry-1);
	}
	return false;
}


