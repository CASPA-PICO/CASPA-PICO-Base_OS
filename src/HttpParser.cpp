#include "HttpParser.h"

HttpParser::HttpParser() {
}

String HttpParser::getRequestStr(const String& path, const String& host) {
	String request = String("GET " + path + " HTTP/1.1\r\n");
	request += "Connection: close\r\n";
	request += "Host: "+host+"\r\n";
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

	if(headersMap->search("Transfer-Encoding").equals("chunked")){
		int debut = responseStr.indexOf("\r\n\r\n") + 4;
		int length;
		while(debut < responseStr.length()-1){
			length = (int)StrToHex(responseStr.substring(debut, responseStr.indexOf("\r\n", debut)).c_str());
			if(length == 0){
				break;
			}
			debut = responseStr.indexOf("\r\n", debut) + 2;
			body += responseStr.substring(debut, debut+length);
			debut += length+2;
		}
	}
	else{
		body = responseStr.substring(responseStr.indexOf("\r\n\r\n") + 4);
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

uint64_t HttpParser::StrToHex(const char* str)
{
	return (uint64_t) strtoull(str, nullptr, 16);
}