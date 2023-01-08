#include <WiFi.h>
#include <WiFiClient.h>

#include <WebServer.h>

#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <BlynkSimpleEsp32.h>

// change ssid and password to your wifi network
const char* ssid = "HCMUS Public";
const char* password = "";

// local web server use port 80, switch to other port if 80 is busy
AsyncWebServer server(80);

const int led = 13;

// Blynk define
#define BLYNK_TEMPLATE_ID "TMPLKEMoERfG"
#define BLYNK_DEVICE_NAME "My ESP32 Smart Bin"
#define BLYNK_AUTH_TOKEN "_WUAhvC6GWdw6EhebnGGmrNCTxvAZ82y"

#define RXp2 3  //16
#define TXp2 1 //17

#define second_default "5"

// Blynk global variable
char auth[] = BLYNK_AUTH_TOKEN;

String RubDistance = "error";
String RubPercent = "error";

//String OpenBin = openbin_default;
String seconds = second_default;
const char html_part1[] PROGMEM= R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Smart rubbish - home page</title>
  <meta http-equiv="refresh" content="2"; url=/">
  <style>
    body {
      background-color: aquamarine;
    }
    .button {
      border: none;
      color: white;
      padding: 16px 32px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
      margin: 4px 2px;
      transition-duration: 0.4s;
      cursor: pointer;
    }
    .button_custom {
      background-color: white;
      color: black;
      border: 2px solid black;
    }
    .button_custom:hover {
      background-color: #4CAF50;
      color: white;
    }
    div {
      text-align: center;
      margin-bottom: 25%;
      margin-right: 25%;
      margin-left: 25%;
    }
  </style>
</head>
<body>
  <div>
    <h1>I'm your smart rubbish bin</h1>
    <h1>Distance to user: )rawliteral";

const char html_part2[] PROGMEM= R"rawliteral( cm</h1>
    <h1>Percent rubbish: )rawliteral";

const char html_part3[] PROGMEM= R"rawliteral( %</h1>
  <a href="/choose-time"><input class="button button_custom" type="submit" value="Manual Open me"></a> )rawliteral";


const char open_part1[] PROGMEM= R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Smart rubbish - choose time</title>
  <style>
    body {
      background-color: aquamarine;
    }
    .button {
      border: none;
      color: white;
      padding: 16px 32px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
      margin: 4px 2px;
      transition-duration: 0.4s;
      cursor: pointer;
    }
    .button_custom {
      background-color: white;
      color: black;
      border: 2px solid black;
    }
    .button_custom:hover {
      background-color: #4CAF50;
      color: white;
    }
    div {
      text-align: center;
      margin-bottom: 25%;
      margin-right: 25%;
      margin-left: 25%;
    }
  </style>
</head>
<body>
  <div>
    <h1>I'm your smart rubbish bin</h1>
    <form action="/open">
      <label for="Second">Time delay after openning:</label>
      <input type="number" id="Second" name="Second" min="1" placeholder="input second number" value=)rawliteral";       
      
const char open_part2[] PROGMEM= R"rawliteral(
      >
      <label for="Second">s</label>
      <input class="button button_custom" type="submit" value="Open">
    </form>
  </div>
</body>
</html>)rawliteral";

String handleRoot() {
  digitalWrite(led, 1);
  String response = html_part1;
  response += RubDistance;
  response +=  html_part2;
  response += RubPercent;
  response +=  html_part3;

  digitalWrite(led, 0);
  return response;
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found - 404 ERROR");
}

String getSensorValue() {
  if (Serial.available()) 
  {
    // Allocate the JSON document
    StaticJsonDocument<300> doc;

    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(doc, Serial);

    if (err == DeserializationError::Ok) 
    {
      // Print the values
      // (we must use as<T>() to resolve the ambiguity)
      RubDistance = String(doc["rubDis"].as<float>());
      RubPercent = String(doc["rubPer"].as<float>());
    } 
    else 
    {
      // Print error to the "debug" serial port
      // Flush all bytes in the "link" serial port buffer
      while (Serial.available() > 0)
        Serial.read();
    }
  }
}

BlynkTimer timer;

// this function use to send infor to blynk
void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V0, RubDistance);
  Blynk.virtualWrite(V1, RubPercent);
}

// this function use to read infor sending from virtual pin V2 of Blynk
BLYNK_WRITE(V2)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V2 to a variable
  if (pinValue == 1){
    Serial.println("5000");
    Blynk.virtualWrite(V2, 0);
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  //Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  
  Serial.print("Blynk connecting...");

  // config blynk and connect
  Blynk.config(auth,"blynk.cloud", 8080);
  Blynk.connect();

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  Serial.println("Web server started!");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String index_html = handleRoot();
    request->send(200, "text/html", index_html);
  });                                                                                                                


  server.on("/choose-time", HTTP_GET, [](AsyncWebServerRequest *request){
      String open_html = open_part1;
      open_html += seconds;
      open_html += open_part2;
      request->send(200, "text/html", open_html);
    });    
  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/open", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam("Second")) {
      seconds = request->getParam("Second")->value();
      String OpenBin = seconds + "000";
      Serial.println(OpenBin);
      request->redirect("/");
    }
  });

  server.onNotFound(handleNotFound);

  server.begin();

  // Setup a function to be called every second
  timer.setInterval(1000L, myTimerEvent);
}

void loop() {
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
  if (Serial.available()){
    getSensorValue();
  }
}