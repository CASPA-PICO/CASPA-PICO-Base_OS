#ifndef BASE_OS_BASEINTERNET_H
#define BASE_OS_BASEINTERNET_H

#include "BaseEthernet.h"
#include "BaseWifi.h"

class BaseInternet {
public:
	enum Source {NoSource, EthernetSource, WifiSource};

	BaseInternet(BaseEthernet *baseEthernet, BaseWifi *baseWifi, BasePreferences *basePreferences);

	bool getTimeFromServer(Source source, int retry=0);
	bool getDeviceKeyFromServer(Source source, int retry=0);
	bool checkDeviceOnServer(Source source, int retry = 0);

private :
	BaseEthernet *baseEthernet;
	BaseWifi *baseWifi;
	BasePreferences *basePreferences;
};


#endif //BASE_OS_BASEINTERNET_H
