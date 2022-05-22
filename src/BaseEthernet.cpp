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
	if (Ethernet.begin(mac, 10*1000, 4*1000) == 0) {
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
#ifdef SYNC_SERVER_USE_HTTPS
	EthernetClient client2;
	SSLClient client(client2, TAs, (size_t)TAs_NUM,A0);
#else
	EthernetClient client;
#endif
	client.setTimeout(10*1000);
	if (!client.connect(SYNC_SERVER_URL, SYNC_SERVER_PORT)) {
#ifdef DEBUG_ETHERNET
		Serial.println(String("Couldn't connect to ") + String(SYNC_SERVER_URL) + String(" on port ")+String(SYNC_SERVER_PORT));
#endif
		if(Ethernet.hardwareStatus() == EthernetNoHardware){
			setStatus(NoHardware);
		}
		else if(Ethernet.linkStatus() != LinkON){
			setStatus(NoLink);
		}

		return "";
	}

	String request = HttpParser::getRequestStr(path, SYNC_SERVER_URL);

#ifdef DEBUG_ETHERNET
	Serial.println("Sending request over Ethernet :");
	Serial.println(request);
#endif

	char requestArray[256];
	request.toCharArray(requestArray, 256);
	client.write((uint8_t*)requestArray, strlen(requestArray));
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
	client.stop();
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

bool BaseEthernet::sendFileHTTPPost(const String &path, File *file) {
#ifdef SYNC_SERVER_USE_HTTPS
	EthernetClient client2;
	SSLClient client(client2, TAs, (size_t)TAs_NUM,A0);
#else
	EthernetClient client;
#endif
	client.setTimeout(10*1000);
	if (!client.connect(SYNC_SERVER_URL, SYNC_SERVER_PORT)) {
#ifdef DEBUG_ETHERNET
		Serial.println(String("Couldn't connect to ") + String(SYNC_SERVER_URL) + String(" on port ")+String(SYNC_SERVER_PORT));
#endif
		if(Ethernet.hardwareStatus() == EthernetNoHardware){
			setStatus(NoHardware);
		}
		else if(Ethernet.linkStatus() != LinkON){
			setStatus(NoLink);
		}

		return "";
	}


	String request = "POST ";
	request += path;
	request += " HTTP/1.1\r\n";
	request += "Connection: close\r\n";;
	request += "Host: "+String(SYNC_SERVER_URL)+"\r\n";
	request += "Content-Length: ";
	request += String(file->size());
	request += "\r\nfilename: ";
	request += file->name();
	request += "\r\nX-API-Key: ";
	request += basePreferences->getDeviceKey();
	request += "\r\n\r\n";


#ifdef DEBUG_ETHERNET
	Serial.println("Sending HTTP POST header :\r\n"+request);
#endif

	char requestArray[256];
	request.toCharArray(requestArray, 256);
	client.write((uint8_t*)requestArray, strlen(requestArray));
#ifndef SYNC_SERVER_USE_HTTPS
	client.flush();
#endif

	char buff[512];
	int currentRead;
	int clientWritten;
#ifdef DEBUG_ETHERNET
	Serial.println("Ethernet : sending file content : "+String(file->size())+" bytes");
#endif

	sizePOSTTotal = file->size();
	sizePOSTSent = 0;
	while(sizePOSTSent < sizePOSTTotal && client.connected()){
		currentRead = file->readBytes(buff, 512);
		clientWritten = 0;
		while(currentRead > clientWritten && client.connected()){
			clientWritten += client.write((uint8_t *)(buff+clientWritten), currentRead);
#ifndef SYNC_SERVER_USE_HTTPS
			client.flush();
#endif
			vTaskDelay(20);
			esp_task_wdt_reset();
		}
		sizePOSTSent += currentRead;
	}
	sizePOSTTotal = -1;
	sizePOSTSent = -1;

	client.flush();

#ifdef DEBUG_ETHERNET
	Serial.println("Ethernet : done sending file content");
#endif

	unsigned long expire = millis() + 30*1000;
	String responseStr = "";
	while(millis() < expire && client.connected()){
		if(client.available()){
			responseStr = client.readString();
		}
		vTaskDelay(pdMS_TO_TICKS(200));
		esp_task_wdt_reset();
	}

	client.stop();

	HttpParser httpParser;
	httpParser.parseResponse(responseStr);

	if(httpParser.getStatusCode() == 200){
#ifdef DEBUG_ETHERNET
		Serial.println("File sent successfully !");
#endif
		return true;
	}
#ifdef DEBUG_ETHERNET
	Serial.println("Wrong return code sending file : "+String(httpParser.getStatusCode()));
	Serial.println("Reponse body :");
	Serial.println(httpParser.getBody());
#endif
	return false;
}

int BaseEthernet::getSizePostTotal() const {
	return sizePOSTTotal;
}

int BaseEthernet::getSizePostSent() const {
	return sizePOSTSent;
}

