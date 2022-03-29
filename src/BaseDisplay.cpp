#include "BaseDisplay.h"

BaseDisplay::BaseDisplay(BasePreferences *basePreferences) : display(0x3c, PIN_SDA, PIN_SCL), basePreferences(basePreferences) {
	state = Booting;
	display.init();
	//display.flipScreenVertically();
	display.setFont(ArialMT_Plain_10);
	display.clear();
	short x = display.getWidth() / 2;
	short y = display.getHeight() / 4;
	display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
	display.drawString(x, y, "Démarrage en cours...");
	display.display();
}

void BaseDisplay::updateDisplay() {
	display.clear();
	switch(ethernetStatus){
		case BaseEthernet::NoHardware:
			display.drawIco16x16(0, 0, epd_bitmap_warning, false);
			break;
		case BaseEthernet::NoLink:
			display.drawIco16x16(0, 0, epd_bitmap_ethernet_no_link, false);
			break;
		case BaseEthernet::ConnectingNoHardware:
			if(blink){
				display.drawIco16x16(0, 0, epd_bitmap_warning, false);
			}
			blink = !blink;
			break;
		case BaseEthernet::Connecting:
			if(blink){
				display.drawIco16x16(0, 0, epd_bitmap_ethernet_connect, false);
			}
			blink = !blink;
			break;
		case BaseEthernet::Connected:
			display.drawIco16x16(0, 0, epd_bitmap_ethernet_connect, false);
			break;
	}

	switch(wifiStatus) {
		case BaseWifi::AccessPointWaitingCredentials:
			display.drawIco16x16(20, 0, epd_bitmap_wifi_access_point, false);
			break;
		case BaseWifi::Connecting:
			if(blink) {
				display.drawIco16x16(20, 0, epd_bitmap_wifi, false);
			}
			blink = !blink;
			break;
		case BaseWifi::Connected:
			display.drawIco16x16(20, 0, epd_bitmap_wifi, false);
			break;
	}

	time_t now;
	struct tm timeDetails;
	time(&now);
	localtime_r(&now, &timeDetails);

	if(state == Booting){
		display.setFont(ArialMT_Plain_10);
		short x = display.getWidth() / 2;
		short y = display.getHeight() / 2;
		display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
		display.drawString(x, y, "En attente d'une\nconnexion internet");
	}
	else if(state == Idle){
		display.setFont(ArialMT_Plain_24);
		short x = display.getWidth() / 2;
		short y = display.getHeight() / 2;
		display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
		char buffer[16];
		sprintf(buffer, "%02d:%02d:%02d", timeDetails.tm_hour, timeDetails.tm_min, timeDetails.tm_sec);
		display.drawString(x, y, buffer);
	}
	else if(state == WebAuthentication){
		display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
		display.setFont(ArialMT_Plain_10);
		short x = display.getWidth() / 2;
		display.drawString(x, 8, "Auth. web");
		display.setTextAlignment(TEXT_ALIGN_LEFT);
		display.drawString(0, 24, "Vérification");
	}
	else if(state == WebAuthenticationCode){
		display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
		display.setFont(ArialMT_Plain_10);
		short x = display.getWidth() / 2;
		display.drawString(x, 8, "Auth. web");
		display.setTextAlignment(TEXT_ALIGN_LEFT);
		display.drawString(0, 24, "Code d'activation :");
		display.setTextAlignment(TEXT_ALIGN_CENTER);
		display.setFont(ArialMT_Plain_16);
		display.drawString(x, 40, basePreferences->getDeviceActivationKey());
	}
	else{
		display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
		display.setFont(ArialMT_Plain_10);
		short x = display.getWidth() / 2;
		display.drawString(x, 8, "Auth. web");
		display.setTextAlignment(TEXT_ALIGN_LEFT);
		display.drawString(0, 24, "Echec de la vérification !");
	}

	display.display();
}

void BaseDisplay::setEthernetStatus(BaseEthernet::Status status) {
	ethernetStatus = status;
}

void BaseDisplay::setWifiStatus(BaseWifi::Wifi_Status status) {
	wifiStatus = status;
}

void BaseDisplay::setState(BaseDisplay::State newState) {
	state = newState;
}
