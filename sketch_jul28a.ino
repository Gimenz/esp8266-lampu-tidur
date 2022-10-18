#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"

const char* ssid = "RISTASOUND HOTSPOT";
const char* password = "";

//Server API JSON untuk weather
String serverLampu = "https://masgimenz.my.id/json/status.json";
String jsonKu;

//atur zona waktu untuk menentukan wilayah jam dan tanggal (utcTimeOffset)
const long utcTimeOffsetInSeconds = 25200;

WiFiUDP ntpUDP;
ESP8266WebServer server(80);

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcTimeOffsetInSeconds);

#define Output_Relay1 2 // Pin D4
#define DHTPin 0 // Pin D3
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);                

float Temperature;
float Humidity;

int ON = LOW, OFF = HIGH;

//Milis
unsigned long lastTime = 0;
unsigned long timerDelay = 500;

//Setup
void setup()
{
    Serial.begin(9600);

    //Memulai konfigurasi WIFI
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());    

    pinMode(Output_Relay1, OUTPUT);
    pinMode(DHTPin, INPUT);
    digitalWrite(Output_Relay1, OFF);
    
    timeClient.begin();
    dht.begin();    

    server.on("/", handle_OnConnect);
    server.onNotFound(handle_NotFound);
  
    server.begin();
    Serial.println("HTTP server started");

  // SSD1306_SWITCHCAPVCC = Aktivasi tegangan 3.3V internal display
  /*if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; 
  }

  // Clear the buffer
  display.clearDisplay();
  delay(2000);*/
}


void loop()
{
    server.handleClient();
    Temperature = dht.readTemperature(); // Gets the values of the temperature
    Humidity = dht.readHumidity(); // Gets the values of the humidity 

    Serial.println("Suhu : " + String(Temperature));
    Serial.println("Kelembapan : " + String(Humidity));
      
    if ((millis() - lastTime) > timerDelay) {
        //Check WiFi status Wifi
        if (WiFi.status() == WL_CONNECTED) {
          // update time client
          timeClient.update();
          int jam = timeClient.getHours();
          String urlJson = "https://masgimenz.my.id/json/api.php?suhu=" + String(Temperature) + "&kelembapan=" + String(Humidity);
          httpGETRequest(urlJson);
          
          const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 80;
          DynamicJsonBuffer jsonBuffer(capacity);
  
          jsonKu = httpGETRequest(serverLampu);
          
          JsonObject& root = jsonBuffer.parseObject(jsonKu);
          String lampu1_status = root["lampu1"]["status"]; // "on"
  
          if (lampu1_status == "on") {
              Serial.println("LAMPU ON " + jam);            
              digitalWrite(Output_Relay1, ON);
          } else if (lampu1_status == "off") {
              Serial.println("LAMPU OFF" + jam);
              digitalWrite(Output_Relay1, OFF);
          } else if (lampu1_status == "auto") {
            Serial.println("LAMPU AUTO - time == ");
            Serial.println(jam);          
            if ((jam >= 18) || (jam <= 6)) {
              Serial.println("MODE AUTO ==> LAMPU ON");            
              digitalWrite(Output_Relay1, ON);
            } else {
              Serial.println("MODE AUTO ==> LAMPU OFF");
              digitalWrite(Output_Relay1, OFF);
            } 
          } else {
              Serial.println("LAMPU ON");            
              digitalWrite(Output_Relay1, ON);
          }
        
        } else {
          Serial.println("WiFi Disconnected");
        }
        lastTime = millis();
    }
}

void handle_OnConnect() {

 Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity 
  server.send(200, "text/html", SendHTML(Temperature,Humidity)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float Temperaturestat,float Humiditystat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>ESP8266 Weather Report</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>ESP8266 NodeMCU Weather Report</h1>\n";
  
  ptr +="<p>Temperature: ";
  ptr +=(int)Temperaturestat;
  ptr +="°C</p>";
  ptr +="<p>Humidity: ";
  ptr +=(int)Humiditystat;
  ptr +="%</p>";
  
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

//Untuk melakukan request ke API JSON
String httpGETRequest(String serverName)
{

  /**
   * Dibuat oleh Coders Indonesia
   * Youtube: https://youtube.com/codersindonesia
   * 
   * */
  WiFiClientSecure httpsClient;
  HTTPClient http;

  httpsClient.setInsecure();
  httpsClient.connect(serverName, 443);

  http.begin(httpsClient, serverName);

  String payload;
  int response = http.GET();
  if (response == HTTP_CODE_OK)
  {
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(response);
  }

  http.end();
  return payload;
}
