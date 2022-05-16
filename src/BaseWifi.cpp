#include "BaseWifi.h"


BaseWifi::BaseWifi(BasePreferences *basePreferences) : webServer(AsyncWebServer(PORT_SERVER)){
	WiFi.mode(WIFI_OFF);
	preferences = basePreferences;
	webServer.addHandler(this);
}

void BaseWifi::startAccessPoint() {
	if(!WiFi.mode(WIFI_AP_STA)){
#ifdef DEBUG_WIFI
		Serial.println("Failed to startAccessPoint : failed to change WiFi mode to WIFI_AP_STA !");
		return;
#endif
	}
	WiFi.scanNetworks(true, false);
	WiFi.softAPConfig(apIP, apIP, subnet);
	WiFi.softAP(ssid, password, 1, 0, 4);

	webServer.begin();
#ifdef DEBUG_WIFI
	Serial.println("Wifi access point and server started !");
#endif
	startDNSServer();
	loadRouterSettingsFromPreferences();
}

void BaseWifi::stopAccessPoint() {
	WiFi.mode(WIFI_STA);
	webServer.end();
	stopDNSServer();
#ifdef DEBUG_WIFI
	Serial.println("Wifi access point and server stopped !");
#endif
}

BaseWifi::DNS_Status BaseWifi::getDnsStatus() const {
	return dnsStatus;
}

void BaseWifi::startDNSServer() {
	if(dnsStatus == Stopped) {
		dnsServer.start(PORT_DNS, "*", apIP);
		dnsStatus = Running;
#ifdef DEBUG_WIFI
		Serial.println("Wifi DNS started !");
#endif
	}
}

void BaseWifi::stopDNSServer() {
	if(dnsStatus == Running) {
#ifdef DEBUG_WIFI
		Serial.println("Stopping DNS server");
#endif
		dnsServer.stop();
		dnsStatus = Stopped;
	}
}

void BaseWifi::processDNS() {
	dnsServer.processNextRequest();
}

BaseWifi::Wifi_Status BaseWifi::getWifiStatus() const {
	return wifiStatus;
}

bool BaseWifi::canHandle(AsyncWebServerRequest *request) {
	return true;
}

void BaseWifi::handleRequest(AsyncWebServerRequest *request) {
	if(request->host() != "base.caspa-pico.fr" && wifiStatus != Connected){
		request->redirect("http://base.caspa-pico.fr");
		return;
	}
	else if(request->method() == HTTP_GET){
		if(request->url() == "/"){
			if(wifiStatus == Connected){
				AsyncResponseStream *response = request->beginResponseStream("text/html");
				response->print(generateConnectedPage());
				request->send(response);
				return;
			}
			else if(wifiStatus == Connecting){
				request->redirect("/connecting");
				return;
			}
#ifdef DEBUG_WIFI
			Serial.println("Wifi : sending portal page");
#endif
			AsyncResponseStream *response = request->beginResponseStream("text/html");
			response->print(generatePortal());
			request->send(response);
		}
		else if(request->url() == "/favicon.ico"){
#ifdef DEBUG_WIFI
			Serial.println("Wifi : sending favicon");
#endif
			AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon_ico, favicon_ico_len);
			request->send(response);
		}
		else if(request->url() == "/refreshWifi"){
#ifdef DEBUG_WIFI
			Serial.println("Refreshing wifi networks");
#endif
			unsigned long start = millis();
			WiFi.scanNetworks(true, false);
			while(WiFi.scanComplete() == -1 && millis()-start < 10*1000){
				esp_task_wdt_reset();
				vTaskDelay(pdMS_TO_TICKS(1000));
			}
#ifdef DEBUG_WIFI
			Serial.println("Found "+String(WiFi.scanComplete())+" wifi networks");
#endif
			request->redirect("/");
		}
		else if(request->url() == "/connect"){
			if(request->hasParam("SSID") && request->getParam("SSID")->value() != ""
			   && request->hasParam("password") && request->getParam("password")->value() != ""){
				if(wifiStatus == Connected){
					request->redirect("/");
					return;
				}
				if(wifiStatus == Connecting){
					WiFi.disconnect();
					failedConnection();
					preferences->setRouterSSIDandPassword("", "");
				}
				routerSSID = request->getParam("SSID")->value();
				routerPassword = request->getParam("password")->value();
#ifdef DEBUG_WIFI
				Serial.println("Got wifi credentials !");
				Serial.println("SSID : "+routerSSID);
				Serial.println("Password : "+routerPassword);
#endif
				request->redirect("/connecting");
				return;
			}
			request->redirect("/");
		}
		else if(request->url() == "/connecting"){
			if(wifiStatus == Connecting){
				AsyncResponseStream *response = request->beginResponseStream("text/html");
				response->print(generateWaitingPage());
				request->send(response);
			}
			else{
				request->redirect("/");
			}
		}
		else if(request->url() == "/disconnect"){
			if(wifiStatus == Connected){
				failedConnection();
				preferences->setRouterSSIDandPassword("", "");
				WiFi.disconnect();
				request->redirect("/");
			}
		}
		return;
	}
	request->send(404);
}

void BaseWifi::failedConnection() {
	WiFi.disconnect();
	wifiStatus = AccessPointWaitingCredentials;
	routerSSID = "";
	routerPassword = "";
}

String BaseWifi::generatePortal() const{
	String result =		"<!doctype html><html lang=\"fr\">"
						"<head><meta charset='UTF-8'>"
						"<title>Base CASPA-PICO</title>"
						"<style>html { font-family: Tahoma; display: flex; flex-direction: column; justify-content:center; margin: 2px auto; text-align: center; align-items: center;}"
							".button { background-color: green; border: none; color: white; padding: 20px 48px;"
							"text-decoration: none; font-size: 30px; margin: 8px; cursor: pointer; border-radius: 40px;}"
							"fieldset{background-color: #eeeeee; border: none; border-radius: 28px; padding: 15px;  width:440px; min-width=440px;}"
						"</style></head>"
						"<body><h1>CASPA-PICO</h1>";
				result += "<h3>Connectons la base à votre réseau wifi</h3>";
				result += "</br><form action=\"/connect\" method=\"get\">"
						  "<fieldset>Voici les réseaux disponibles à proximité, choisissez le votre :</br>"
						  "</br>SSID: <select id=\"SSID\" name=\"SSID\">";
				for(int i=0; i<WiFi.scanComplete(); i++){
					result += "<option>"+WiFi.SSID(i)+"</option>";
				}
				result += "</select> <input type=\"button\" value=\"Rafraichir\" onclick=\"this.value='Rafraichissement en cours...';location.href='/refreshWifi'\"></fieldset>";
				result += "</br></br><fieldset>Entrez le mot de passe wifi ici :</br></br>"
						  "Mot de passe : <input type=\"password\" name=\"password\"></fieldset></br></br>"
						  "<input class=\"button\" type=\"submit\" value=\"Valider\"></form>";
			result +=	"</body>"
						"</html>";

	return result;
}

String BaseWifi::generateWaitingPage() const {
	String result =		"<!doctype html><html lang=\"fr\">"
						   "<head><meta charset='UTF-8'>"
						   "<meta http-equiv=\"refresh\" content=\"5\">"
						   "<title>Base CASPA-PICO</title>"
						   "<style>html { font-family: Tahoma; display: flex; flex-direction: column; justify-content:center; margin: 2px auto; text-align: center; align-items: center;}.wifi-symbol{display:none;}.wifi-symbol [foo], .wifi-symbol{display:block;position:absolute;top:50%;display:inline-block;width:150px;height:150px;margin-top:-187.5px;-ms-transform:rotate(-45deg) translate(-100px);-moz-transform:rotate(-45deg) translate(-100px);-o-transform:rotate(-45deg) translate(-100px);-webkit-transform:rotate(-45deg) translate(-100px);transform:rotate(-45deg) translate(-100px);}.wifi-symbol .wifi-circle{box-sizing:border-box;-moz-box-sizing:border-box;display:block;width:100%;height:100%;font-size:21.4285714286px;position:absolute;bottom:0;left:0;border-color:#000000;border-style:solid;border-width:1em 1em 0 0;-webkit-border-radius:0 100% 0 0;border-radius:0 100% 0 0;opacity:0;-o-animation:wifianimation 3s infinite;-moz-animation:wifianimation 3s infinite;-webkit-animation:wifianimation 3s infinite;animation:wifianimation 3s infinite;}.wifi-symbol .wifi-circle.first{-o-animation-delay:800ms;-moz-animation-delay:800ms;-webkit-animation-delay:800ms;animation-delay:800ms;}.wifi-symbol .wifi-circle.second{width:5em;height:5em;-o-animation-delay:400ms;-moz-animation-delay:400ms;-webkit-animation-delay:400ms;animation-delay:400ms;}.wifi-symbol .wifi-circle.third{width:3em;height:3em;}.wifi-symbol .wifi-circle.fourth{width:1em;height:1em;opacity:1;background-color:#000000;-o-animation:none;-moz-animation:none;-webkit-animation:none;animation:none;}@-o-keyframes wifianimation{0%{opacity:0.4;} 5%{opactiy:1;} 6%{opactiy:0.1;} 100%{opactiy:0.1;}}@-moz-keyframes wifianimation{0%{opacity:0.4;} 5%{opactiy:1;} 6%{opactiy:0.1;} 100%{opactiy:0.1;}}@-webkit-keyframes wifianimation{0%{opacity:0.4;} 5%{opactiy:1;} 6%{opactiy:0.1;} 100%{opactiy:0.1;}}"
						   "</style></head>"
						   "<body><h1>CASPA-PICO</h1>";
	result += "<h3>Connexion en cours au réseau "+routerSSID+"</br>Merci de patienter</h3>";
	result += "<div class=\"wifi-symbol\">"
			  "<div class=\"wifi-circle first\"></div>"
			  "<div class=\"wifi-circle second\"></div>"
			  "<div class=\"wifi-circle third\"></div>"
			  "<div class=\"wifi-circle fourth\">"
			  "</div>";
	result +=	"</body>"
				 "</html>";

	return result;
}

String BaseWifi::generateConnectedPage() const {
	String result =		"<!doctype html><html lang=\"fr\">"
						   "<head><meta charset='UTF-8'>"
						   "<title>Base CASPA-PICO</title>"
						   "<style>html { font-family: Tahoma; display: flex; flex-direction: column; justify-content:center; margin: 2px auto; text-align: center; align-items: center;}"
						   ".button { background-color: green; border: none; color: white; padding: 20px 48px;"
						   "text-decoration: none; font-size: 30px; margin: 8px; cursor: pointer; border-radius: 40px;}"
						   "fieldset{background-color: #eeeeee; border: none; border-radius: 28px; padding: 15px;  width:440px; min-width=440px;}"
						   "</style></head>"
						   "<body><h1>CASPA-PICO</h1>";
	result += "<h3>Connecté au réseau "+routerSSID+" !</h3>";
	result += "Adresse IP : "+WiFi.localIP().toString();
	result += "<br/><br/><a href=\"/disconnect\"><button>Se déconnecter</button></a>";
	result +=	"</body>"
				 "</html>";

	return result;
}

void BaseWifi::loadRouterSettingsFromPreferences() {
	routerSSID = preferences->getRouterSSID();
	routerPassword = preferences->getRouterPassword();
}

String BaseWifi::wifiHttpGet(const String &path) {
	HTTPClient http;
	http.setTimeout(10*1000);
	http.setConnectTimeout(10*1000);
#ifdef DEBUG_WIFI
	Serial.println(String("Connecting to ") + String(SERVER_URL)+path);
#endif
	if(!http.begin(SERVER_URL, 80, path)) {
#ifdef DEBUG_WIFI
		Serial.println(String("Couldn't connect to ") + String(SERVER_URL) + String(" on port 80 !"));
#endif
		return "";
	}
	int httpResponseCode = http.GET();
	String resultStr = http.getString();
	http.end();
#ifdef DEBUG_WIFI
	Serial.println("Got response from server over Wifi code "+String(httpResponseCode));
	Serial.println("Received : "+resultStr);
#endif
	if(httpResponseCode == 200){
		return resultStr;
	}
	return "";
}

bool BaseWifi::sendFileHTTPPost(const String &path, File *file) {
	HTTPClient http;
	http.setTimeout(10*1000);
	http.setConnectTimeout(10*1000);
#ifdef DEBUG_WIFI
	Serial.println(String("Connecting to ") + String(SERVER_URL)+path);
#endif
	if(!http.begin(SERVER_URL, 80)) {
#ifdef DEBUG_WIFI
		Serial.println(String("Couldn't begin connection to ") + String(SERVER_URL) + String(" on port 80 !"));
#endif
		return false;
	}

	if(!http.connect()){
#ifdef DEBUG_WIFI
		Serial.println(String("Couldn't connect to ") + String(SERVER_URL) + String(" on port 80 !"));
#endif
	}

#ifdef DEBUG_WIFI
	Serial.println("Wifi : sending file header");
#endif
	WiFiClient *stream = http.getStreamPtr();
	if(stream == nullptr){
#ifdef DEBUG_WIFI
		Serial.println("Wifi : http stream is null !");
#endif
		return false;
	}
	stream->write("POST ");
	stream->write(path.c_str());
	stream->write(" HTTP/1.1\r\n");
	stream->write("Connection: close\r\n");
	stream->write("Content-Length: ");
	stream->write(String(file->size()).c_str());
	stream->write("\r\nfilename: ");
	stream->write(file->name());
	stream->write("\r\nX-API-Key: ");
	stream->write(preferences->getDeviceKey().c_str());
	stream->write("\r\n\r\n");
	stream->flush();

	char buff[512];
	int currentRead;
	int streamWritten;
#ifdef DEBUG_WIFI
	Serial.println("Wifi : sending file content : "+String(file->size())+" bytes");
#endif
	sizePOSTTotal = file->size();
	sizePOSTSent = 0;
	while(sizePOSTSent < sizePOSTTotal && http.connected()){
		currentRead = file->readBytes(buff, 512);
		streamWritten = 0;
		while(currentRead > streamWritten && http.connected()){
			streamWritten += stream->write(buff+streamWritten, currentRead);
			stream->flush();
			vTaskDelay(20);
			esp_task_wdt_reset();
		}
		sizePOSTSent += currentRead;
	}
	sizePOSTTotal = -1;
	sizePOSTSent = -1;

#ifdef DEBUG_WIFI
	Serial.println("Wifi : done sending file content");
#endif

	unsigned long expire = millis() + 30*1000;
	String responseStr = "";
	while(millis() < expire && stream->connected()){
		if(stream->available()){
			responseStr = http.getString();
		}
		else{
			vTaskDelay(pdMS_TO_TICKS(200));
		}
		esp_task_wdt_reset();
	}

	http.end();

	HttpParser httpParser;
	httpParser.parseResponse(responseStr);

	if(httpParser.getStatusCode() == 200){
#ifdef DEBUG_WIFI
		Serial.println("File sent successfully !");
#endif
		return true;
	}
#ifdef DEBUG_WIFI
	Serial.println("Wrong return code sending file : "+String(httpParser.getStatusCode()));
	Serial.println("Reponse body :");
	Serial.println(httpParser.getBody());
#endif
	return false;
}

int BaseWifi::getSizePostTotal() const {
	return sizePOSTTotal;
}

int BaseWifi::getSizePostSent() const {
	return sizePOSTSent;
}
