#include "BaseEthernet.h"

BaseEthernet::BaseEthernet(BasePreferences *basePreferences): basePreferences(basePreferences) {
	pinMode(ETHERNET_RESET_PIN, OUTPUT);
	digitalWrite(ETHERNET_RESET_PIN, HIGH);
	Ethernet.init(ETHERNET_CS_PIN);
	status = NoHardware;
	if(Ethernet.hardwareStatus() != EthernetNoHardware){
		setStatus(NoLink);
	}
	else{
		setStatus(NoHardware);
	}
}

bool BaseEthernet::initEthernet() {
	byte mac[] = ETHERNET_MAC;
#ifdef DEBUG_ETHERNET
		Serial.println("Connecting to Ethernet with DHCP...");
#endif
	if(Ethernet.linkStatus() == LinkOFF && Ethernet.hardwareStatus() != EthernetNoHardware){
#ifdef DEBUG_ETHERNET
		Serial.println("Ethernet no link !");
#endif
		setStatus(NoLink);
		return false;
	}

	setStatus(Connecting);
	if (Ethernet.begin(mac, 30000, 4000) == 0) {
#ifdef DEBUG_ETHERNET
		Serial.println("Failed to configure Ethernet using DHCP");
#endif
		setStatus(NoLink);
		return false;
	}

	setStatus(Connected);
#ifdef DEBUG_ETHERNET
	Serial.print("Connected ! IP address : ");
	Serial.println(Ethernet.localIP());
#endif
	return true;
}

String BaseEthernet::ethernetHttpGet(const String& path) {
	EthernetClient client;
	if (!client.connect(SERVER_URL, 80)) {
#ifdef DEBUG_ETHERNET
		Serial.println(String("Couldn't connect to ") + String(SERVER_URL) + String(" on port 80 !"));
#endif
		if(Ethernet.hardwareStatus() == EthernetNoHardware){
			setStatus(NoHardware);
		}
		else if(Ethernet.linkStatus() != LinkON){
			setStatus(NoLink);
		}

		return "";
	}

	String request = HttpParser::getRequestStr(path);

#ifdef DEBUG_ETHERNET
	Serial.println("HTTP GET :\r\n" + request);
#endif

	char requestArray[512];
	request.toCharArray(requestArray, 512);
	client.write(requestArray, strlen(requestArray));
	client.flush();

	int len;
	char buff[256];
	String resultStr;
	unsigned long startTime = millis();
	while (client.connected() || millis()-startTime > 30*1000) {
		len = client.available();
		if (len > 0) {
			if (len > 255) {
				len = 255;
			}

#ifdef DEBUG_ETHERNET
			Serial.println("Received " + String(len) + " bytes from server");
#endif

			client.read((uint8_t *) buff, len);
			buff[len] = '\0';
			resultStr += buff;
		}
	}
#ifdef DEBUG_ETHERNET
	Serial.println("HTTP GET done :\r\n" + resultStr);
#endif
	return resultStr;
}

bool BaseEthernet::keepAlive() {
	if(Ethernet.hardwareStatus() == EthernetNoHardware){
		setStatus(NoHardware);
		return false;
	}
	else if(Ethernet.linkStatus() == LinkOFF){
		setStatus(NoLink);
		return false;
	}

	int res = Ethernet.maintain();
	if(res == DHCP_CHECK_NONE || res == DHCP_CHECK_RENEW_OK || res == DHCP_CHECK_REBIND_OK){
#ifdef DEBUG_ETHERNET
			Serial.println("Ethernet keept alive !");
#endif
		setStatus(Connected);
		return true;
	}

#ifdef DEBUG_ETHERNET
	Serial.println("Failed to keep alive !");
#endif
	return false;
}

BaseEthernet::Status BaseEthernet::getStatus() {
	return status;
}

void BaseEthernet::setStatus(BaseEthernet::Status newStatus) {
#ifdef DEBUG_ETHERNET
	printStatus(newStatus);
#endif
	status = newStatus;
}

void BaseEthernet::printStatus(BaseEthernet::Status status) {
	Serial.print("Ethernet status : ");
	switch(status){
		case NoHardware:
			Serial.println("no hardware");
			break;
		case NoLink:
			Serial.println("no link");
			break;
		case ConnectingNoHardware:
			Serial.println("connecting no hardware");
			break;
		case Connecting:
			Serial.println("connecting");
			break;
		case Connected:
			Serial.println("connected");
			break;
	}
}

