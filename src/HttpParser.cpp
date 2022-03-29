#include "HttpParser.h"

HttpParser::HttpParser() {
}

String HttpParser::getRequestStr(const String& path) {
	String request = String("GET " + path + " HTTP/1.1\r\n");
	request += "Connection: close\r\n";
	request += "\r\n";
	return request;
}

void HttpParser::parseRequestHeaders(const String &requestStr) {
	statusCode = 0;
	requestMethod = requestStr.substring(0, requestStr.indexOf(" "));
	requestPath = requestStr.substring(requestStr.indexOf(" ")+1, requestStr.indexOf(" ", requestStr.indexOf(" ")+1));
	headers = requestStr.substring(0, requestStr.indexOf("\r\n\r\n")) + "\r\n";

	headersMap = new Dictionary();

	//Fill the headersMap
	String temp = headers.substring(headers.indexOf("\r\n") + 2);
	if (temp.lastIndexOf("\r\n") != temp.length() - 2) {
		temp = temp + "\r\n";
	}

	while (temp.length() > 0 && temp != "\r\n") {
		headersMap->insert(temp.substring(0, temp.indexOf(": ")),
						   temp.substring(temp.indexOf(": ") + 2, temp.indexOf("\r\n")));
		temp = temp.substring(temp.indexOf("\r\n") + 2);
	}
}

void HttpParser::parseResponse(const String &responseStr) {
	//split headers from body and get status code
	headers = responseStr.substring(0, responseStr.indexOf("\r\n\r\n")) + "\r\n";
	statusCode = responseStr.substring(responseStr.indexOf(" "),
									   responseStr.indexOf(" ", responseStr.indexOf(" ") + 1)).toInt();
	body = responseStr.substring(responseStr.indexOf("\r\n\r\n") + 4);
	headersMap = new Dictionary();

	//Fill the headersMap
	String temp = headers.substring(headers.indexOf("\r\n") + 2);
	if (temp.lastIndexOf("\r\n") != temp.length() - 2) {
		temp = temp + "\r\n";
	}

	while (temp.length() > 0 && temp != "\r\n") {
		headersMap->insert(temp.substring(0, temp.indexOf(": ")),
						   temp.substring(temp.indexOf(": ") + 2, temp.indexOf("\r\n")));
		temp = temp.substring(temp.indexOf("\r\n") + 2);
	}
}

const String &HttpParser::getRequestPath() const {
	return requestPath;
}

const String &HttpParser::getRequestMethod() const {
	return requestMethod;
}

String HttpParser::getReponseStr(const String &body) {
	String response = "";
	response += "HTTP/1.1 200 OK\r\n";
	response += "Content-Length: "+String(body.length())+"\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Connection: Closed\r\n";
	response += "\r\n";
	response += body;
	return response;
}
