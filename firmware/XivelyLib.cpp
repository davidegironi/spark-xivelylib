//********************************************************************
// XivelyLib for Spark
// Copyright (c) Davide Gironi, 2014
// 
// Released under GPLv3.
// Please refer to LICENSE file for licensing information.
//********************************************************************

#include "XivelyLib.h"
#include <string.h> 
#include "spark_wiring_usartserial.h"

/**
 * Initialize the XivelyLib
 * Set the Feed and the Api Key
 */
XivelyLib::XivelyLib(char *feedId, char *apiKey) {
    setFeedId(feedId);
    setApiKey(apiKey);
	_enabled = true;
}

/**
 * Update datapoints listed to Xively
 */
bool XivelyLib::updateDatapoints(xivelyLib_datapoint *datapoints, int size) {
    int statusOk = 0;
    if(_enabled) {
        static int xivelyErrors = 0; //number of errors happens
        TCPClient client; //TCP xively connection socket
        
        //connect to xively
        if (!client.connected()) {
            #if XIVELYLIB_UARTDEBUG == 1
            Serial.print("Connect to xively... ");
            #endif
            if(client.connect(XIVELYLIB_URI, XIVELYLIB_PORT)) {
                #if XIVELYLIB_UARTDEBUG == 1
                Serial.println("successful.");
                #endif
            } else {
                #if XIVELYLIB_UARTDEBUG == 1
                Serial.println("fail.");
                #endif
            }
        }
        
        //send data
        if (client.connected()) {
            #if XIVELYLIB_UARTDEBUG == 1
            Serial.print("Send data to xively... ");
            #endif
            
            // Connection succesful, update datastreams
            client.println("{");
            client.println("\"method\" : \"PUT\",");
            client.print("\"resource\" : \"/feeds/");
                client.print(_feedId);
                client.println("\",");
            client.println("\"params\" : {},");
            client.print("\"headers\" : {\"X-ApiKey\":\"");
                client.print(_apiKey);
                client.println("\"},");
            client.print("\"body\" : ");
                client.print("{ ");
                client.print("\"version\" : \"1.0.0\", ");
                client.println("\"datastreams\" : [ ");
                for(int i=0; i<size; i++) {
                    //set datapoint
                    client.print("{ ");
                    client.print("\"id\" : \"");
                    client.print(datapoints[i].id);
                    client.print("\",");
                    client.print("\"current_value\" : \"");
                    client.print(datapoints[i].value);
                    client.print("\"");
                    if(i+1!=size)
                        client.println("}, ");
                    else
                        client.println("} ");
                    
                }
                client.print("] ");
                client.println("}");
            client.println("}");
            client.println();
            
            client.flush();
            
            //get xively 200 (SUCCESSFUL) status code
            char c = '\0';
            int statusState = 0;
            int statusCode = 0;
            do {
                unsigned long statusResponseStart = millis();
                const char* statusPrefix = "\"status\":";
                const char* statusPtr = statusPrefix;
                while ((c != '\n') &&  ( (millis() - statusResponseStart) < 30*1000 )) {
                    if (client.available()) {
                        c = client.read();
                        #if XIVELYLIB_UARTDEBUG == 1 && XIVELYLIB_UARTDEBUGRESPONSE == 1
                        Serial.print(c);
                        #endif
                        if (c != -1) {
                            switch(statusState) {
                                case 0:
                                    if ((*statusPtr == '*') || (*statusPtr == c)) {
                                        statusPtr++;
                                        if (*statusPtr == '\0') {
                                            statusState = 1;
                                        }
                                    }
                                    break;
                                case 1:
                                    if (isdigit(c)) {
                                        statusCode = statusCode*10 + (c - '0');
                                    } else {
                                        statusState = 2;
                                    }
                                    break;
                                case 2:
                                    break;
                            }
                            statusResponseStart = millis();
                        }
                    } else {
                        delay(500); //delay a little before checking again
                    }
                }
                if ((c == '\n') && (statusCode < 200)) {
                    c = '\0';
                }
            }
            while ((statusState == 2) && (statusCode < 200));
            if ((c == '\n') && (statusState == 2) && (statusCode == XIVELYLIB_STATUSSUCCESSFUL)) {
                #if XIVELYLIB_UARTDEBUG == 1
                Serial.println("successful.");
                #endif
                
                statusOk = 1;
                
                //set the status led and reset errors
                xivelyErrors = 0;
                _isUpdateSuccessful = true;
            } else {
                #if XIVELYLIB_UARTDEBUG == 1
                Serial.println("fail.");
                #endif
                
                if(xivelyErrors < XIVELYLIB_ERRORSRESET)
                    xivelyErrors++;
            }
        
            #if XIVELYLIB_UARTDEBUG == 1
            Serial.println("Disconnect from xively.");
            #endif
        } else {
            #if XIVELYLIB_UARTDEBUG == 1
            Serial.println("Can not send data to xively, connection fails.");
            #endif
            if(xivelyErrors < XIVELYLIB_ERRORSRESET)
                xivelyErrors++;
        }
        
        //stop the connection
        client.stop();
        
        //check errors
        if(xivelyErrors >= XIVELYLIB_ERRORSRESET) {
            _isUpdateSuccessful = false;
        }
    }
    return (statusOk == 1 ? true : false);
}

/**
 * Enable or disable di XivelyLib
 */
void XivelyLib::setEnabled(bool enabled) {
    _enabled = enabled;
}

/**
 * Is the XivelyLib enabled
 */
bool XivelyLib::isEnabled() {
    return _enabled;
}

/**
 * Set the XivelyLib Feed
 */
void XivelyLib::setFeedId(char *feedId) {
    memset(_feedId, 0, sizeof(_feedId));
    strncpy(_feedId, feedId, strlen(feedId));
}

/**
 * Get the XivelyLib Feed
 */
char *XivelyLib::getFeedId() {
    return _feedId;
}

/**
 * Check if a Feed is valid
 */
bool XivelyLib::validateFeedId(char *feedId) {
    if(strlen(feedId) < XIVELYLIB_FEEDID_SIZE && strlen(feedId) > 0) {
        return true;
    } else {
        return false;
    }
}

/**
 * Set the XivelyLib Api Key
 */
void XivelyLib::setApiKey(char *apiKey) {
    memset(_apiKey, 0, sizeof(_apiKey));
    strncpy(_apiKey, apiKey, strlen(apiKey));
}

/**
 * Get the XivelyLib Api Key
 */
char *XivelyLib::getApiKey() {
    return _apiKey;
}

/**
 * Check if a Api Key is valid
 */
bool XivelyLib::validateApiKey(char *apiKey) {
    if(strlen(apiKey) == XIVELYLIB_APIKEY_SIZE-1) {
        return true;
    } else {
        return false;
    }
}

/**
 * Check the update status
 */
bool XivelyLib::isUpdateSuccessful() {
    return _isUpdateSuccessful;
}
