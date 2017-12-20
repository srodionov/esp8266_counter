#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

#define           THING_TYPE        "counter"       // Pulse counter
#define           PIN_LED           5
#define           PIN_BUTTON        0
#define           PIN_COUNTER       4

const char        hostname[]      = "esp8266";
const byte        DNS_PORT        = 53;

char              ap_ssid[16]     = {0};
IPAddress         apIP(192, 168, 1, 1);
IPAddress         netMsk(255, 255, 255, 0);

char              wifi_ssid[16]   = {0};
char              wifi_pass[16]   = {0};
unsigned long     wifi_lastCon    = 0;

char              mqtt_server[32] = {0};
int               mqtt_port       = 1883;
boolean           mqtt_secure     = false;
char              mqtt_user[16]   = {0};
char              mqtt_pass[16]   = {0};
char              mqtt_topic[32]  = {0};
int               mqtt_syncFreq   = 30;             // how often we have to send MQTT message
char              buf[12];

int               state           = 0;
int               prevState       = 0;
boolean           firstPass       = true;
int               counterValue    = 0;

DNSServer         dnsServer;
ESP8266WebServer  server(80);
WiFiClient        espClient;
PubSubClient      mqtt_client(espClient);
  
void setup() {      
  Serial.begin(115200);
  delay(100);
  Serial.println("Setup...");
 
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);  
  pinMode(PIN_COUNTER, INPUT);  
 
  sprintf(ap_ssid, "IoT-%06X", ESP.getChipId());
  loadCredentials();
  if (strlen(wifi_ssid) == 0){
    Serial.println("Start AP mode");
    WiFi.softAPConfig(apIP, apIP, netMsk);
    
    WiFi.softAP(ap_ssid);  
    delay(500);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
       
    server.on("/", handleRoot);
    server.on("/wifisave", handleWifiSave);
    server.onNotFound ( handleNotFound );

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    Serial.println("DNS Server started");
  }else{
    Serial.println("Start STA mode");
    WiFi.mode(WIFI_STA); 
    WiFi.begin (wifi_ssid, wifi_pass);
  
    server.on("/", handleRoot);
    mqtt_client.setServer(mqtt_server, mqtt_port);

    sprintf(mqtt_topic, "%s%c%06X", THING_TYPE, '/', ESP.getChipId());
    //Serial.println(mqtt_topic);
    if (strlen(mqtt_user) != 0) { 
      strcat(mqtt_topic, mqtt_user);
      strcat(mqtt_topic, "/");    
    }
  }
  server.begin();
  Serial.println("HTTP Server started");
}

void loop() {
 
  if (strlen(wifi_ssid) == 0)
  {
    dnsServer.processNextRequest();
  }
  else
  {    
    if (WiFi.status() != WL_CONNECTED){
      digitalWrite(PIN_LED, HIGH);
      
      if (millis() < wifi_lastCon) wifi_lastCon = millis();
      if (millis() > (wifi_lastCon + 60000) || (wifi_lastCon == 0)) {    //reconnect WiFi 
        //Serial.println("Reconnect Wi-Fi");
        WiFi.disconnect();
        delay(250);
        WiFi.begin (wifi_ssid, wifi_pass);
        wifi_lastCon = millis();
      }
    } else {
      //Serial.println("Wi-Fi connected");
      if (!mqtt_client.connected()) { 
        digitalWrite(PIN_LED, HIGH);
        mqtt_client.connect(ap_ssid);
        //Serial.println("Reconnect MQTT");
      } 
    }

    if (mqtt_client.connected())
    {  
        //Serial.println("MQTT connected");
        if (counterValue >= mqtt_syncFreq) {
          Serial.println("send MQTT message");
          sprintf(buf, "%d", counterValue);          
          mqtt_client.publish(mqtt_topic, buf);
          counterValue = 0;
          mqtt_client.loop();
        }
    }
  
    state = digitalRead(PIN_COUNTER);
    delay(5);
    digitalWrite(PIN_LED, state);                   // set the same state as COUNTER PIN
    if (state != prevState) 
    {
      prevState = state;              
      if (firstPass == true)
      {
        firstPass = false;
      } 
      else 
      {
        firstPass = true;
        counterValue = counterValue + 1;
        //Serial.print("pulse: ");
        //Serial.println(counterValue);
      }
    }   
  }

  //HTTP
  server.handleClient();
 
  resetButton();
}

