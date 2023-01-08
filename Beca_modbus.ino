#include <SoftwareSerial.h>       // Leave the main serial line (USB) for debugging and flashing
#include <ModbusMaster.h>         // Modbus master library for ESP8266
#include <ESP8266WiFi.h>          // Wifi connection
#include <ESP8266WebServer.h>     // Web server for general HTTP response
#include <ESP8266mDNS.h>          // mDNS
#include <PubSubClient.h>         // MQTT support
#include <WiFiUdp.h>                // Added for NTP functionality
#include <Time.h>
#include <TimeLib.h>
#include <sunMoon.h>                // Sunrise, sunset calculation
#include "NTP.h"

#define MODBUS_RATE     9600      // Modbus speed of Beca, do not change
#define SERIAL_RATE     115200    // Serial speed for status info
#define MAX485_DE       5         // D1, DE pin on the TTL to RS485 converter
#define MAX485_RE_NEG   4         // D2, RE pin on the TTL to RS485 converter
#define MAX485_RX       14        // D5, RO pin on the TTL to RS485 converter
#define MAX485_TX       12        // D6, DI pin on the TTL to RS485 converter
#define SLAVE_ID        1         // Default slave ID of Beca
#define STATUS_LED      2         // Status LED on the Wemos D1 mini (D4)
#define UPDATE_MODBUS   1         // 1: modbus device is read every second
#define UPDATE_STATUS   10        // 10: status mqtt message is sent every 10 seconds

// Update the below parameters for your project
// Also check NTP.h for some parameters as well
const char* ssid = "xxxxxxxxxxxxx";           // Wifi SSID
const char* password = "xxxxxxxxxxxxxxxx";    // Wifi password
const char* mqtt_server = "192.168.x.xx";     // MQTT server
const char* mqtt_user = "xxxxxx";             // MQTT userid
const char* mqtt_password = "xxxxxx";         // MQTT password
const char* clientID = "Beca";                // MQTT client ID
const char* topicRoot = "/beca/";             // MQTT root topic for the device, keep / at the end

String NTPtime = "--:--";
char msg[50];
String mqttStat = "";
String message = "";
unsigned long lastTick, uptime, lastNTP, seconds;

os_timer_t myTimer;
MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(mqtt_server, 1883, 0, espClient);
SoftwareSerial modbus(MAX485_RX, MAX485_TX, false, 256); //RX, TX
ModbusMaster beca;

void preTransmission() {
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission() {
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

void sendModbusError(uint8_t result) {
  String message = "";
  if (result==beca.ku8MBIllegalFunction) {
    message = "Illegal function";
  }
  if (result==beca.ku8MBIllegalDataAddress) {
    message = "Illegal data address";
  }
  if (result==beca.ku8MBIllegalDataValue) {
    message = "Illegal data value";
  }
  if (result==beca.ku8MBSlaveDeviceFailure) {
    message = "Slave device failure";
  }
  if (result==beca.ku8MBInvalidSlaveID) {
    message = "Invalid slave ID";
  }
  if (result==beca.ku8MBInvalidFunction) {
    message = "Invalid function";
  }
  if (result==beca.ku8MBResponseTimedOut) {
    message = "Response timed out";
  }
  if (result==beca.ku8MBInvalidCRC) {
    message = "Invalid CRC";
  }
  if (message=="") {
    message = result;
  }
  Serial.println(message);    
  char topic[80];
  char value[30];
  sprintf(topic,"%serror",topicRoot);
  mqtt.publish(topic, message.c_str());
  delay(5);
}

// This is the 1 second timer callback function
void timerCallback(void *pArg) {
  seconds++;
  // Query the modbus device 
  if (seconds % UPDATE_MODBUS==0) {
    uint8_t result;
    //uint16_t data[6];

    digitalWrite(STATUS_LED, 0);
    ESP.wdtDisable();
    result = beca.readHoldingRegisters(0,10);
    ESP.wdtEnable(1);
    if (result == beca.ku8MBSuccess)   {
      char topic[80];
      char value[10];
      // Power
      sprintf(topic,"%sread/power",topicRoot);
      sprintf(value,"%d",beca.getResponseBuffer(0x00));
      //itoa(beca.getResponseBuffer(0x00),value,10);
      mqtt.publish(topic, value);
      Serial.print(F("Power: "));
      Serial.print(value);
      //yield();
      // Fan
      sprintf(topic,"%sread/fan",topicRoot);
      sprintf(value,"%d",beca.getResponseBuffer(0x01));
      //itoa(beca.getResponseBuffer(0x01),value,10);
      mqtt.publish(topic, value);
      Serial.print(F(", Fan: "));
      Serial.print(value);
      //yield();
      // Mode
      sprintf(topic,"%sread/mode",topicRoot);
      //sprintf(value,"%d",beca.getResponseBuffer(0x02));
      itoa(beca.getResponseBuffer(0x02),value,10);
      mqtt.publish(topic, value);
      Serial.print(F(", Mode: "));
      Serial.print(value);
      //yield();
      // Target temperature
      sprintf(topic,"%sread/target",topicRoot);
      sprintf(value,"%.1f",beca.getResponseBuffer(0x03)/10.0f);
      mqtt.publish(topic, value);
      Serial.print(F(", Target: "));
      Serial.print(value);
      //yield();
      // Ambient
      sprintf(topic,"%sread/ambient",topicRoot);
      sprintf(value,"%.1f",beca.getResponseBuffer(0x08)/10.0f);
      mqtt.publish(topic, value);
      Serial.print(F(", Ambient: "));
      Serial.print(value);
      //yield();
      // Lock
      sprintf(topic,"%sread/lock",topicRoot);
      sprintf(value,"%d",beca.getResponseBuffer(0x04));
      mqtt.publish(topic, value);
      Serial.print(F(", Lock: "));
      Serial.print(value);
      //yield();
      // Valve
      sprintf(topic,"%sread/valve",topicRoot);
      sprintf(value,"%d",beca.getResponseBuffer(0x09));
      mqtt.publish(topic, value);
      Serial.print(F(", Valve: "));
      Serial.print(value);
      //yield();
      // Weekday
      sprintf(topic,"%sread/weekday",topicRoot);
      sprintf(value,"%d",beca.getResponseBuffer(0x07));
      mqtt.publish(topic, value);
      Serial.print(F(", Weekday: "));
      Serial.print(value);
      //yield();
      // Time
      sprintf(topic,"%sread/time",topicRoot);
      sprintf(value,"%02d:%02d",beca.getResponseBuffer(0x06),beca.getResponseBuffer(0x05));
      mqtt.publish(topic, value);
      Serial.print(F(", Time: "));
      Serial.println(value);
    } else {
      Serial.print(F("Error: "));
      sendModbusError(result);
    }
    digitalWrite(STATUS_LED, 1);

  }
  // Send RSSI and uptime status
  if (seconds % UPDATE_STATUS==0) {
    // Send MQTT update
    if (mqtt_server!="") {
      char topic[80];
      char value[10];
      sprintf(topic,"%s%s",topicRoot,"rssi");
      itoa(WiFi.RSSI(),value,10);
      mqtt.publish(topic, value);
      sprintf(topic,"%s%s",topicRoot,"uptime");
      itoa(uptime,value,10);
      mqtt.publish(topic, value);
      Serial.println(F("MQTT status sent"));
    }
  }
}

// MQTT reconnect logic
void reconnect() {
  //String mytopic;
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(clientID, mqtt_user, mqtt_password)) {
      Serial.println(F("connected"));
      // ... and resubscribe
      char topic[80];
      sprintf(topic,"%swrite/#",topicRoot);
      mqtt.subscribe(topic);
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(mqtt.state());
      Serial.println(F(" try again in 5 seconds"));
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void setup() {

  Serial.begin(SERIAL_RATE);
  modbus.begin(MODBUS_RATE);
  Serial.println(F("\nBeca Modbus to MQTT Gateway"));
  // Init outputs, RS485 in receive mode
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  // Initialize some variables
  uptime = 0;
  seconds = 0;

  // Connect to Wifi
  Serial.print(F("Connecting to Wifi"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
    seconds++;
    if (seconds>180) {
      // reboot the ESP if cannot connect to wifi
      ESP.restart();
    }
  }
  seconds = 0;
  Serial.println("");
  Serial.println(F("Connected to wifi network"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("Signal [RSSI]: "));
  Serial.println(WiFi.RSSI());

  // Set up the Modbus line
  beca.begin(SLAVE_ID , modbus);
  // Callbacks allow us to configure the RS485 transceiver correctly
  beca.preTransmission(preTransmission);
  beca.postTransmission(postTransmission);
  Serial.println("Modbus connection is set up");

  // Create the 1 second timer interrupt
  os_timer_setfn(&myTimer, timerCallback, NULL);
  os_timer_arm(&myTimer, 1000, true);

  // Set up the MDNS and the HTTP server
  if (mdns.begin("Beca", WiFi.localIP())) {
    Serial.println(F("MDNS responder started"));
  }  
  server.on("/", [](){                        // Dummy page
    server.send(200, "text/plain", "Beca Modbus to MQTT Gateway");
  });
  server.begin();
  Serial.println(F("HTTP server started"));

  // Set up the MQTT server connection
  if (mqtt_server!="") {
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setCallback(callback);
  }

  // Start UDP for NTP function
  Serial.println(F("Starting UDP"));
  udp.begin(localPort);
  Serial.print(F("Local port: "));
  Serial.println(udp.localPort());
  requestNTPUpdate();
  // Initialize sunrise and sunset calculator
  sm.init(GMTOffset, sm_latitude, sm_longtitude);  
  
}



void handleNTPResponse() {
  // Check for NTP response
  int cb = udp.parsePacket();
  if (cb!=0) {
    NTPUpdateMillis = millis();
    NTPRequested = false;
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    epoch = secsSince1900 - seventyYears;
    if (summerTime(epoch)) {
      epoch += 3600; // add 1 hour DST
    }
    epoch += GMTOffset * 60;
    Serial.print(F("NTP Update (epoch): "));
    Serial.println(epoch);
    Serial.print(F("NTP Update (time): "));
    printDate(epoch);  Serial.println("");

    // Send the updates to the modbus device
    uint8_t result;
    digitalWrite(STATUS_LED, 0);
    ESP.wdtDisable();
    result = beca.writeSingleRegister(5,getMinute(epoch));
    ESP.wdtEnable(1);
    if (result!=beca.ku8MBSuccess) {
      Serial.print(F("Modbus minute update failed: "));
      sendModbusError(result);
    }
    delay(5); // Added this delay as sometimes the not all registers got updated on the device
    ESP.wdtDisable();
    result = beca.writeSingleRegister(6,getHour(epoch));
    ESP.wdtEnable(1);
    if (result!=beca.ku8MBSuccess) {
      Serial.print(F("Modbus hour update failed: "));
      sendModbusError(result);
    }
    delay(5);
    ESP.wdtDisable();
    result = beca.writeSingleRegister(7,calcDayOfWeek(epoch));
    ESP.wdtEnable(1);
    if (result!=beca.ku8MBSuccess) {
      Serial.print(F("Modbus weekday update failed: "));
      sendModbusError(result);
    }
    digitalWrite(STATUS_LED, 1);

/*
    // Sunrise and Sunset calculations
    time_t sRise = sm.sunRise(epoch);
    time_t sSet  = sm.sunSet(epoch);
    if (summerTime(epoch)) {
      sRise += 3600; // add 1 hour DST
      sSet += 3600; // add 1 hour DST
    }
    renderTime();
    Serial.print(F("Today sunrise and sunset: "));
    printDate(sRise); Serial.print(F("; "));
    printDate(sSet);  Serial.println("");
*/
  }  
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Convert the incoming byte array to a string
  String mytopic = (char*)topic;
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;

  Serial.print(F("Message arrived on topic: ["));
  Serial.print(topic);
  Serial.print(F("], "));
  Serial.println(message);


  char checktopic[80];
  uint8_t result;
  // Update the power register
  if (mytopic==(String)topicRoot+"write/power") {
     digitalWrite(STATUS_LED, 0);
     ESP.wdtDisable();
     result = beca.writeSingleRegister(0,atoi((char*)payload));
     ESP.wdtEnable(1);
     if (result == beca.ku8MBSuccess)   {
        Serial.println("Power updated");
     } else {
        Serial.print("Power update failed: ");
        sendModbusError(result);
     }
     digitalWrite(STATUS_LED, 1);
  }

  // Update the fan register
  if (mytopic==(String)topicRoot+"write/fan") {
     digitalWrite(STATUS_LED, 0);
     ESP.wdtDisable();
     result = beca.writeSingleRegister(1,atoi((char*)payload));
     ESP.wdtEnable(1);
     if (result == beca.ku8MBSuccess)   {
        Serial.println("Fan updated");
     } else {
        Serial.print("Fan update failed: ");
        sendModbusError(result);
     }
     digitalWrite(STATUS_LED, 1);
  }

  // Update the mode register
  if (mytopic==(String)topicRoot+"write/mode") {
     digitalWrite(STATUS_LED, 0);
     ESP.wdtDisable();
     result = beca.writeSingleRegister(2,atoi((char*)payload));
     ESP.wdtEnable(1);
     if (result == beca.ku8MBSuccess)   {
        Serial.println("Mode updated");
     } else {
        Serial.print("Mode update failed: ");
        sendModbusError(result);
     }
     digitalWrite(STATUS_LED, 1);
  }

  // Update the lock register
  if (mytopic==(String)topicRoot+"write/lock") {
     digitalWrite(STATUS_LED, 0);
     ESP.wdtDisable();
     result = beca.writeSingleRegister(4,atoi((char*)payload));
     ESP.wdtEnable(1);
     if (result == beca.ku8MBSuccess)   {
        Serial.println("Lock updated");
     } else {
        Serial.print("Lock update failed: ");
        sendModbusError(result);
     }
     digitalWrite(STATUS_LED, 1);
  }

  // Update the target temperature register
  if (mytopic==(String)topicRoot+"write/target") {
     digitalWrite(STATUS_LED, 0);
     ESP.wdtDisable();
     result = beca.writeSingleRegister(3,atof((char*)payload)*10);
     ESP.wdtEnable(1);
     if (result == beca.ku8MBSuccess)   {
        Serial.println("Target temperature updated");
     } else {
        Serial.print("Target temperature update failed: ");
        sendModbusError(result);
     }
     digitalWrite(STATUS_LED, 1);
  }
}

void loop() {
  // Handle HTTP server requests
  server.handleClient();

  // Handle MQTT connection/reconnection
  if (mqtt_server!="") {
    if (!mqtt.connected()) {
      reconnect();
    }
    mqtt.loop();
  }

  // Uptime calculation
  if (millis() - lastTick >= 60000) {            
    lastTick = millis();            
    uptime++;            
  }    


  // Check if NTP update is due
  if ((millis() - NTPUpdateMillis >= 60*60*1000) && (!NTPRequested)) {  
    requestNTPUpdate();
  }    
  // Handle NTP response
  handleNTPResponse();

}
