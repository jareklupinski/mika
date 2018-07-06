#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <PCD8544.h>
#include <FS.h>

#define SCLK_PIN 14
#define SCE_PIN  12
#define MOSI_PIN 13
#define DC_PIN   5
#define RST_PIN  4
#define LED_PIN  16
#define BTN_PIN  0

const char *ssid = "MIKA-01";
const char *password = "randomPass";

PCD8544 lcd( SCLK_PIN, MOSI_PIN, DC_PIN, RST_PIN, SCE_PIN );
ESP8266WebServer httpServer( 80 );
IPAddress apIP( 192, 168, 1, 1 );

String responseHTML = String( "" ) +
                      String( "<!DOCTYPE html><html><head><title>MIKA Portal</title></head><body>" ) +
                      String( "<h1>My Internet Konnected Appliance Prototype</h1>" ) +
                      String( "</body></html>" );

String updateFailedResponse = "Update Failed!";
String updateSuccessResponse = "<META http-equiv=\"refresh\" content=\"15;URL=\">Update Success! Rebooting...";

void getParams() {
  lcd.print( "Got parameters" );
  lcd.print( httpServer.arg( "speed" ) );
  httpServer.send( 200, "text/plain", "Thanks!" );
}

void updateRespond() {
  httpServer.send(200, "text/html", Update.hasError() ? updateFailedResponse : updateSuccessResponse);
  ESP.restart();
}

void handleUpload() {
  // handler for the file upload, get's the sketch bytes, and writes
  // them through the Update object
  HTTPUpload& upload = httpServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    WiFiUDP::stopAll();
    lcd.print( String( "Update: " ) + String( upload.filename ));
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) lcd.print( "New BIN Too Large!" );
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    lcd.print( "." );
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) lcd.print( "New BIN Wrong Size!" );
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true))lcd.print( "rebooting" );
    else lcd.print( "New BIN Wrong Format!" );
  }
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    lcd.print( "Upload Abort!" );
  }
  yield();
}

void setup() {
  WiFi.mode( WIFI_OFF );
  lcd.begin( 84, 48 );
  lcd.setCursor( 0, 0 );
  lcd.print( "MIKA Prototype" );
  for ( int i = 0; i < 5000 ; i++ ) {
    lcd.setCursor( 0, 1 );
    lcd.print( "Press Update " + String(( 5000 - i ) / 1000 ));
    delay( 1 );
    if ( !digitalRead( BTN_PIN ) ) {
      WiFi.mode( WIFI_AP );
      WiFi.softAPConfig( apIP, apIP, IPAddress( 255, 255, 255, 0 ) );
      WiFi.softAP( ssid, password );
      IPAddress myIP = WiFi.softAPIP();
      SPIFFS.begin();
      httpServer.serveStatic( "/", SPIFFS, "/index.html" );
      httpServer.on( "/settings", HTTP_POST, getParams );
      httpServer.on( "/update", HTTP_POST, updateRespond, handleUpload );
      httpServer.begin();
      lcd.setCursor( 0, 0 );
      lcd.print( "On your smartphone" );
      lcd.print( "connect to the wifi" );
      lcd.print( String( ssid ));
      lcd.print( String( password ));
      String ipString = String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
      lcd.print( ipString );
      while ( 1 ) {
        httpServer.handleClient();
        yield();
      }
    }
  }
  pinMode( LED_PIN, OUTPUT );
  pinMode( BTN_PIN, INPUT_PULLUP );
  digitalWrite( LED_PIN, LOW );
  lcd.print( "Setup Complete" );
}

void loop() {
  lcd.print( "Program 3 Loop~" );
  delay( 100 );
}
