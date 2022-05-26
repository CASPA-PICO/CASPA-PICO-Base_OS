#include "BaseDisplay.h"

BaseDisplay::BaseDisplay(BasePreferences *basePreferences) : display(0x3c, PIN_SDA, PIN_SCL), basePreferences(basePreferences) {
	state = Booting;
	bluetoothBytesReceived = -1;
	bluetoothTotalBytes = -1;
	bluetoothETA = -1;
	internetETA = -1;
	display.init();
	if(FLIP_SCREEN_VERTICALLY) {
		display.flipScreenVertically();
	}
	display.setFont(ArialMT_Plain_10);
	display.clear();
	short x = display.getWidth() / 2;
	short y = display.getHeight() / 4;
	display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
	display.drawString(x, y, "Démarrage en cours...");
	display.display();
}

/**
 * Fonction principale, appelée toute les secondes par la tâche affichage
 */
void BaseDisplay::updateDisplay() {
	display.clear();
	//Blink nous permet de faire clignoter une icône
	blink = !blink;
	//On affiche une icône différente en fonction de l'état de la connexion Ethernet
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
			break;
		case BaseEthernet::Connecting:
			if(blink){
				display.drawIco16x16(0, 0, epd_bitmap_ethernet_connect, false);
			}
			break;
		case BaseEthernet::Connected:
			display.drawIco16x16(0, 0, epd_bitmap_ethernet_connect, false);
			break;
	}

	//On affiche une icône différente en fonction de l'état de la connexion Wifi
	if(bluetoothStatus == BaseBluetooth::Off) {
		switch (wifiStatus) {
			case BaseWifi::AccessPointWaitingCredentials:
				display.drawIco16x16(20, 0, epd_bitmap_wifi_access_point, false);
				break;
			case BaseWifi::Connecting:
				if (blink) {
					display.drawIco16x16(20, 0, epd_bitmap_wifi, false);
				}
				break;
			case BaseWifi::Connected:
				display.drawIco16x16(20, 0, epd_bitmap_wifi, false);
				break;
		}
	}

	//On affiche une icône différente en fonction de l'état de la connexion Bluetooth
	switch(bluetoothStatus){
		case BaseBluetooth::Off:
			break;
		case BaseBluetooth::Connecting:
			if(blink){
				display.drawIco16x16(20, 0, epd_bitmap_bluetooth, false);
			}
			break;
		case BaseBluetooth::Connected:
			display.drawIco16x16(20, 0, epd_bitmap_bluetooth, false);
			break;
	}

	//On récupère l'heure du microcontroleur
	time_t now;
	struct tm timeDetails;
	time(&now);
	localtime_r(&now, &timeDetails);

	//Si un transfert Bluetooth est en cours, on affiche sa progression
	if(bluetoothTotalBytes != -1 && bluetoothBytesReceived != -1){
		display.setFont(ArialMT_Plain_10);
		short x = display.getWidth() / 2;
		short y = display.getHeight() / 2;
		display.drawProgressBar(4, y-4, display.getWidth()-8, 8, bluetoothBytesReceived*100/bluetoothTotalBytes);
		display.setFont(ArialMT_Plain_10);
		display.setTextAlignment(TEXT_ALIGN_LEFT);
		display.drawString(38, 2, "Transfert Bluetooth");
		if(bluetoothETA != -1){
			char str[50];
			sprintf(str, "Temps restant : %d:%02d", bluetoothETA/60, bluetoothETA%60);
			display.drawString(4, display.getHeight()-18, String(str));
		}
		else{
			display.drawString(4, display.getHeight()-18, "Démarrage...");
		}
	}
	else if(state == Booting){
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
	else if(state == Error){
		display.setFont(ArialMT_Plain_16);
		short x = display.getWidth() / 2;
		short y = display.getHeight() * 5 / 12;
		display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
		display.drawString(x, y, errorTitle);
		y = display.getHeight() * 8 / 12;
		display.setFont(ArialMT_Plain_10);
		display.drawStringMaxWidth(x, y, display.getWidth(), errorStr);
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
	else if(state == DataTransfer){
		display.setFont(ArialMT_Plain_10);
		short x = display.getWidth() / 2;
		short y = display.getHeight() / 2;
		display.drawProgressBar(4, y-4, display.getWidth()-8, 8, internetBytesSent*100/internetTotalBytes);
		display.setFont(ArialMT_Plain_10);
		display.setTextAlignment(TEXT_ALIGN_LEFT);
		display.drawString(38, 2, "Transfert internet");
		if(internetETA != -1){
			char str[50];
			sprintf(str, "Temps restant : %d:%02d", internetETA/60, internetETA%60);
			display.drawString(4, display.getHeight()-18, String(str));
		}
		else{
			display.drawString(4, display.getHeight()-18, "Démarrage...");
		}
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

void BaseDisplay::setBluetoothStatus(BaseBluetooth::Status status) {
	bluetoothStatus = status;
}

void BaseDisplay::setState(BaseDisplay::State newState) {
	state = newState;
}

void BaseDisplay::showError(String errorTitle, String errorStr) {
	this->errorTitle = errorTitle;
	this->errorStr = errorStr;
	state = Error;
}

void BaseDisplay::setBluetoothProgress(int bytesReceived, int totalBytes) {
	if(bluetoothTotalBytes != -1 && bluetoothBytesReceived != -1){
		int diff = bytesReceived-bluetoothBytesReceived;
		if(diff > 0){
			bluetoothETA = (totalBytes-bytesReceived)/diff;
		}
	}
	bluetoothBytesReceived = bytesReceived;
	bluetoothTotalBytes = totalBytes;
}

void BaseDisplay::setInternetProgress(int bytesSent, int totalBytes) {
	if(internetTotalBytes != -1 && internetBytesSent != -1){
		int diff = bytesSent-internetBytesSent;
		if(diff > 0){
			internetETA = (totalBytes-bytesSent)/diff;
		}
	}
	internetBytesSent = bytesSent;
	internetTotalBytes = totalBytes;
}
