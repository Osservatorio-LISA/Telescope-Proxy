#include "serial_config.h"
//IMPLEMENTING LX200 CONTROL FUNCTIONS

/**
 * @brief converts RA AltAz struct into a coordinate string
 * @returns coordinates in String
 * @param AltAz struct of the RA cordinates to convert
 */
String AltAz2RAString(AltAz coord){
    return String(coord.hours) + ":" + String(coord.minutes) + ":" + String(coord.seconds); 
}
/**
 * @brief converts DA AltAz struct into a coordinate string
 * @returns coordinates in a String
 * @param AltAz struct of the DA coordinates to convert
 */
String AltAz2DAString(AltAz coord){
    String scoord = coord.positive ? "+" : "-"; 
    return scoord + String(coord.hours) + "*" + String(coord.minutes) + ":" + String(coord.seconds); 
}
/**
 * @brief this function does the initialization of the telescope
 * @return true if successful, false if failed
 * @details the setup function does: 
a) send an ack bit and wait for the reply with the alignment mode
a1) if its wrong correct it to ALTAZ
b) save the initial RA & DA coords
c) send the :CM# command and wait for the reply string (idk why, stellarium does it)
d) sends the :Gh# command, and then saves the lower bound for the DA
e) returns true if successful
 */
bool lx200_setup() {
//ask telescope for alignment
char ack = LX200_ACK;
    String alignMode = lx200_send_and_get_CMD(ack); //send ack bit with overloaded lx200_getcmd
    if (alignMode.length() == 0) {
        #ifdef DEBUG
        Serial.println("ERROR TELESCOPE UNRESPONSIVE DURING SETUP ALIGNMENT");
        #endif
        #ifdef USE_WIFI
        tcp_send_error_message("telescope unresponsive during setup", TELESCOPE_NOT_RESPONDING);
        #endif
        return false;
    }

// correcting alignment mode if wrong (must be A, AltAz)
    if (alignMode.c_str()[0] != 'A') {
        #ifdef DEBUG
        Serial.print("Wrong alignment, correcting");
        #endif
        lx200_send_and_get_CMD(LX200_FORCE_ALIGNMENT);
        delay(100); // Pausa di assestamento per il firmware
    } 
    #ifdef DEBUG
    else {
        Serial.println("correct alignment");
    }
    #endif

// send calibration signal
    /*String cmResponse = lx200_send_and_get_CMD(LX200_CALIBRATE_MOUNT);

// LX200 replies with the static string(es. "M31 EX GAL MAG 3.5 SZ178.0'")
    if(cmResponse = ""){
        #ifdef DEBUG
        Serial.print("CM FAILED");
        #endif
        #ifdef USE_WIFI
        tcp_send_error_message("calibrate mount failed", PARSING_ERROR);
        #endif
        //return false;
    }else{
        #ifdef DEBUG
        Serial.print("CM COMMAND OK");
        Serial.println(cmResponse);
        #endif
    }*/



//save initial coords into the global variable
    currentRA = lx200_getRA();
    currentDA = lx200_getDA();
	
///TODO: ADD PRECISION CONTROL HERE
    #ifdef DEBUG
    Serial.printf("POSITION READ DURING SETUP -> RA: %02dh %02dm %02ds DA: %c%02d° %02d' %02d\"\n",
                  currentRA.hours, currentRA.minutes, currentRA.seconds,
                  currentDA.positive ? '+' : '-',
                  currentDA.hours, currentDA.minutes, currentDA.seconds);
    #endif


    //get lower horizon limit (:Gh#) -> sDD#
  
    String ghResponse = lx200_send_and_get_CMD(LX200_GET_HORIZON_LIMIT);

    ghResponse.trim();
    if (ghResponse.startsWith(":")) {
        ghResponse.remove(0, 1);
    }

// parsing sDD (es. "+05", "-02", "05")
    bool isNegative = false;
    if (ghResponse.startsWith("-")) {
        isNegative = true;
        ghResponse.remove(0, 1);
    } else if (ghResponse.startsWith("+")) {
        ghResponse.remove(0, 1);
    }

    String numStr = "";
    for (size_t i = 0; i < ghResponse.length(); i++) {
        uint8_t c = (uint8_t)ghResponse.charAt(i);
        if (isdigit(c)) {
            numStr += (char)c;
        } else {
            // when meets ? or '#' stops
            break; 
        }
    }

//assigning value to global variable
    if (numStr.length() > 0) {
        int rawVal = numStr.toInt();
        lowerhorizon = isNegative ? -rawVal : rawVal;
    } else {
        lowerhorizon = 0;
        return false;
    }
//return
    return true;
}

/**
 * @brief queries the telescope for its current RA position
 * @returns the telescope RA position in an AltAz struct
 */
AltAz lx200_getRA(){
    AltAz RA;   //istantiate temporary AltAz struct
    String response = lx200_send_and_get_CMD(LX200_GET_RA);
    
//parse response
  response.trim();  //library function to remove blank spaces

  //remove an eventual initial : symbol, on the LX200 in cassini there isnt (read info.md)
  if (response.startsWith(":")) {
    response.remove(0, 1);
  }

  int firstColon  = response.indexOf(':');
  int secondColon = response.indexOf(':', firstColon + 1);

  //parse here the string into the AltAz Struct
  if (firstColon != -1 && secondColon != -1) {
    RA.hours   = (uint8_t) response.substring(0, firstColon).toInt();
    
    RA.minutes = (uint8_t) response.substring(firstColon + 1, secondColon).toInt();
    
    RA.seconds = (uint8_t) response.substring(secondColon + 1).toInt();
    
    RA.positive = true; // always true for RA
  } else {
    #ifdef DEBUG
    Serial.println("ERROR WHILE PARSING");
    #endif
    #ifdef USE_WIFI
    tcp_send_error_message("error while parsing RA", PARSING_ERROR);
    #endif
  }


  return RA;
}
/**
 * @brief queries the telescope for its current DA position
 * @returns the telescope DA position in an AltAz struct
 */
AltAz lx200_getDA(){
    String response = lx200_send_and_get_CMD(LX200_GET_DA);
    AltAz DA;
    
    response.trim();
//remove an eventual initial :
  if (response.startsWith(":")) {
    response.remove(0, 1);
  }
// manage the + or - sign
    if (response.startsWith("-")) {
        DA.positive = false;
        response.remove(0, 1); // remove the sign for further processing
    } else if (response.startsWith("+")) {
        DA.positive = true;
        response.remove(0, 1);
    } else {
        #ifdef DEBUG
        Serial.print("ERROR PARSING SIGN DA");
        #endif
        #ifdef USE_WIFI
        tcp_send_error_message("error while parsing DA", PARSING_ERROR);
        #endif
        return DA;
    }

    //Since the lx200 used a weird ANSI codification in which the ° symbol appears as � (0xDF in hexadecimal), in UTF-8, this piece of 
    //code is necessary to handle such problem. it also works if there is a * symbol
  int colonIndex = response.indexOf(':'); 
  int degreeIndex = -1;
  for (int i = 0; i < response.length(); i++) {
    uint8_t c = (uint8_t)response.charAt(i);        //casting to uint8 cuz the ? symbol is = to 223
    if (!isDigit(c)) { 
      degreeIndex = i;
      break;
    }
  }

  // numerical parsing
  if (degreeIndex != -1 && colonIndex != -1 && colonIndex > degreeIndex) {

    DA.hours   = (uint8_t) response.substring(0, degreeIndex).toInt();

    DA.minutes = (uint8_t) response.substring(degreeIndex + 1, colonIndex).toInt();
    
    DA.seconds = (uint8_t) response.substring(colonIndex + 1).toInt();
  } else {
    #ifdef DEBUG
    Serial.print("ERROR PARSING DA '");
    Serial.print(response);
    Serial.println("'");
    #endif
    #ifdef USE_WIFI
    tcp_send_error_message("error while parsing DA", PARSING_ERROR);
    #endif
  }
  return DA;
}
/**
 * @brief sends the target RA coordinates to the telescope
 * @returns true if accepted, false if rejected
 * @param AltAz struct of the target coordinates
 */
bool lx200_setRA(AltAz coords){
    bool endswithpound = false;
    char cmd[24] = {0};
    snprintf(cmd, sizeof(cmd), ":Sr%02d:%02d:%02d#",
             (int)coords.hours,
             (int)coords.minutes,
             (int)coords.seconds);
    //String cmd = ":Sr" + String(coords.hours) + ":" + String(coords.minutes) + ":" + String(coords.seconds) + "#";
    #ifdef DEBUG
    Serial.print(cmd);
    #endif
    String response = lx200_send_and_get_CMD(cmd, TELESCOPE_TIMEOUT_MS, endswithpound);    //c_str() converts String type into const char *
    if(!response.startsWith("1")){
        #ifdef DEBUG
        Serial.print("ERROR WITH RESPONSE TO Sr COMMAND");
        #endif
        #ifdef USE_WIFI
        tcp_send_error_message("error in setRA function, rejected by telescope", REJECTED_BY_TELESCOPE);
        #endif
        return false;
    }
    return true;
}
/**
 * @brief sends the target DA coordinates to the telescope
 * @returns true if accepted, false if rejected
 * @param AltAz struct of the target coordinates
 */
bool lx200_setDA(AltAz coords) {
    /*if (coords.hours < lowerhorizon) {
        #ifdef DEBUG
        Serial.println("ERROR SETDA: BELOW HORIZON");
        #endif
        #ifdef USE_WIFI
        tcp_send_error_message("error in setDA function, below horizon", BELOW_HORIZON);
        #endif
        return false;
    }*/

    char sign = coords.positive ? '+' : '-';

    char cmd[24] = {0};
    snprintf(cmd, sizeof(cmd), ":Sd%c%02d%c%02d:%02d#",
             sign,
             (int)coords.hours,
             (char)0xDF, // '°' char
             (int)coords.minutes,
             (int)coords.seconds);

    #ifdef DEBUG
    for (size_t i = 0; cmd[i]; i++) Serial.printf("%02X ", (uint8_t)cmd[i]);
    Serial.println();
    #endif

    String response = lx200_send_and_get_CMD(cmd, TELESCOPE_TIMEOUT_MS, false);
    if (!response.startsWith("1")) {
        #ifdef DEBUG
        Serial.println("ERROR WITH RESPONSE TO Sd COMMAND");
        #endif
        #ifdef USE_WIFI
        tcp_send_error_message("error with the response to Sd command, rejected by telescope", REJECTED_BY_TELESCOPE);
        #endif
        return false;
    }
    return true;
}
/**
 * @brief sends the Move/Slew command to the telescope
 * @returns 0 if accepted, 1 if there's problems
 */
int lx200_go(){
    String response = lx200_send_and_get_CMD(LX200_MOVEMENT_START, TELESCOPE_TIMEOUT_MS, false);
    return (response.c_str()[0] - '0');   //0 is good, 1 below horizon
}
/////////////////////////////////////// UTILITY FUNCTIONS ////////////////////////////////////////////
/**
 * @brief sends a command string and records the answer, checking if its correct
 * @param cmd a string containing the command to send
 * @param timeout how much time should pass before throwing a telescope unresponsive error
 * @param endswithpound always true as default, set to false if the answer doesnt have a # in the end
 * @returns String with the reply from the telescope
 */
String lx200_send_and_get_CMD(const char* cmd, unsigned long timeout, bool endswithpound){

    String response = "";
    unsigned long startMillis;
    bool responseReceived = false;

    /* flush serial buffer */
    while (Serial2.available() > 0) {  
        Serial2.read();
    }
    /* send command */
    Serial2.print(cmd); 

    /* wait for response */
    startMillis = millis();
    if(endswithpound){
        #ifdef DEBUG
		Serial.println("###");
        #endif
        while (millis() - startMillis < timeout) {
            if (Serial2.available() > 0) {
                char c = Serial2.read();
                /* end char of each response */
                #ifdef DEBUG
				Serial.print(c, HEX); //dumps serial into usb Serial
                #endif
                if (c == '#'){
                    responseReceived = true;
                    break;
                }
                response += c;
            }
        }
    }else{
        while (millis() - startMillis < timeout) {
            if(Serial2.available()){
                while(Serial2.available() > 0) {
                    char c = Serial2.read();
                    response += c;
                }
                responseReceived = true;
                break;
            }
        }
    }
    if (!responseReceived){
        #ifdef DEBUG
        Serial.println("ERROR TELESCOPE TIMEOUT");
        #endif //debug
        #ifdef USE_WIFI
        tcp_send_error_message("error  responserecieved = false", TELESCOPE_NOT_RESPONDING);
        #endif
        return "";
    }
    return response;
}
/**
 * @brief overloaded function to send hexadecimal values instead of a string
 * @note endswithpound is set to false since 99% of the cases where you send a single byte the answer is a single byte
 * @param cmd a char value of the HEX byte to send
 * @returns string with the response
 */
String lx200_send_and_get_CMD(char cmd, unsigned long timeout, bool endswithpound){
    char s_cmd[] = {cmd, '\0'}; // "0xchar 0x00"
    return lx200_send_and_get_CMD(s_cmd, timeout, endswithpound);
}