



/**
* 
* 
*  This is an IR controller designed to integrate a remote control LED curtain
*  with my smart home using MQTT. This code is dedicating to transmitting those 
*  IR codes.    -Manny Batt
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

//MQTT Startup
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe ledCurtain = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/ledcurtainremote");

//IR
#include <IRremoteESP8266.h>
#include <IRsend.h>
const uint16_t kIrLed = D2;
IRsend irsend(kIrLed);




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
  mqtt.subscribe(&ledCurtain);
  MQTT_connect();

  delay(1000);
}




// ***************************************
// ************* Da Loop *****************
// ***************************************


void loop() {

  //Network Housekeeping
  ArduinoOTA.handle();
  MQTT_connect();

  //IR State Manager
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(10))) {
    Serial.println("Subscription Recieved");
    uint16_t value = atoi((char *)ledCurtain.lastread);
    Serial.println(value);
    delay(50);

    //Power off
    if (value == 1) {
      irsend.sendNEC(16234567, 64);
      //irsend.sendNEC(0xF7B847, 64);
    }
    //Power on
    if (value == 2) {
      irsend.sendNEC(16201927, 64);
      //irsend.sendNEC(0xF738C7, 64);
    }
    //Mode
    if (value == 3) {
      irsend.sendNEC(16218247, 64);
    }
    //Red
    if (value == 4) {
      irsend.sendNEC(16187647, 64);
    }
    //Green
    if (value == 5) {
      irsend.sendNEC(16220287, 64);
    }
    //Blue
    if (value == 6) {
      irsend.sendNEC(16203967, 64);
    }
    //Yellow
    if (value == 7) {
      irsend.sendNEC(16195807, 64);
    }
    //Cyan
    if (value == 8) {
      irsend.sendNEC(16228447, 64);
    }
    //Purple
    if (value == 9) {
      irsend.sendNEC(16212127, 64);
    }
    //White
    if (value == 10) {
      irsend.sendNEC(16236607, 64);
    }
    //Color Wheel
    if (value == 11) {
      irsend.sendNEC(16244767, 64);
    }
    //Mode +
    if (value == 12) {
      irsend.sendNEC(16189687, 64);
    }
    //Mode -
    if (value == 13) {
      irsend.sendNEC(16222327, 64);
    }
    //Speed +
    if (value == 14) {
      irsend.sendNEC(16206007, 64);
    }
    //Speed -
    if (value == 15) {
      irsend.sendNEC(16238647, 64);
    }
    //M1
    if (value == 16) {
      irsend.sendNEC(16191727, 64);
    }
    //M2
    if (value == 17) {
      irsend.sendNEC(16224367, 64);
    }
    //M3
    if (value == 18) {
      irsend.sendNEC(16208047, 64);
    }
    //M4
    if (value == 19) {
      irsend.sendNEC(16240687, 64);
    }
    //M5
    if (value == 20) {
      irsend.sendNEC(16199887, 64);
    }
    //M6
    if (value == 21) {
      irsend.sendNEC(16232527, 64);
    }
    //M7
    if (value == 22) {
      irsend.sendNEC(16216207, 64);
    }
    //M8
    if (value == 23) {
      irsend.sendNEC(16248847, 64);
    }
    delay(10);
  }
  delay(10);
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
  ArduinoOTA.setHostname("LedCurtains-Blaster");
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
