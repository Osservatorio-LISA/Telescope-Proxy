/** @file serial_config.h
 * @author Mauro Gori
 * 
 * @brief hosts all data types, function definition and pin config
*/
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <Arduino.h>

#undef USE_WIFI
#define USE_WIFI


#undef DEBUG
#define DEBUG


/**
 * @brief holds astronomic coordinates written as +/-DD:MM:SS in three different uint8_t. contains a constructor that sets everything to 0
 * @struct AltAz
 */
typedef struct altaz_t {				//Struct representing the AltAz telescope coordinates
	bool positive;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	altaz_t(){	//costructor
		positive = true;
		hours = minutes = seconds = 0;
	}
}AltAz;


//allocate the global status variables
extern AltAz currentRA;
extern AltAz currentDA;
extern AltAz targetRA;
extern AltAz targetDA;
extern int lowerhorizon;
////////////////////////////////// DEFINES ///////////////////////////////////////////////////

// timing defines
#define TELESCOPE_TIMEOUT_MS 10000
#define TWO_SECONDS_MS  2000
#define FOUR_SECONDS_MS 4000
#define THIRTY_SECONDS_MS 30000
#define FIVE_MINUTES_MS 300000

//lx200 commands defines
#define LX200_GET_RA ":GR#"
#define LX200_GET_DA ":GD#"
#define LX200_GET_HORIZON_LIMIT ":Gh#"

#define LX200_MOVEMENT_START ":MA#"
#define LX200_MOVEMENT_STOP ":Q#"

#define LX200_TOGGLE_PRECISION ":U#"
#define LX200_CALIBRATE_MOUNT ":CM#"
#define LX200_FORCE_ALIGNMENT ":AA#"
#define LX200_RESET	":I#"
#define LX200_DEGCHAR 0xDF
#define LX200_ACK 0x06

//////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////// UTILITY ////////////////////////////////////////////////////
String AltAz2RAString(AltAz coord);
String AltAz2DAString(AltAz coord);
//////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////// LX200 ///////////////////////////////////////////////////
//if returns "" has failed, unless its a responseless command, ewp is to determine wheter the response string ends with # or not
String lx200_send_and_get_CMD(const char* cmd, unsigned long timeout = TELESCOPE_TIMEOUT_MS, bool endswithpound = true);

// if we are sending a single char it will certantly not end with '#'
String lx200_send_and_get_CMD(char cmd, unsigned long timeout = TELESCOPE_TIMEOUT_MS, bool endswithpound = false);

AltAz lx200_getRA();				//asks telescope for RA
AltAz lx200_getDA();
bool lx200_setup();					//does all the preliminary commands such as CM, alignment and saves into memory the boundss
bool lx200_setDA(AltAz coords);		//check with lowerhorizon and set target DA coords
bool lx200_setRA(AltAz coords);		//set target RA coords
int lx200_go();						//start slewing with the set coordinates, returns 0 if good, 1 if Below Horizon, 2 if above

///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// HTTP & TCP //////////////////////////////////////////////////
#ifdef USE_WIFI
/** NOTE: for better info on how to use http & tcp functions read start of server.cpp */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <lwip/sockets.h> //used to parse IPv4 address

/*---------------------------------- Http status codes --------------------------------------  */
#define HTTP_STATUS_OK 		    200 //request is ok and response is in body
#define HTTP_STATUS_ACCEPTED    202 //request is accepted but not processed yet
#define HTTP_STATUS_OK_NO_BODY  204 //request is ok but there is no http body <- our case, response is TCP
#define HTTP_STATUS_BAD_REQUEST 400 // syntax errors in the call		
#define HTTP_STATUS_NOT_FOUND 	404 // page not found

/*---------------------------------- Tcp error codes --------------------------------------  */
typedef enum {	//enum di messaggi di errore tcp
	CRASH_TCP_ERROR_CODE = -10,
	TCP_ERROR_CODE_COULD_NOT_SEND_MESSAGE,//self explanatory
	TELESCOPE_NOT_RESPONDING,	//telescope timed out error (didnt respond within TELESCOPE_TIMEOUT_MS)
	BELOW_HORIZON,				//slew to target DA coordinate rejected
	PARSING_ERROR,				//string recieved from lx200_send_and_recieve_CMD is syntattically wrong
	REJECTED_BY_TELESCOPE,		//target coordinates rejected
	SETUP_ERROR,				//something went wrong during setup

	//error messages should be always negative
	__tcp_error_coders_count__
} Tcp_error_codes;
static_assert(__tcp_error_coders_count__ <= 0 && "tcp_error_codes values are greater or equal than zero ");
//questa static assert controlla che i messaggi di errore siano minori di 0 o  non compila
/*---------------------------------- Tcp message markers --------------------------------------  */
//inizio e fine stringa tcp
#define TCP_MESSAGE_BEGIN_CHAR '@'
#define TCP_MESSAGE_BEGIN_STR "@"
#define TCP_MESSAGE_END_CHAR '$'
#define TCP_MESSAGE_END_STR "$"
#define TCP_MESSAGE_DATA_LEN 128
//this is supposed to be handled by tcp-socket but i put it this way to debug
#define TCP_DEFAULT_PORT 6767
#define TCP_DEFAULT_IP "192.168.10.12"

/* 
to check if a request has a body you should use
	http_server.hasArg("plain");
this define makes it clearer
*/
#define HTTP_BODY "plain"//non invi nulla in realtà nel body



/** define to help with sending and receiving 
 * @note const char* so it is saved once in eeprom
*/
//questi sono nomi delle key da mandare in json
/*	{
		"key": value,
	}
*/

extern const char* J_SERIAL_STATUS;
extern const char* J_SERIAL_ERROR_REASON;
extern const char* J_SERIAL_ERROR_CODE;

extern const char* J_POSITION_RA_STRING;
extern const char* J_POSITION_DA_STRING;
extern const char* J_POSITION_ERROR_REASON;
extern const char* J_POSITION_ERROR_CODE;	//0 position invalid, 1 below horizon or 2 above horizon

extern const char* J_STATUS_OK ;

/* max tcp connection attempts before throwing and error*/
#define MAX_ATTEMPT 30

/*---------------------------------- network data variables --------------------------------------  */

extern WebServer  http_server;

extern String  tcp_server_IP;
extern uint16_t tcp_server_port;
extern WiFiClient tcp_client;


/** Handle tcp events */ 
bool tcp_connect();
bool tcp_send_message( String & msg);
bool tcp_send_message( const char* msg = J_STATUS_OK);
bool tcp_send_message( JsonDocument& doc );
void tcp_send_error_message(const char* error_reason, const int error_code);
void tcp_disconnect( char* exit_msg = nullptr );

/*---------------------------------- HTTP callback functions --------------------------------------  */
void http_init();
void http_position_GET();
void http_position_PUT();

void http_position_RA_GET();
void http_position_RA_PUT();

void http_position_DA_GET();
void http_position_DA_PUT();

void http_position_home_PUT();
// settings//
void http_reset_ESP_PUT();
void http_reset_LX_PUT();
void http_heartbeat_GET();
// network //
void http_tcp_socket_GET();
void http_tcp_socket_PUT();


#endif //USE_WIFI

//////////////////////////////////////////////////////////////////////////////////////////////
/*--------------------------------------- PIN SECTION --------------------------------------------- */

#define SERIAL_TX 17
#define SERIAL_RX 16

#endif // _CONFIG_H_