/*
Geekstips.com
IoT project - Communication between two ESP8266 - Talk with Each Other
ESP8266 Arduino code example
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <LiquidCrystal.h>

#define tempSelector A0 // Pot for selecting a temperature 


// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 5, en = 0, d4 = 1, d5 = 2, d6 = 3, d7 = 4;
LiquidCrystal lcd(D5, D0, D1, D2, D3, D4);// rs, en, d4, d5, d6, d7

// AP Wi-Fi credentials
const char* ssid = "2degrees Broadband - 772F"; //your WiFi Name
const char* password = "38389716620490527637";  //Your Wifi Password

// Local ESP web-server address
String serverHost = "http://192.168.178.41/feed";
String data;
// DEEP_SLEEP Timeout interval
int sleepInterval = 5;
// DEEP_SLEEP Timeout interval when connecting to AP fails
int failConnectRetryInterval = 2;
int counter = 0;

float h;
float t;
// Static network configuration
IPAddress ip(192, 168, 4, 4);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);


WiFiClient client;

void setup() {
//  ESP.eraseConfig();
  WiFi.persistent(false);
  Serial.begin(115200);
//  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("**************************");
  Serial.println("**************************");
  Serial.println("******** BEGIN ***********");
  lcd.begin(16, 2);
  delay(500);
  Serial.println("- set ESP STA mode");
  WiFi.mode(WIFI_OFF);  
  WiFi.mode(WIFI_STA);
  Serial.println("- connecting to wifi");
  ArduinoOTA.setPassword((const char *)"YourMomGay");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
//  WiFi.config(ip, gateway, subnet); 
//  WiFi.setOutputPower(0);
  WiFi.begin(ssid, password);
  Serial.println("");
  lcd.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
//    if(counter > 20){
//       Serial.println("- can't connect, going to sleep");    
//    }
    delay(500);
    Serial.print(".");
    counter++;
  }
  pinMode(tempSelector, INPUT);
  lcd.clear();
  lcd.print("Connected");
//  Serial.println("- read DHT sensor");
//  readDHTSensor();
  t = analogRead(tempSelector);
  t = map(t,0,1023,14,24);
  Serial.println("- build DATA stream string");
  buildDataStream();
  Serial.println("- send GET request");
  sendHttpRequest();
  Serial.println();
  Serial.println("- got back to sleep");
  Serial.println("**************************");
  Serial.println("**************************");
//  hibernate(sleepInterval);
}

void sendHttpRequest() {
  HTTPClient http;
  http.begin(serverHost);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(data);
  http.writeToStream(&Serial);
  http.end();
}

//void readDHTSensor() {
//  delay(200);
//  h = dht.readHumidity();
//  t = dht.readTemperature();
//  if (isnan(h) || isnan(t)) {
//    t = 0.00;
//    h = 0.00;
//  }
//  Serial.println("- temperature read : "+String(t));
//  Serial.println("- humidity read : "+String(h));
//}

void buildDataStream() {
  data = "temp=";
  data += String(t);
  data += "&hum=";
  data += String(h);
  Serial.println("- data stream: "+data);
}

void sendData() {
  buildDataStream();
  sendHttpRequest();
}


void loop() {
  for (int i = 0; i < 100; i++)
  { 
    t = analogRead(tempSelector);
    t = map(t,0,1023,14,24);
    lcd.clear();
    lcd.print("Desired Temp: "+String(t));
    delay(100);
  }
  sendData();
}
