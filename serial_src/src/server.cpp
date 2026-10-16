/** @file server.cpp
 *  @author Mauro Gori
 *  @author Alessandro Maryni
 * 
 * @brief Implementation of HTTP & TCP functions in dome_config.h
 * 
 * OVERVIEW:
 * This API follows the REST (Representational State Transfer) architecture for requests,
 * while responses are dispatched via TCP connection to the LISA main controller. 
 * This decoupled approach allows for better modularity within the LISA ecosystem.
 * 
 * WORKFLOW:
 * To extend or use this architecture, a basic understanding of <WebServer.h> is required.
 * 
 * 1. CALLBACKS & ROUTING
 *    To enable the server to answer a client request, each request must be 'mapped' 
 *    to a specific function. This process is called 'routing':
 *    
 *    http_server.on("/your/path", HTTP_METHOD, callback_function);
 *      
 *    Each `http_server.on()` call registers a (Path, Method) pair into an internal list.
 *    When a request arrives, the server dispatches it by searching this list linearly:
 *    - If a match is found, the associated callback function is executed.
 *    - If no match exists, the `.onNotFound()` handler is triggered (defaulting to 404).
 *    
 *    NOTE: Since the search is linear, registration order matters for performance 
 *    and to prevent "shadowing" (where a generic path might accidentally catch 
 *    a request intended for a more specific one).
 * 
 * 2. INPUT VALIDATION
 *    Query parameters are used to refine requests (e.g., filtering GET keys or 
 *    specifying units for a PUT command). 
 *    - Use `http_server.hasArg("key")` to check for URL parameters (?key=value).
 *    - Use `http_server.arg("key")` to access the value of that parameter.
 *    - To access the request body, use the HTTP_BODY macro instead of a specific "key".
 * 
 * 3. RESPONSES (Two-Layer System)
 *    This system issues TWO separate responses to ensure robust communication:
 *    - HTTP RESPONSES : Used for transmission status ONLY. 
 *      If the request is valid, return HTTP_STATUS_OK, HTTP_STATUS_OK_NO_BODY, 
 *      or HTTP_STATUS_ACCEPTED (if processing takes time). 
 *      The HTTP response should be sent as early as possible to free the connection.
 *      
 *    - TCP RESPONSES : Used for data payload ONLY. 
 *      This layer carries the actual status and data keys.
 *      NOTE: To be valid, every TCP message MUST include a "status" field set to 
 *      "ok" or "offline". Any other value is treated as an error by the controller.
 */

#include "serial_config.h"
#ifdef USE_WIFI
/** Handle tcp events */
//IMPLEMENTAZIONE FUNZIONI TCP (NON FUNZIONI DI CALLBACK)
//su tcp connect è necessario sapere l'ip del server
bool tcp_connect(){
    if(tcp_client.connected()) return true;
    for(size_t attempt = 0; attempt < MAX_ATTEMPT; ++attempt) {
        if(tcp_client.connect(tcp_server_IP.c_str(), tcp_server_port)){
            return true;
        }
        delay(250);
    }
    return false;
}

bool tcp_send_message( JsonDocument& doc ){
    String msg;
    serializeJson(doc,msg);
    return tcp_send_message(msg);
}

bool tcp_send_message(const char* msg){
    String s_msg = msg;
    return tcp_send_message(s_msg);
}

bool tcp_send_message( String & msg ){ 
    if(!tcp_client || !tcp_client.connected()){
        tcp_connect();
    }
    /* 
    This won't work because in ESP32 there is not a tx buffer per say
    Data is send to FreeRTOS that will handle the message
    calling this function will always return 0 in an ESP32

    if(tcp_client.availableForWrite() < msg.length() + 2) {
        Serial.println(F("tcp TX buffer is full"));
        return false;
    }
    */

    msg = TCP_MESSAGE_BEGIN_CHAR + msg + TCP_MESSAGE_END_CHAR;

    Serial.print("sending: ");Serial.println(msg);
    size_t sent_bytes = tcp_client.write(msg.c_str());

    if(sent_bytes != msg.length() ){
        Serial.println(F("could not send message"));
        return false;
    }
    return true;
}

void tcp_send_error_message(const char* error_reason, const int error_code = 0){
    JsonDocument error;
    error[J_SERIAL_STATUS]       = "error";
    error[J_SERIAL_ERROR_REASON] = error_reason;
    error[J_SERIAL_ERROR_CODE]   = error_code;
    tcp_send_message(error);
}

void tcp_disconnect( const char* exit_msg = nullptr ){
    if(tcp_client.connected()){
        if(exit_msg!=nullptr){
            String ext = exit_msg;
            tcp_send_message(ext);
        }
        tcp_client.flush();
        tcp_client.stop();
    }
}



/** Handle http events */
//IMPLEMENTAZIONE FUNZIONI HTTP
void http_init(){
    if(WiFi.status() != WL_CONNECTED){
        Serial.println(F("could not initialize web, no wifi"));
        return;
    }

    Serial.println(F("Server initialisation"));

    /*  root */ 
    http_server.on("/", [](){
        http_server.send(HTTP_STATUS_OK, F("text/plain"), F("welcome to LISA SERIAL!"));
    });

    /* position */
    //QUI ASSOCIO AI PATH LE FUNZIONI DI CALLBACK DICHIARATE NEL CONFIG
    http_server.on("/position/RA", HTTP_GET, http_position_RA_GET);
    http_server.on("/position/RA", HTTP_PUT, http_position_RA_PUT);
    http_server.on("/position/DA", HTTP_GET, http_position_DA_GET);
    http_server.on("/position/DA", HTTP_PUT, http_position_DA_PUT);
    http_server.on("/position/home", HTTP_PUT, http_position_home_PUT);
    http_server.on("/position", HTTP_GET, http_position_GET);
    http_server.on("/position", HTTP_PUT, http_position_PUT);

    /* network */
    http_server.on("/network/tcp-socket"   , HTTP_PUT , http_tcp_socket_PUT);
    http_server.on("/network/tcp-socket"   , HTTP_GET , http_tcp_socket_GET);


    /* settings */
    http_server.on("/settings/heartbeat", HTTP_GET, http_heartbeat_GET);
    http_server.on("/settings/resetESP",  HTTP_PUT, http_reset_ESP_PUT);
    http_server.on("/settings/resetLX",   HTTP_PUT, http_reset_LX_PUT);

    /* Not Found */
    http_server.onNotFound([](){
        String method_str;
        switch (http_server.method()) {
            case HTTP_GET:     method_str = "GET";    break;
            case HTTP_POST:    method_str = "POST";   break;
            case HTTP_PUT:     method_str = "PUT";    break;
            case HTTP_DELETE:  method_str = "DELETE"; break;
            default:           method_str = "UNKNOWN";break;
        }
        Serial.printf("NOT_FOUND: %s %s\n", method_str, http_server.uri().c_str());
        http_server.send(HTTP_STATUS_NOT_FOUND);
    });

    http_server.begin();
    Serial.println(F("http_Server started!"));
}
/*
/lx200/ 
    /network/
        /tcp-socket/    -> PUT:  sets tcp-server socket config (ip , port),
                                ip is necessary, port is 7000 by default
                                set this params via query (not 100% REST but saves ram. Might change it later.)
                        -> GET: get current tcp socket
    /settings/
        /resetESP/  -> PUT: resetta esp
        /resetLX/   -> PUT: invia stringa di reset al telesc.
        /heartbeat/ -> GET: "quick connectivity check"
    /position/      -> GET: ottieni entrambe le posizioni 
                    -> PUT: imposta entrambe le posizioni e parti: position?RA=""&DA=""
        /RA/        -> GET: ottieni stringa di RA
                    -> PUT: imposta RA con query e parti
        /DA/        -> GET: ottieni stringa di DA
                    -> PUT: imposta DA con query e parti
        /home/      -> PUT: invia :I#
    /stop/

    // PUT /position/RA?RA=HH:MM:SS
*/
//QUI IMPLEMENTO LE FUNZIONI DI CALLBACK --> loro gestiscono la richiesta
// position //
///TODO: IMPLEMENTARE QUESTE FUNZIONI

/**
 * @brief callback function for position GET
 * @note sends a JSON file via TCP
 */
void http_position_GET(){
    http_server.send(HTTP_STATUS_OK_NO_BODY);
    JsonDocument doc;
    doc[J_SERIAL_STATUS] = "ok";
    doc[J_POSITION_RA_STRING] = AltAz2RAString(currentRA);
    doc[J_POSITION_DA_STRING] = AltAz2DAString(currentDA);
    if(!tcp_send_message(doc))
        tcp_send_error_message("could not send RA", TCP_ERROR_CODE_COULD_NOT_SEND_MESSAGE);
}

// PUT /position?RA=HH:MM:SS&DA=sDD:MM:SS   (es. RA=10:10:00&DA=-05:30:00)
/**
 * @brief callback function for position PUT
 * @note must be called with a query with the following syntax: /position?RA=HH:MM:SS&DA=sDD:MM:SS (es. RA=10:10:00&DA=-05:30:00)
 */
void http_position_PUT(){
    if(!http_server.hasArg("RA") || !http_server.hasArg("DA")){
        http_server.send(HTTP_STATUS_BAD_REQUEST, F("text/plain"),
                         F("Missing RA or/and DA argument\n/position?RA=HH:MM:SS&DA=sDD:MM:SS"));
        return;
    }

    // --- RA ---
    int rh, rm, rs;
    String ra = http_server.arg("RA");
    ra.trim();
    if(sscanf(ra.c_str(), "%d:%d:%d", &rh, &rm, &rs) != 3 ||
       rh < 0 || rh > 23 || rm < 0 || rm > 59 || rs < 0 || rs > 59){
        http_server.send(HTTP_STATUS_BAD_REQUEST, F("text/plain"),
                         F("invalid RA, expected HH:MM:SS"));
        return;
    }

    // --- DA ---
    // note: in a query string the "+" is a space so the absence of a sign is a positive
    String da = http_server.arg("DA");
    da.trim();
    bool negative = false;
    if(da.startsWith("-")){
        negative = true;
        da.remove(0, 1);
    } else if(da.startsWith("+")){
        da.remove(0, 1);
    }

    int dd, dm, ds;
    if(sscanf(da.c_str(), "%d%*[:*]%d:%d", &dd, &dm, &ds) != 3 ||
       dd < 0 || dd > 90 || dm < 0 || dm > 59 || ds < 0 || ds > 59){
        http_server.send(HTTP_STATUS_BAD_REQUEST, F("text/plain"),
                         F("invalid DA, expected sDD:MM:SS"));
        return;
    }

    http_server.send(HTTP_STATUS_ACCEPTED);

    targetRA.positive = true;
    targetRA.hours    = (uint8_t)rh;
    targetRA.minutes  = (uint8_t)rm;
    targetRA.seconds  = (uint8_t)rs;

    targetDA.positive = !negative;
    targetDA.hours    = (uint8_t)dd;
    targetDA.minutes  = (uint8_t)dm;
    targetDA.seconds  = (uint8_t)ds;

    if(!lx200_setRA(targetRA)) return;
    if(!lx200_setDA(targetDA)) return;

    int res = lx200_go();

    if(res == 0){
        JsonDocument doc;
        doc[J_SERIAL_STATUS]      = "ok";
        doc[J_POSITION_RA_STRING] = AltAz2RAString(targetRA);
        doc[J_POSITION_DA_STRING] = AltAz2DAString(targetDA);
        if(!tcp_send_message(doc))
            tcp_send_error_message("could not send position", TCP_ERROR_CODE_COULD_NOT_SEND_MESSAGE);
    } else if(res == 1){
        tcp_send_error_message("slew rejected: below horizon", BELOW_HORIZON);
    } else {
        tcp_send_error_message("unexpected reply to slew command", PARSING_ERROR);
    }
}
/**
 * @brief RA get callback function, sends the current RA coordinate saved in JSON file
 */
void http_position_RA_GET(){
    http_server.send(HTTP_STATUS_OK_NO_BODY);
    JsonDocument doc;
    //update the position if changed
    currentRA = lx200_getRA();
    doc[J_SERIAL_STATUS] = "ok";
    doc[J_POSITION_RA_STRING] = AltAz2RAString(currentRA);
    if(!tcp_send_message(doc))
        tcp_send_error_message("could not send RA", TCP_ERROR_CODE_COULD_NOT_SEND_MESSAGE);
}
/**
 * @brief callback function for position RA PUT
 * @note must be called with a query with the following syntax: /position?RA=HH:MM:SS(es. RA=10:10:00)
 */
void http_position_RA_PUT(){
    if(!http_server.hasArg("RA")){
        http_server.send(HTTP_STATUS_BAD_REQUEST, F("text/plain"),
                         F("Missing RA argument\n/position/RA?RA=HH:MM:SS"));
        return;
    }

    int h, m, s;
    String ra = http_server.arg("RA");
    if(sscanf(ra.c_str(), "%d:%d:%d", &h, &m, &s) != 3 ||
       h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59){
        http_server.send(HTTP_STATUS_BAD_REQUEST, F("text/plain"),
                         F("invalid RA, expected HH:MM:SS"));
        return;
    }

    http_server.send(HTTP_STATUS_ACCEPTED);

    targetRA.positive = true;
    targetRA.hours    = (uint8_t)h;
    targetRA.minutes  = (uint8_t)m;
    targetRA.seconds  = (uint8_t)s;

    if(!lx200_setRA(targetRA)) return;

    int res = lx200_go();   // 0 ok, 1 problem

    if(res == 0){
        JsonDocument doc;
        doc[J_SERIAL_STATUS]      = "ok";
        doc[J_POSITION_RA_STRING] = AltAz2RAString(targetRA);
        if(!tcp_send_message(doc))
            tcp_send_error_message("could not send RA", TCP_ERROR_CODE_COULD_NOT_SEND_MESSAGE);
    } else if(res == 1){
        tcp_send_error_message("slew rejected: below horizon", BELOW_HORIZON);
    } else if(res == 2){
        tcp_send_error_message("slew rejected: above high limit", REJECTED_BY_TELESCOPE);
    } else {
        tcp_send_error_message("unexpected reply to slew command", PARSING_ERROR);
    }
}
/**
 * @brief callback function for GET DA
 * @note returns a JSON file with the current DA
 */
void http_position_DA_GET(){
    http_server.send(HTTP_STATUS_OK_NO_BODY);
    JsonDocument doc;
    currentDA = lx200_getDA();
    doc[J_SERIAL_STATUS] = "ok";
    doc[J_POSITION_DA_STRING] = AltAz2DAString(currentDA);
    if(!tcp_send_message(doc))
        tcp_send_error_message("could not send RA", TCP_ERROR_CODE_COULD_NOT_SEND_MESSAGE);
}
void http_position_DA_PUT(){
}
void http_position_home_PUT(){
    http_server.send(HTTP_STATUS_OK);
}
// settings//
void http_reset_ESP_PUT(){
    http_server.send(HTTP_STATUS_OK);
}
void http_reset_LX_PUT(){
    http_server.send(HTTP_STATUS_OK);
}



// ========================================== NETWORK ========================================== //
/**
 * @brief callback function for the TCP socket PUT, useful to change the IP and PORT of the device
 * @note gotta put the new IP&port in the query
 */
void http_tcp_socket_PUT(){
    if( http_server.args() < 1 || http_server.args() > 2 ){ //se non ha due argomenti crasha
        http_server.send(HTTP_STATUS_BAD_REQUEST, F("text/plain"), F("invalid number of arguments\n/system/config-tcp-socket?IP=...&port=..."));
        return;
    } else if(!http_server.hasArg("IP") || !http_server.hasArg("port")){
        http_server.send(HTTP_STATUS_BAD_REQUEST, F("text/plain"), F("Missing IP or/and port argument\n/system/config-tcp-socket?IP=...&port=..."));
        return; //se non ho la porta do errore via http
    }
    String IP = http_server.arg("IP");
    uint16_t port = (uint16_t)http_server.arg("port").toInt();
    struct in_addr dst;
    // lwip_inet_pton returns 1 if the address was successfully converted
    if(lwip_inet_pton(AF_INET, IP.c_str() , &dst) != 1){
        http_server.send(HTTP_STATUS_BAD_REQUEST, F("text/plain"), F(" invalid IP "));
        return;
    }
    http_server.send(HTTP_STATUS_OK, F("text/plain"), F("OK"));
//variabili globali nel config
    tcp_server_IP = IP;
    tcp_server_port = port;
    Serial.print("[ Config ]\nIP:");Serial.println(tcp_server_IP);
    Serial.print("port:");Serial.println(tcp_server_port);

    tcp_send_message(J_STATUS_OK);
}

/** 
 * NOTE: we send json here via http because if tcp is not config
 * we wouldn't be able to get the message
 */
//con questo chiedo al modulo le informazioni che ha salvate via http e tcp
/**
 * @brief tcp-socket callback GET function
 * @note returns info in a JSON files
 */
void http_tcp_socket_GET(){ 
    JsonDocument doc;
    String msg;
    doc[J_SERIAL_STATUS] = "ok";
    doc["IP"] = tcp_server_IP;
    doc["port"] = tcp_server_port;
    serializeJson( doc , msg );
    http_server.send(HTTP_STATUS_OK, "application/json", msg);
    tcp_send_message(msg);
}
// ========================================== DIAGNOSTIC ========================================== //


void http_heartbeat_GET(){  //il server controlla che tu sia vivo
    if (tcp_server_IP == "None" || tcp_server_port == 0){   //se non ho la porta mando questo errore
        JsonDocument doc;
        String msg;
        doc["method"] = "PUT";
        doc["path"]   = "/network/tcp-socket";
        doc["reason"] = "server TCP socket not configured yet";
        serializeJson( doc , msg );
        //http_server.send(HTTP_STATUS_RETRY_WITH, "application/json", msg);
        
    } else {
        http_server.send(HTTP_STATUS_OK_NO_BODY);
        /** heart beat requires A tcp message,
         *  we send status so that we can keep global state of the server ok */
        /** NOTE: if it's too heavy or useless send just the position
         *  http_position_GET();
         */
        //http_status_GET();  //nel dubbio invio lo stato
    }
}

#endif //USE_WIFI