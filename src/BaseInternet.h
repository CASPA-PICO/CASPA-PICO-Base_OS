#ifndef BASE_OS_BASEINTERNET_H
#define BASE_OS_BASEINTERNET_H

#include "BaseEthernet.h"
#include "BaseWifi.h"
#include "BaseBluetooth.h"

class BaseInternet {
public:
	enum Source {NoSource, EthernetSource, WifiSource};

	BaseInternet(BaseEthernet *baseEthernet, BaseWifi *baseWifi, BaseBluetooth *baseBluetooth, BasePreferences *basePreferences);

	bool getTimeFromServer(Source source, int retry=0);
	bool getDeviceKeyFromServer(Source source, int retry=0);
	bool checkDeviceOnServer(Source source, int retry = 0);
	void syncLocalFiles(Source source);

private :
	BaseEthernet *baseEthernet;
	BaseWifi *baseWifi;
	BasePreferences *basePreferences;
	BaseBluetooth *baseBluetooth;

};


#endif //BASE_OS_BASEINTERNET_H
