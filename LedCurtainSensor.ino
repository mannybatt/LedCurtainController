



/**
* 
* 
*  This is an IR controller designed to integrate a remote control LED curtain
*  with my smart home using MQTT. This code is the combination of both halves
*  and can sense for the power signal and send IR codes accordingly. -Manny Batt
* 
* 
*         LED Curtain IR Codes
* F738C7 On     F7B847 Off       F77887 Mode
* F700FF Red    F7807F Green     F740BF Blue
* F720DF Yellow F7A05F Cyan      F7609F Purple
* F7C03F White  F7E01F Color W.  F708F7 Mode+
* F78877 Mode-  F748B7 Speed+    F7C837 Speed-
* F710EF M1     F7906F M2        F750AF M3
* F7D02F M4     F730CF M5        F7B04F M6
* F7708F M7     F7F00F M8
*
**/


// ***************************************
// ********** Global Variables ***********
// ***************************************


//Globals for Wifi Setup and OTA
#include <credentials.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//WiFi Credentials
#ifndef STASSID
#define STASSID "your_ssid"
#endif
#ifndef STAPSK
#define STAPSK  "your_password"
#endif
const char* ssid = STASSID;
const char* password = STAPSK;

//MQTT
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#ifndef AIO_SERVER
#define AIO_SERVER      "your_MQTT_server_address"
#endif
#ifndef AIO_SERVERPORT
#define AIO_SERVERPORT  0000 //Your MQTT port
#endif
#ifndef AIO_USERNAME
#define AIO_USERNAME    "your_MQTT_username"
#endif
#ifndef AIO_KEY
#define AIO_KEY         "your_MQTT_key"
#endif
#define MQTT_KEEP_ALIVE 150
unsigned long previousTime;

//IR
#include <IRremoteESP8266.h>
#include <IRsend.h>
const uint16_t kIrLed = D7;
IRsend irsend(kIrLed);

//Analog
int sensor = A0;

//MQTT Startup
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish ledCurtain = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/ledcurtainremote");

//Variables
int powerState = 0;
int prevPowerState = 0;
int pendingPowerState = 0;




// ***************************************
// *************** Setup *****************
// ***************************************


void setup() {

  //Initialize IR
  irsend.begin();

  //Initialize Serial
  Serial.begin(115200);
  Serial.println("Booting");

  //WiFi Initialization
  wifiSetup();

  //Initialize MQTT
  MQTT_connect();

  //Check for power status of patronus
  int firstCheck = analogRead(sensor);
  if (firstCheck > 500) {
    powerState = 1;
    prevPowerState = 1;
  }
  Serial.print("Initial Power State: ");
  Serial.println(powerState);
  delay(1000);
}




// ***************************************
// ************* Da Loop *****************
// ***************************************


void loop() {

  //Network Housekeeping
  ArduinoOTA.handle();
  MQTT_connect();

  //Check for power to the Patronus and determine if the state is on or off
  int reading = analogRead(sensor);
  Serial.print("Sensor: ");
  Serial.println(reading);
  if(reading > 500){
    pendingPowerState++;
    if(pendingPowerState > 49){
      pendingPowerState = 50;
      powerState = 1;
    }
  }
  else if(reading < 500){
    pendingPowerState--;
    if(pendingPowerState < 0){
      pendingPowerState = 0;
      powerState = 0;
    }
  }

  //Turn the curtains on and select a random color if patronus is turned on.
  if (powerState == 1 && prevPowerState == 0) {
    Serial.println("************** TURNING ON **************");
    ledCurtain.publish(2);
    delay(500);
    int choice = random(1, 5);
    switch (choice) {
      case 1: ledCurtain.publish(4); //red
        break;
      case 2: ledCurtain.publish(5); //green
        break;
      case 3: ledCurtain.publish(6); //blue
        break;
      case 4: ledCurtain.publish(9); //purple
        break;
      case 5: ledCurtain.publish(8); //cyan
        break;
    }
  }

  //Turn off the curtains if patronus is turned off
  else if (powerState == 0 && prevPowerState == 1) {
    Serial.println("************** TURNING OFF **************");
    ledCurtain.publish(1);
    delay(500);
  }

  //Ping Timer
  unsigned long currentTime = millis();
  if ((currentTime - previousTime) > MQTT_KEEP_ALIVE * 1000) {
    previousTime = currentTime;
    if (! mqtt.ping()) {
      mqtt.disconnect();
    }
  }
  delay(10);
  prevPowerState = powerState;
}




// ***************************************
// ********** Backbone Methods ***********
// ***************************************


void wifiSetup() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname("LedCurtain-Sensor");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    //Serial.println("Connected");
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      //while (1);
      Serial.println("Wait 10 min to reconnect");
      delay(600000);
    }
  }
  Serial.println("MQTT Connected!");
}
