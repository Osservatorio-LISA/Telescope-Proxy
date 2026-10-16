/** 
 * This file contains the list of all Json keys of the API 
 */

#ifndef _J_SON_KEYS_
#define _J_SON_KEYS_

/* status & error keys*/
const char* J_SERIAL_STATUS 			         = "status";
const char* J_SERIAL_ERROR_REASON 		    	 = "reason";
const char* J_SERIAL_ERROR_CODE			 		 = "code";

/// TODO: pensa a dei nomi migliori
const char* J_POSITION_RA_STRING				 = "ra_pos";
const char* J_POSITION_RA_DEG					 = "ra_deg";
const char* J_POSITION_DA_STRING				 = "da_pos";
const char* J_POSITION_DA_DEG					 = "da_deg";
const char* J_POSITION_ERROR_REASON				 =  "error_r";
const char* J_POSITION_ERROR_CODE				 =  "error_c"; //1 below horizon or 2 above horizon

/* used to send a single ok status */
const char* J_STATUS_OK = "{\"status\":\"ok\"}";


#endif //_J_SON_KEYS_