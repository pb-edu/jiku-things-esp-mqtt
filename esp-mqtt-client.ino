#include "EspMQTTClient.h"
#include <ArduinoJson.h>

#define MAX_LEN 255   // make sure it is sufficient for your data
#define WIFI_READY 201

bool led = LOW;
char incomingSerial[MAX_LEN];
uint8_t count = 0;
bool configured = false;
unsigned int reconnectTimer = 1;

StaticJsonDocument<200> configurationData;  

EspMQTTClient mqttClient;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  initializeMqttClient();
}

void initializeMqttClient() {
  Serial.print("\n"); //Make shure message is not prepended by some ESP internal messages
  Serial.println("INIT_ESP");
  while(!configured) {
    while(!readSerialMessage()) {
      if(millis() > 10000 * reconnectTimer) {
        Serial.println("INIT_ESP");
        reconnectTimer++;
      }
      //waiting for configuration via serial
      
    }     
    deserializeIncomingConfiguration();
    
    mqttClient.setWifiCredentials(configurationData["ssid"], configurationData["password"]);
    mqttClient.setMqttClientName(configurationData["client"]);
    mqttClient.setMqttServer(configurationData["broker"], "", "", configurationData["port"]);

    configured = true;    
  }
}

void loop() {
  mqttClient.loop(); // mandatory for mqtt client!
  if(readSerialMessage()) {
    mqttClient.publish("sensor", incomingSerial);
  }    
}

void deserializeIncomingConfiguration() {
  if(incomingSerial[0] != '{') {
    return;
  }
  DeserializationError err = deserializeJson(configurationData, incomingSerial);
  if (err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.print(err.f_str());
    return;
  }
}

void onConnectionEstablished() {
  Serial.println("");
  Serial.println("WIFI_READY");
  digitalWrite(LED_BUILTIN, HIGH);
  
  mqttClient.subscribe("act", [] (const String &payload)  {
    Serial.println(payload);
  });
}

bool readSerialMessage() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      incomingSerial[count] = '\0';         // terminating null byte
      count = 0;
      return true;
    }
    else {
      if (count < (MAX_LEN - 1)) {
        incomingSerial[count++] = c;
      }
    }
  }
  return false;
}
