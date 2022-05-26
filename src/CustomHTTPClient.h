#ifndef BASE_OS_CUSTOMHTTPCLIENT_H
#define BASE_OS_CUSTOMHTTPCLIENT_H

#include <HTTPClient.h>

class CustomHTTPClient : public HTTPClient{
public:
	bool startConnection();
	static const char* getRootCA();
};


#endif //BASE_OS_CUSTOMHTTPCLIENT_H
