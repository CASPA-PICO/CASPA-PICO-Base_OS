#include "BaseWifi.h"


BaseWifi::BaseWifi(BasePreferences *basePreferences) : webServer(AsyncWebServer(PORT_SERVER)){
	WiFi.mode(WIFI_OFF);
	preferences = basePreferences;
}

void BaseWifi::startAccessPoint() {
	WiFi.mode(WIFI_AP_STA);
	WiFi.scanNetworks(true, false, false, 500, 0);
	WiFi.softAPConfig(apIP, apIP, subnet);
	WiFi.softAP(ssid, password, 1, 0, 4);

	webServer.addHandler(this);
	webServer.begin();
#ifdef DEBUG_WIFI
	Serial.println("Wifi access point and server started !");
#endif
	startDNSServer();
	loadRouterSettingsFromPreferences();
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
			while(WiFi.scanNetworks(true, false) < 0 && millis()-start < 10*1000){
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
