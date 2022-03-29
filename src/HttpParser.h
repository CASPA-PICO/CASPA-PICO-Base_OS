#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include <Arduino.h>
#include <DictionaryDeclarations.h>

class HttpParser {
public :
	HttpParser();

	static String getRequestStr(const String& path);
	static String getReponseStr(const String& body);

	Dictionary *getHeadersMap() { return headersMap; }

	String getHeaders() { return headers; }

	String getBody() { return body; }

	int getStatusCode() { return statusCode; }

	void parseRequestHeaders(const String& requestStr);
	void parseResponse(const String& responseStr);

	void setBody(String body){this->body = body;}

	const String &getRequestPath() const;

	const String &getRequestMethod() const;

private :
	Dictionary *headersMap;
	String headers;
	String body;
	int statusCode;
	String requestPath;
	String requestMethod;
};

#endif
