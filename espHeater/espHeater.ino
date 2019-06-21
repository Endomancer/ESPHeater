#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define ONE_WIRE_BUS D1 // DS18B20 pin
#define ARRAY_MAX 100
#define GRAPHX 1000
#define GRAPHY 500
//#define relayPin 8
//#define interruptPin 2
//#define tempSelector A0

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
//
//const int tempSelector = A0;
//const int interruptPin = D2;
const int relayPin = D0;

const char* ssid = "2degrees Broadband - 772F"; //your WiFi Name
const char* password = "38389716620490527637";  //Your Wifi Password
String apiKey = "*************";
const char* logServer = "192.168.178.41";

String header;
int desiredTemp = 18;

int tempArray[ARRAY_MAX] = {0};
int currentStart = 0;
int currentValue = 0;
int state = 0;
int refValue = 0;
bool changeStart = false;

ESP8266WebServer server(80);

IPAddress ip(192, 168, 178, 41);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);
//char htmlResponse[3000];

//write the graph
String graphcode()
{
  String graph = "";
  int x = 50;
  int scaling = 0;
  int previous = tempArray[currentStart];
  if (changeStart == false) {
    scaling = int((GRAPHX - 50) / (1 + currentValue));
  } else {
    scaling = 9;
  }

  for (int i = currentStart + 1; i != currentValue; i++) {
    if (i >= ARRAY_MAX) {
      i -= 100;
    }
    x += scaling;
    graph += "ctx.beginPath();";
    graph += "ctx.moveTo(" + String(x - scaling) + "," + String(previous) + ");";
    graph += "ctx.lineTo(" + String(x) + "," + String(tempArray[i]) + ");";
    if (tempArray[i] <= int(-20 * desiredTemp + 400)) {
      graph += "ctx.strokeStyle = '#008000';";
    } else {
      graph += "ctx.strokeStyle = '#ff0000';";
    }
    graph += "ctx.stroke();";
    previous = tempArray[i];
  }
  return graph;
}

String cState() {
  if (state % 2 == 0) {
    return ("On");
  }
  else {
    return ("Off");
  }
}

void setupStMode(String t, String v){
  Serial.println("** SETUP STATION MODE **");
  Serial.println("- disconnect from any other modes");
  WiFi.disconnect();
  Serial.println();
  Serial.println("- connecting to Home Router SID: **********");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("- succesfully connected");
  Serial.println("- starting client");
  
  WiFiClient client;

  Serial.println("- connecting to Database server: " + String(logServer));
  if (client.connect(logServer, 80)) {
    Serial.println("- succesfully connected");
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(t);
    postStr += "&field2=";
    postStr += String(v);
    postStr += "\r\n\r\n";
    Serial.println("- sending data...");
    Serial.println(String(t));
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
  client.stop();
  Serial.println("- stopping the client");
  /** If your ESP does not respond you can just
  *** reset after each request sending 
  Serial.println("- trying reset");
  ESP.reset();
  **/
}

String htmlPageValue() {
  refValue++;
  return String(refValue);
}
//prepare the html code as a string
String prepareHtmlPage()
{

  String htmlPage =
    String("HTTP/1.1 200 OK\r\n") +
    "Content-Type: text/html\r\n" +
    "Connection: open\r\n" +
    "Refresh:60\r\n" +
    "\r\n" +
    "<!DOCTYPE html>" +
    "<body>" +//href idea from https://randomnerdtutorials.com/esp8266-web-server/
    "<p><a href=\"/h/" +htmlPageValue() + "\"<button id='Power' class='button'>" + cState() + "</button></a>" +
    " Current temperature: " +
    String(int((tempArray[currentValue] - 400) / -20))  +
    " Desired temperature: " + String(desiredTemp) + "</p>" +
    "<canvas id='myCanvas' width='1000' height='500'" +
    "style='border:1px solid #000000;'>" +
    "<script>" +
    "var canvas = document.getElementById('myCanvas');" +
    "var ctx = canvas.getContext('2d');" +
    "ctx.moveTo(50,0);" +
    "ctx.lineTo(50,500);" +
    "ctx.stroke();" +
    "ctx.moveTo(50,400);" +
    "ctx.lineTo(1000,400);" +
    "ctx.stroke();" +
    "ctx.font = '30px Arial';" +
    "ctx.fillText('20',10,25);" +
    "ctx.fillText('15',10,115);" +
    "ctx.fillText('10',10,215);" +
    "ctx.fillText('5',25,315);" +
    "ctx.fillText('0',25,415);" +
    "ctx.fillText('-5',10,485);" +
    graphcode() +
    "ctx.beginPath();" +
    "ctx.moveTo(50," + String(int(-20 * desiredTemp + 400)) + ");" +
    "ctx.lineTo(1000," + String(int(-20 * desiredTemp + 400)) + ");" +
    "ctx.strokeStyle = '#0000ff';" +
    "ctx.stroke();" +
    "</script>" +
    "</canvas>" +
    "</body>" +
    "</html>" +
    "\r\n";
  return htmlPage;
}

// Handling the / root web page from my server
void handle_index() {
  server.send(200, "text/html", prepareHtmlPage());
}

// Handling the /feed page from my server
void handle_feed() {
  String t = server.arg("temp");
  String h = server.arg("hum");
  desiredTemp = t.toInt();
  Serial.println("recieved something");
  server.send(200, "text/plain", "This is response to client");
  setupStMode(t, h);
}

void setupServer(){
  Serial.println("** SETUP SERVER **");
  Serial.println("- starting server :");
  server.on("/", handle_index);
  server.on("/feed", handle_feed);
  server.begin();
};

void setupAccessPoint(){
  Serial.println("** SETUP ACCESS POINT **");
  Serial.println("- disconnect from any other modes");
  WiFi.disconnect();
  Serial.println("- start ap with SID: "+ String(ssid));
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("- AP IP address is :");
  Serial.print(myIP);
  setupServer();
}

void setup() {
  Serial.begin(115200);
  //    pinMode(tempSelector, INPUT);
  //    pinMode(interruptPin, INPUT);
  pinMode(relayPin, OUTPUT);
  //    attachInterrupt(digitalPinToInterrupt(interruptPin), desired_temperature, CHANGE);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_AP_STA);
  WiFi.config(ip, gateway, subnet);
  setupAccessPoint();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // No authentication by default
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
  Serial.println("");
  Serial.println("WiFi connected");
  server.begin();
  Serial.println("Server Started");
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}


void onoroff()
{
  state += 1;
}

float read_temperature() {
  float temp = 1;
//  while (temp == 85.0 || temp == (-127.0)) {
//    DS18B20.requestTemperatures();
//    temp = DS18B20.getTempCByIndex(0);
//    Serial.print("Temperature: ");
//    Serial.println(temp);
//  }
  return (temp);
}


void loop() {
  //check to start adding to the start position
  ArduinoOTA.handle();
  server.handleClient();
  if (changeStart == true) {
    currentStart++;
  }
  if (currentValue == ARRAY_MAX) {
    changeStart = true;
    currentStart = 1;
    currentValue = 0;
  }
  float temp = read_temperature();
  tempArray[currentValue] = int(-20 * temp + 400);
  if (tempArray[currentValue] > 500) { //max values
    tempArray[currentValue] = 500;
  } else if (tempArray[currentValue] < 0) {
    tempArray[currentValue] = 0;
  }

  Serial.printf("%d %d\n", currentValue, currentStart);

  delay(1000);
  Serial.print("Current Temperature: ");
  Serial.print(temp);
  Serial.print(" Desired Temperature: ");
  Serial.println(desiredTemp);
  if ((temp < desiredTemp) && state % 2 == 0) {
    digitalWrite(relayPin, HIGH);
    Serial.println("Turning on");
  } else {
    digitalWrite(relayPin, LOW);
    Serial.println("Turning  off");
  }
//  WiFiClient client = server.available();
//  //not my code, from https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/server-examples.html
//  // wait for a client (web browser) to connect
//  if (client)
//  {
//    Serial.println("\n[Client connected]");
//    while (client.connected())
//    {
//      // read line by line what the client (web browser) is requesting
//      if (client.available())
//      {
//        String line = client.readStringUntil('\r');
//        Serial.print(line);
//        header += line;
//        if (header.indexOf("GET /h/" + String(refValue)) >= 0) {
//          Serial.println("Switch");
//          onoroff();
//          header = "";
//        }
//        // wait for end of client's request, that is marked with an empty line
//        if (line.length() == 1 && line[0] == '\n')
//        {
//          client.println(prepareHtmlPage());
//          break;
//        }
//      }
//    }
    delay(1000); // give the web browser time to receive the data

    // close the connection:
//    client.stop();
//    Serial.println("[Client disonnected]");
//  }
  //server.handleClient();
  currentValue++;
}


//
//void desired_temperature() {
//  while(digitalRead(interruptPin)) {
//    desiredTemp = 15 + int(analogRead(tempSelector)/100);
//    Serial.print("Desired temperature: ");
//    Serial.println(desiredTemp);
//  }
//}
