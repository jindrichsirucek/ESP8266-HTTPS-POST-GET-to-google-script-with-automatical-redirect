#include "FS.h"
#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"


#define AP_SSID "UPC3049010"
#define AP_PASSWORD "fdfsdfsfs"

#define DATA_SERVER_HOST_ADDRESS F("script.google.com")
#define DATA_SERVER_SCRIPT_URL F("/macros/s/AKfycbziCOySJ-cxsjn6-v8WpIbcsmlE77RqkzGX728nht2wO4HYsvVK/exec"); //deployed script google


void setup() 
{
 
	WiFi.begin(AP_SSID, AP_PASSWORD);

	const String host = DATA_SERVER_HOST_ADDRESS;
	String url = DATA_SERVER_SCRIPT_URL;


	char IPString[50];
	IPAddress ip = WiFi.localIP();
	sprintf(IPString, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  
	url += "?chipId=" + String(ESP.getChipId(), HEX) + "&ip=" + URLEncode(IPString);



	HTTPSRedirect client;

	if (!client.GET(host, url))
	{
		client.stop();      
		Serial.println("GET NOT sent!");
	}
	else
	{
		client.stop();
		Serial.println("GET sent!");
	}


	/////// POST FILE ///////



	File fileToSent = SPIFFS.open("/log.txt","w");
	fileToSent.println("Some data to sent..");
	fileToSent.close();


	fileToSent = SPIFFS.open("/log.txt","r");

	if (!client.POST_FILE(host, url, fileToSent))
	{
		client.stop();      
		Serial.println("File NOT sent!");
	}
	else
	{
		client.stop();
		Serial.println("File sent!");
	}
}

void loop() {
  // put your main code here, to run repeatedly:
}




String URLEncode(String msg){return URLEncode(msg.c_str());}
String URLEncode(const char* msg)
{
  const char *hex = "0123456789abcdef";
  String encodedMsg = "";

  while(*msg != '\0') {
    if( ('a' <= *msg && *msg <= 'z')
         || ('A' <= *msg && *msg <= 'Z')
         || ('0' <= *msg && *msg <= '9') ) {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
}