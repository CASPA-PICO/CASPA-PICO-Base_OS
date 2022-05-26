#include "BaseBluetooth.h"

BaseBluetooth::BaseBluetooth(){
	lastReceivedFile.filename = nullptr;
	lastReceivedFile.totalSize = -1;
	lastReceivedFile.totalReceived = -1;
}

/**
 * Démarrage de la syncrhonisation Bluetooth avec le capteur
 */
void BaseBluetooth::startSync() {
#ifdef DEBUG_BLUETOOTH_SYNC
	Serial.println("Begin sync with sensor...");
#endif
	status = Connecting;
	if(!pltp.begin()){
#ifdef DEBUG_BLUETOOTH_SYNC
		Serial.println("Failed to begin PLTP !");
#endif
		status = Off;
		return;
	}
	esp_task_wdt_reset();

#ifdef DEBUG_BLUETOOTH_SYNC
	Serial.println("PLTP init success !");
#endif
	status = Connected;

	//On commence par envoyer l'heure au capteur
	if(!pltp.sendTime()){
#ifdef DEBUG_BLUETOOTH_SYNC
		Serial.println("PLTP failed to send time !");
#endif
		pltp.end();
		status = Off;
		return;
	}

	lastReceivedFile.totalSize = 1;
	lastReceivedFile.totalReceived = 0;
	esp_task_wdt_reset();


	PLTP::Message message;
	File file;
	bool doneWritingFile;
	//On boucle tant qu'on reçoit des messages du capteur
	while(pltp.processMessage(10*1000)){
		message = pltp.getLastMessage();
		//Si le message est de type FileInfo : on enregistre les infos sur le fichier qu'on va recevoir et on l'ouvre en vu d'une écriture
		if(message.type == PLTP::FileInfo){
			lastReceivedFile.filename = new char[message.size-5];
			memcpy(&lastReceivedFile.totalSize, &message.content[1], 4);
			memcpy(lastReceivedFile.filename+1, &message.content[6], message.size-7);
			lastReceivedFile.filename[0] = '/';
			lastReceivedFile.filename[message.size-6] = '\0';
			lastReceivedFile.totalReceived = 0;
			file = SPIFFS.open(lastReceivedFile.filename, FILE_WRITE, true);
			doneWritingFile = false;
			if(!file){
#ifdef DEBUG_BLUETOOTH_SYNC
				Serial.println("Failed to open file "+String(lastReceivedFile.filename));
#endif
			}
#ifdef DEBUG_BLUETOOTH_SYNC
			Serial.println("Bluetooth receiving file info : "+String(lastReceivedFile.filename)+" ("+String(lastReceivedFile.totalSize)+" bytes)");
#endif
		}
		//Si le message est de type FileContent : on écrit dans le fichier son contenu qu'on reçoit
		else if(message.type == PLTP::FileContent){
			if(lastReceivedFile.totalSize >= 0){
				lastReceivedFile.totalReceived += message.size - 1;
#ifdef DEBUG_BLUETOOTH_SYNC
				Serial.println("Received file content "+String(lastReceivedFile.totalReceived*100/lastReceivedFile.totalSize)+"% ("+String(lastReceivedFile.totalReceived)+"/"+String(lastReceivedFile.totalSize)+" bytes)");
#endif
				if(file){
					file.write(message.content+1, message.size-1);
					if(lastReceivedFile.totalReceived >= lastReceivedFile.totalSize){
#ifdef DEBUG_BLUETOOTH_SYNC
						Serial.println("Completely received file "+String(lastReceivedFile.filename));
#endif
						file.flush();
						file.close();
						doneWritingFile = true;
						delete[] lastReceivedFile.filename;
						lastReceivedFile.filename = nullptr;
					}
				}
			}
		}
	}

	//Si la connexion est interrompue avant d'avoir reçu le fichier en entier : on supprime le fichier partiellement reçu
	if(file && !doneWritingFile){
#ifdef DEBUG_BLUETOOTH_SYNC
		Serial.println("File not completely transferred : deleting "+String(lastReceivedFile.filename));
#endif
		file.close();
		SPIFFS.remove(lastReceivedFile.filename);
	}

#ifdef DEBUG_BLUETOOTH_SYNC
	Serial.println("PLTP ended !");
#endif
	delete[] lastReceivedFile.filename;
	lastReceivedFile.totalSize = -1;
	lastReceivedFile.totalReceived = -1;
	pltp.end();
	status = Off;
}

/**
 * @return Nombre d'octets reçus pour le fichier en cours de transfert
 */
int BaseBluetooth::getBytesReceived() {
	if(lastReceivedFile.totalSize != -1 && lastReceivedFile.totalReceived != -1){
		return lastReceivedFile.totalReceived;
	}

	return -1;
}

/**
 * @return Nombre d'octets total (taille du fichier) en cours de transfert
 */
int BaseBluetooth::getTotalBytes() {
	if(lastReceivedFile.totalSize != -1 && lastReceivedFile.totalReceived != -1){
		return lastReceivedFile.totalSize;
	}

	return -1;
}
