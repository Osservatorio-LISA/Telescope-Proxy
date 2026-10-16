/** @file main.cpp
 * @author Alessandro Maryni
 * @author Mauro Gori
 * 
 * @brief Entry point of firmware
 * 
 * OVERVIEW:
 * This file contains main setup and execution of serial firmware.
 * - initializes the serial module, http_server drivers, and telescope
 * - handle Wifi initialisation
 * - executes main loop
 */


#include "serial_config.h"
#include "Json_key_names.h" 

#include <rom/rtc.h>
#include <esp_system.h>

void Handle_serial_debug_interface();
bool commandsPrinted = false;

/* -------------------------------------------------------------  Server.cpp dependencies --------------------------------------------------------------- */
#ifdef USE_WIFI


#include "WiFi_credentials.h" // define SSID and PWD in this file (in .gitignore)

const char* ssid = SSID;
const char* password = PWD;
String tcp_server_IP     =  TCP_DEFAULT_IP;
uint16_t tcp_server_port = TCP_DEFAULT_PORT;

WebServer  http_server(80);
WiFiClient tcp_client;
#endif //USE_WIFI

//allocate the status variables
uint8_t hexcmd[32];
uint8_t hexresp[32];
AltAz currentRA;
AltAz currentDA;
AltAz targetRA;
AltAz targetDA;
int lowerhorizon;   //minimum height before telescope throws "below horizon" error at :MS# command
/*================================================================== SETUP =========================================================================*/
void setup(){
  Serial.begin(9600);
  unsigned long startUsb = millis();
  //SETUP USB SERIAL FOR DEBUG
  #ifdef DEBUG
  Serial.print(F("initializing serial..."));
  #endif
  while (!Serial && (millis() - startUsb < FOUR_SECONDS_MS)) {
     delay(10);
  }
  #ifdef DEBUG
  Serial.println(F("ok"));
  #endif
  delay(200);
  //SETUP SERIAL2 FOR TLSC CONTROL
  #ifdef DEBUG
  Serial.print(F("initializing Telescope serial..."));
  #endif
  Serial2.begin(9600);
  while(!Serial2);
  #ifdef DEBUG
  Serial.println(F("ok"));
  #endif
  delay(200);
  //SETUP WIFI
#ifdef USE_WIFI
  WiFi.begin(ssid, password);
  uint8_t attempt = 0;
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    attempt++;
    if(attempt > MAX_ATTEMPT){
      Serial.println(F("could not connect to Wifi"));
      break;
    }
  }
  if(WiFi.status() == WL_CONNECTED)
    Serial.println(F("Wifi OK!"));

  http_init(); 
  if(WiFi.status() == WL_CONNECTED)
    #ifdef DEBUG
    Serial.print("IP: ");Serial.println(WiFi.localIP());
    #endif
  #endif//USE_WIFI
  delay(200);
//SETUP LX200
#ifdef DEBUG

#endif
  lx200_setup();
  #ifdef DEBUG
  Serial.println(F("setup successful"));
  #endif
}
/*================================================================== LOOP =========================================================================*/

void loop(){ 

  /* Communications */
#ifdef USE_WIFI
  http_server.handleClient(); //svolge le richieste http in coda
#endif
#ifdef DEBUG
  Handle_serial_debug_interface();
#endif

}
/*================================================================== HELPING FUNCTIONS =========================================================================*/

#ifdef DEBUG
/**
 * @brief this debug function allows to directly access the lx200 functions from the usb serial
 */
void Handle_serial_debug_interface(){
  if (!commandsPrinted) {
    Serial.println(F("\nAvailable commands:"));
    Serial.println(F("d   -   : get declination angle"));
    Serial.println(F("r   -   : get right ascension angle"));
    Serial.println(F("t   -   : set DA"));
    Serial.println(F("p [cmd] : pass [cmd] to lx200, note: cmd MUST end with '#' char"));
    commandsPrinted = true;
  }

  if (Serial.available() > 0){
    char inByte = Serial.read();
    switch(inByte){
      case 'd':{
        AltAz da = lx200_getDA();
        Serial.println("DA: "+ String(da.hours) + ":" + String(da.minutes) + ":" + String(da.seconds));
        break;
      }
      case 'r':{
        AltAz ra = lx200_getRA();
        Serial.println("RAA: "+ String(ra.hours) + ":" + String(ra.minutes) + ":" + String(ra.seconds));
        break;
      }
      case 'p':{
        /* getting [cmd] */
        String cmd = "";
        bool valid_comand = false;
        char c;
        while (Serial.available() > 0) { 
          c = Serial.read(); 
          cmd += c;
          if(c == '#') valid_comand = true;
        }
        /* sending [cmd] and getting response */
        if( valid_comand) {
          cmd = lx200_send_and_get_CMD(cmd.c_str());
          Serial.println("GOT: " + cmd);
        } else {
          Serial.println("[ERROR] cmd MUST end with '#' char ");
        }
        break;
      }
    }

    /* empty the buffer */
    while (Serial.available() > 0) { 
        Serial.read(); 
    }
  }
}
#endif


