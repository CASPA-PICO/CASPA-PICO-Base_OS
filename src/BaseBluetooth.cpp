#include "BaseBluetooth.h"

BaseBluetooth::BaseBluetooth(){
	lastReceivedFile.filename = nullptr;
	lastReceivedFile.totalSize = -1;
	lastReceivedFile.totalReceived = -1;
}

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
	/*if(!SPIFFS.begin(true)){
#ifdef DEBUG_BLUETOOTH_SYNC
		Serial.println("Failed to open SPIFFS !");
#endif
		pltp.end();
		status = Off;
		return;
	}*/

	//SPIFFS.format();

	PLTP::Message message;
	File file;
	bool doneWritingFile;
	while(pltp.processMessage(5*1000)){
		message = pltp.getLastMessage();
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
						/*lastReceivedFile.totalSize = -1;
						lastReceivedFile.totalReceived = 0;*/
					}
				}
			}
		}
	}

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

int BaseBluetooth::getBytesReceived() {
	if(lastReceivedFile.totalSize != -1 && lastReceivedFile.totalReceived != -1){
		return lastReceivedFile.totalReceived;
	}

	return -1;
}

int BaseBluetooth::getTotalBytes() {
	if(lastReceivedFile.totalSize != -1 && lastReceivedFile.totalReceived != -1){
		return lastReceivedFile.totalSize;
	}

	return -1;
}
