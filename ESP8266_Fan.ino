#include <FS.h>
#include <DNSServer.h>
#include <NTPClient.h>
#include <Timezone.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "src/ESP8266_Fan.h"
#include "src/html.h"

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 60000);

Timezone myTZ(myDST, mySTD);

TimeChangeRule *tcr;

// Création des objets / create Objects
ESP8266WebServer server ( 80 );

// Handle request for jsonapi document ("/fan.json")
void apiHandler() {
  server.send ( 200, "application/json", jsonApiHandler() );
}

String jsonApiHandler() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  JsonObject& fanstate = json.createNestedObject("fanstate");
  fanstate["curLocal_time"] = currentTime("local");
  fanstate["curUTC_time"] = currentTime("utc");
  fanstate["autooff_time"] = autooff_time;

  String output;
  json.printTo(output);
  return output;
}

String indexPage() {
  if ( server.hasArg("timer") ) {
    checkTimer(server.arg("timer"));
  }
  return main_page;
}

void checkTimer(String timer) {
  if ( timer == "1" ) {
    Serial.println("AutoOff in 1 Hour");
    autooff_time = currentTime("local") + 3600;
  } else if ( timer == "2" ) {
    Serial.println("AutoOff in 2 Hours");
    autooff_time = currentTime("local") + 3600 * 2;
  } else if ( timer == "3" ) {
    Serial.println("AutoOff in 4 Hours");
    autooff_time = currentTime("local") + 3600 * 4;
  } else {
    Serial.println("AutoOff at specific time");
    //2018/07/14 23:00
    //2018-07-14T22:27
    unsigned long requested_time = timer.toInt();
    autooff_time = myTZ.toLocal(requested_time, &tcr);
  }
}

void handleAutoOff() {
  if (autooff_time) {
    if (autooff_time >= currentTime("local")) {
      if (autooff_time == currentTime("local") && triggered == false) {
        handlePower();
        triggered = true;
      }
    } else {
      triggered = false;
    }
  }
}

void handleRoot() {
  if ( server.hasArg("power") ) {
    handlePower();
  } else if ( server.hasArg("breeze") ) {
    handleBreeze();
  } else if ( server.hasArg("speed") ) {
    handleSpeed();
  } else if ( server.hasArg("swing") ) {
    handleSwing();
  } else {
    server.send ( 200, "text/html", indexPage() );
  }
}

void handlePower() {
  updateGPIO(0, "1");
  delay(200);
  updateGPIO(0, "0");
}

void handleBreeze() {
  updateGPIO(1, "1");
  delay(200);
  updateGPIO(1, "0");
}

void handleSpeed() {
  if ( server.arg("speed") == "1" ) {
    handlePower();
    delay(200);
    updateGPIO(2, "1");
    delay(200);
    updateGPIO(2, "0");
  } else if ( server.arg("speed") == "2" ) {
    handlePower();
    delay(200);
    updateGPIO(2, "1");
    delay(200);
    updateGPIO(2, "0");
    delay(200);
    updateGPIO(2, "1");
    delay(200);
    updateGPIO(2, "0");
  } else if ( server.arg("speed") == "3" ) {
    handlePower();
    delay(200);
    updateGPIO(2, "1");
    delay(200);
    updateGPIO(2, "0");
    delay(200);
    updateGPIO(2, "1");
    delay(200);
    updateGPIO(2, "0");
    delay(200);
    updateGPIO(2, "1");
    delay(200);
    updateGPIO(2, "0");
  }
}

void handleSwing() {
  updateGPIO(3, "1");
  delay(200);
  updateGPIO(3, "0");
}

long currentTime(String type) {
  if (type == "local") {
    time_t utc = timeClient.getEpochTime();
    time_t local = myTZ.toLocal(utc, &tcr);
    return local;
  } else if (type == "utc") {
    time_t utc = timeClient.getEpochTime();
    return utc;
  }
}

// format and print a time_t value, with a time zone appended.
void dateTime(time_t t, const char *tz)
{
  char buf[32];
  char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
  strcpy(m, monthShortStr(month(t)));
  sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
          hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz);
  Serial.println(buf);
}

void updateGPIO(int gpio, String DxValue) {
  Serial.println("");
  Serial.println("Update GPIO "); Serial.print(GPIOPIN[gpio]); Serial.print(" -> "); Serial.println(DxValue);

  if ( DxValue == "1" ) {
    digitalWrite(GPIOPIN[gpio], HIGH);
    server.send ( 200, "text/html", indexPage() );
  } else if ( DxValue == "0" ) {
    digitalWrite(GPIOPIN[gpio], LOW);
    server.send ( 200, "text/html", indexPage() );
  } else {
    Serial.println("Err Led Value");
  }
}

void setup() {
  for ( int x = 0 ; x < 5 ; x++ ) {
    pinMode(GPIOPIN[x], OUTPUT);
  }
  Serial.begin ( 115200 );

  WiFi.hostname( host );
  WiFiManager wifiManager;

  wifiManager.setBreakAfterConfig(true);
  if (!wifiManager.autoConnect("ESP_Fan")) {
    Serial.println("failed to connect, we will fire up config mode");
    delay(3000);
    wifiManager.startConfigPortal("ESP_Fan");
    delay(5000);
  }

  Serial.print ( "IP address: " ); Serial.println ( WiFi.localIP() );

  /*return index page which is stored in serverIndex */
  server.on ( "/", handleRoot );
  server.on ( "/fan.json", apiHandler );
  server.begin();
  Serial.println ( "HTTP server started" );
  timeClient.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  timeClient.update();
  handleAutoOff();
}

