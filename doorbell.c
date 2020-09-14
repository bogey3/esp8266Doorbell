
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define EEPROM_SIZE 1

const char* ssid = "EnterSSIDHere";
const char* password = "EnterPasswordHere";

ESP8266WiFiMulti WiFiMulti;

const int r1 = D5;
const int button = D6 ;
bool enabled = true;
bool sendToHomeKit = false;
ESP8266WebServer server(80);

void handleInterrupt() {
  Serial.println("Sending to homekit");
  //HTTP requests in interrupts don't work, set this boolean and the loop will perform the http request to send the status to homekit
  sendToHomeKit = true;
  if(enabled){
    digitalWrite(r1, LOW);
    while(digitalRead(button) == LOW){
    }
    digitalWrite(r1, HIGH);
  }else{
    Serial.println("Doorbell is disabled");
  }
  
}

void handleRoot() {
  server.send(200, "text/plain", "Wifi doorbell enabled!");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  //Retrieve previous value of variable from eeprom
  enabled = EEPROM.read(0) == 1;
    
  pinMode(r1, OUTPUT_OPEN_DRAIN);
  digitalWrite(r1, HIGH);
  pinMode(button, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(button), handleInterrupt, FALLING);

  server.on("/", handleRoot);

  server.on("/on", []() {
    //Enable the doorbell chime when this url is retrieved
    server.send(200, "text/plain", "Enabled bell");
    enabled = true;
    Serial.println("Enabled bell");
    //Write updated status to EEPROM so status will remain after reboots
    EEPROM.write(0, 1);
    EEPROM.commit();
  });
  server.on("/off", []() {
    //Enable the doorbell chime when this url is retrieved
    server.send(200, "text/plain", "Disabled bell");
    enabled = false;
    Serial.println("Disabled bell");
    //Write updated status to EEPROM so status will remain after reboots
    EEPROM.write(0, 0);
    EEPROM.commit();
  });
  server.on("/status", []() {
    String out = "";  
    if(enabled){
      out = "True";
    }else{
      out = "False";
    }
    server.send(200, "text/plain", out);
  });
  server.on("/s", []() {
    //This is the status URL for the homebridge switch plugin I use (https://www.npmjs.com/package/homebridge-http-switch)
    if (enabled){
      server.send(200, "text/plain", "1");
    }else{
      server.send(200, "text/plain", "0");
    }
   
  });
 
  server.onNotFound(handleNotFound);

  WiFiMulti.addAP(ssid, password);
  while(WiFiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("HTTP server started");
  
}


void loop() {
  server.handleClient();
  if (sendToHomeKit){
    //Send the http request to homebridge running a http doorbell plugin (https://www.npmjs.com/package/homebridge-http-doorbell), this will show up as a motion sensor
    HTTPClient http;
    http.begin("http://172.16.0.101:9053/front");
    http.GET();
    Serial.println("Sent to homekit");
    http.end();
    sendToHomeKit = false;
  }
}
