#ifndef BASE_OS_BASEPREFERENCES_H
#define BASE_OS_BASEPREFERENCES_H

#include <Preferences.h>

class BasePreferences {
public:
	BasePreferences();

	String getDeviceActivationKey();
	String getDeviceKey();
	void setDeviceKey(String deviceKey);
	void removeDeviceKeys();

	String getRouterSSID();
	String getRouterPassword();
	void setRouterSSIDandPassword(String routerSSID, String routerPassword);

private:
	Preferences preferences;
};


#endif //BASE_OS_BASEPREFERENCES_H
