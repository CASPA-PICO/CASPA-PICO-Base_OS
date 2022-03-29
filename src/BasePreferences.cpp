//
// Created by Nicolas on 11/03/2022.
//

#include "BasePreferences.h"

BasePreferences::BasePreferences() {
	preferences.begin("CASPA-PICO", false);
}

String BasePreferences::getDeviceActivationKey() {
	if(!preferences.isKey("devActKey")){
		char chars[7];
		sprintf(chars, "%06d", (int)random(0, 999999));
		preferences.putString("devActKey", chars);
	}

	return preferences.getString("devActKey", "");
}

String BasePreferences::getDeviceKey() {
	if(preferences.isKey("devKey")){
		return preferences.getString("devKey", "");
	}
	return "";
}

void BasePreferences::setDeviceKey(String deviceKey) {
	preferences.putString("devKey", deviceKey);
}

void BasePreferences::removeDeviceKeys() {
	preferences.remove("devActKey");
	preferences.remove("devKey");
}

String BasePreferences::getRouterSSID() {
	return preferences.getString("routerSSID", "");
}

String BasePreferences::getRouterPassword() {
	return preferences.getString("routerPass", "");
}

void BasePreferences::setRouterSSIDandPassword(String routerSSID, String routerPassword) {
	preferences.putString("routerSSID", routerSSID);
	preferences.putString("routerPass", routerPassword);
}
