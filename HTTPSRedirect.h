#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#ifndef HTTPS_REDIRECT_DEBUG
  #define HTTPS_REDIRECT_DEBUG false
#endif

#ifdef DEBUG_OUTPUT
#else
#define DEBUG_OUTPUT Serial
#endif

typedef uint8_t (*CallBackFunction)(uint8_t);

class HTTPSRedirect : public WiFiClientSecure {
private:
   CallBackFunction animationProgressCallbackFunction;
   uint8_t animationProgressPosition = 0;
   bool isAnimationCallBackFunctionInitialized_flag = false;
   void animateProgress() {if(isAnimationCallBackFunctionInitialized_flag) animationProgressPosition = (*(animationProgressCallbackFunction))(animationProgressPosition);}

public:
   HTTPSRedirect(int port = 443) : m_port(port)
   { }
   void setAnimationProgressCallback(CallBackFunction func) {animationProgressCallbackFunction = func; isAnimationCallBackFunctionInitialized_flag = true;}

   bool GET(const String &host, const String &url)
   {
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(__PRETTY_FUNCTION__);
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf("!!!Free Heap: %u\n", ESP.getFreeHeap());

      if (establishConncetionWithServer(host, m_port) == false) return false;

      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print("requesting URL: ");
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(url.substring(0,50));

      print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "Accept: */*\r\n" +
         "Connection: close\r\n\r\n");

      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print("Sending GET request..");
      while (available() == 0) 
      {
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(".");
         delay(200);
         animateProgress();
      }
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println("Sent!");
      return processHeader();
   }


   bool POST_STRING(const String &host, const String &url, String stringToSend)
   {
      if(establishConncetionWithServer(host, m_port) == false)
         return false;

      setNoDelay(true);
      
      printHeaderPost(host, url, stringToSend.length());

      print(stringToSend);

      return readResponseAfterPostSent();
   }


   bool POST_FILE(const String &host, const String &url, File fileToSend)
   {
      if(establishConncetionWithServer(host, m_port) == false)
         return false;

      setNoDelay(true);
      
      printHeaderPost(host, url, fileToSend.size());

      printFileToClient(fileToSend);

      return readResponseAfterPostSent();
   }


protected:
   int m_port;

   bool establishConncetionWithServer(const String &host, const int &port)
   {        
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(__PRETTY_FUNCTION__);
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf("!!!Free Heap: %u\n", ESP.getFreeHeap());
      yield();
      animateProgress();

      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print("connecting to ");
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(host);

      bool success = connect(host.c_str(), port);
      if (!success) 
      {
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(" - NOT connected!");
         return false;
      }
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(" - Succesfully connected!");
      return true;
   }


   bool printHeaderPost(const String &host, const String &url, uint32_t postSize)
   {
      
      print(String("POST ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "Accept: */*\r\n" +
         "Content-Length: " + (postSize) + "\r\n" +
         "Connection: close\r\n\r\n");

   }


   void printFileToClient(File fileToSend)
   {
      uint32_t startTime = millis();
      if (HTTPS_REDIRECT_DEBUG) { DEBUG_OUTPUT.printf("Sending file: '%s' of size: %d B\n", fileToSend.name(), fileToSend.size());}

      uint16_t bufSize = 1760; 
      byte clientBuf[bufSize];
      uint16_t clientCount = 0;
      uint32_t alreadySent = 0;

      while (fileToSend.available() && alreadySent < fileToSend.size())
      {
         clientBuf[clientCount] = fileToSend.read();
         clientCount++;
         alreadySent++;

         if (clientCount > bufSize-1)
         {          
            write((const uint8_t *)clientBuf, bufSize);
            clientCount = 0;
            if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(".");
            animateProgress();
         }             
      } 

   // final < bufSize byte cleanup packet
      if (clientCount > 0) write((const uint8_t *)clientBuf, clientCount);
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf("Sent, in: %d seconds\n", ((millis() - startTime)/1000));
   // close the file:
      fileToSend.close();
   }

   bool readResponseAfterPostSent()
   {
      // if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println((""));
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print("Reading response..");

      uint32_t startTime = millis();

      uint8_t attempt = 100;
      while (available() == 0 && attempt > 0) 
      {
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf(".%d",available());
         delay(200);
         animateProgress();
         attempt--;
      }
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print((attempt == 0) ? "Timeout" : "Received");
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf(", in: %d seconds\n", ((millis() - startTime)/1000));

      return processHeader();
   }


   bool processHeader()
   {
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(__PRETTY_FUNCTION__);
      if (true) DEBUG_OUTPUT.printf("HTTPSRedirect Connection Free Heap: %u\n", ESP.getFreeHeap());

      while (available()) 
      {
         String line = readStringUntil('\n');
   // if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println("line: " + line);
         if (line.startsWith(("HTTP/1.1 302 Moved Temporarily")))
         {
            return redirect();
         }
         else if (line == "\r") {
            return true;
         }
      }
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println("!!Redirect NOT found!" );
      return false;
   }

   bool redirect()
   {
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(__PRETTY_FUNCTION__);        
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf("!!!Free Heap: %u\n", ESP.getFreeHeap());

      String locationStr = "Location: ";
      while (connected()) {
         String line = readStringUntil('\n');
         if (line.startsWith(locationStr)) {
            String address = line.substring(locationStr.length());
            String redirHost, redirUrl;
            parseUrl(address, redirHost, redirUrl);

            return GET(redirHost, redirUrl);
         }
         else if (line == "\r") {
            if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println((""));
            return false;
         }
      }

      return false;
   }

   void parseUrl(const String &address, String &host, String &url)
   {
//if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print();
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf("%s !Heap(%u)\n", String(__PRETTY_FUNCTION__).c_str(), ESP.getFreeHeap());

      int protocolEnd = address.indexOf("://");

      int hostBegin = protocolEnd;
      if (protocolEnd != 0) {
         hostBegin += 3;
      }

      int urlBegin = address.indexOf('/', hostBegin);
      host = address.substring(hostBegin, urlBegin);
      url = address.substring(urlBegin);
   }
};


