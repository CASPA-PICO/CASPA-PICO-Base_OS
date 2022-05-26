#include "HttpParser.h"

HttpParser::HttpParser() {
}

/**
 * Renvoie une requête HTTP GET (pour l'écrire sur un socket TCP)
 * @param path : chemin de la requête HTTP GET
 * @param host : host de la requête HTTP GET
 * @return Renvoie une chaîne de caractère contenant la requête HTTP GET
 */
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

/**
 * Lit la réponse du serveur et la "met en forme" pour une lecture plus facile
 * (Headers dans un dictionnaire et corps de la réponse dans une seule chaîne de caractère)
 * @param responseStr : chaîne de caractère contenant la réponse du serveur web
 */
void HttpParser::parseResponse(const String &responseStr) {
	//On séparer les entêtes de la réponse du corps de la réponse et on récupère le code de retour
	headers = responseStr.substring(0, responseStr.indexOf("\r\n\r\n")) + "\r\n";
	statusCode = responseStr.substring(responseStr.indexOf(" "),
									   responseStr.indexOf(" ", responseStr.indexOf(" ") + 1)).toInt();
	headersMap = new Dictionary();

	//On remplit le dictionnaire des entêtes
	String temp = headers.substring(headers.indexOf("\r\n") + 2);
	if (temp.lastIndexOf("\r\n") != temp.length() - 2) {
		temp = temp + "\r\n";
	}

	while (temp.length() > 0 && temp != "\r\n") {
		headersMap->insert(temp.substring(0, temp.indexOf(": ")),
						   temp.substring(temp.indexOf(": ") + 2, temp.indexOf("\r\n")));
		temp = temp.substring(temp.indexOf("\r\n") + 2);
	}

	//On lit le corps de la réponse (qui peut être fragmenté si l'entête Transfer-Encoding: chunked est présent)
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
/**
 * Converti une chaîne de caractères hexadécimaux en chiffre représenté par cette chaîne de caractère
 * Ex : "4f" -> 79
 * @param str : chaîne de caractères hexadécimaux
 * @return Chiffre décimal représenté par la chaîne de caractère
 */
uint64_t HttpParser::StrToHex(const char* str)
{
	return (uint64_t) strtoull(str, nullptr, 16);
}