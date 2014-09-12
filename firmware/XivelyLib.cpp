//********************************************************************
// XivelyLib for Spark
// http://github.com/davidegironi/spark-xivelylib
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
	_xivelyErrors = 0;
	_dataSent = false;
}

/**
 * Update datapoints listed to Xively
 */
void XivelyLib::updateDatapoints(xivelyLib_datapoint *datapoints, int size) {
    //connect to xively
    if (!_client.connected()) {
        #if XIVELYLIB_UARTDEBUG == 1
        Serial.print("Connect to xively... ");
        #endif
        if(_client.connect(XIVELYLIB_URI, XIVELYLIB_PORT)) {
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
    if (_client.connected()) {
        #if XIVELYLIB_UARTDEBUG == 1
        Serial.println("Sending data to xively... ");
        #endif
        
        // Connection succesful, update datastreams
        _client.println("{");
        _client.println("\"method\" : \"PUT\",");
        _client.print("\"resource\" : \"/feeds/");
            _client.print(_feedId);
            _client.println("\",");
        _client.println("\"params\" : {},");
        _client.print("\"headers\" : {\"X-ApiKey\":\"");
            _client.print(_apiKey);
            _client.println("\"},");
        _client.print("\"body\" : ");
            _client.print("{ ");
            _client.print("\"version\" : \"1.0.0\", ");
            _client.println("\"datastreams\" : [ ");
            int enableddatatreams = 0;
            for(int i=0; i<size; i++) {
                if(datapoints[i].enabled)
                    enableddatatreams++;
            }
            for(int i=0; i<size; i++) {
                if(datapoints[i].enabled) {
                    //set datapoint
                    _client.print("{ ");
                    _client.print("\"id\" : \"");
                    _client.print(datapoints[i].id);
                    _client.print("\",");
                    _client.print("\"current_value\" : \"");
                    _client.print(datapoints[i].value);
                    _client.print("\"");
                    if(enableddatatreams > 1)
                        _client.println("}, ");
                    else
                        _client.println("} ");
                    enableddatatreams--;
                }
            }
            _client.print("] ");
            _client.println("}");
        _client.println("}");
        _client.println();
        
        _client.flush();
        
        _dataSent = true;
        
        #if XIVELYLIB_UARTDEBUG == 1
        Serial.println("Disconnect from xively.");
        #endif
    } else {
        #if XIVELYLIB_UARTDEBUG == 1
        Serial.println("Can not send data to xively, connection fails.");
        #endif
        
        if(_xivelyErrors < XIVELYLIB_ERRORSRESET)
            _xivelyErrors++;
    }
}

/**
 * Check return response from Xively
 */
void XivelyLib::responseListener() {
    if(_dataSent) {
        if (_client.connected()) {
            if (_client.available()) {
                _dataSent = false;
                
                //get xively 200 (SUCCESSFUL) status code
                char c = '\0';
                int statusState = 0;
                int statusCode = 0;
                unsigned long statusResponseStart = millis();
                do {
                    const char* statusPrefix = "\"status\":";
                    const char* statusPtr = statusPrefix;
                    while (c != '\n') {
                        if (_client.available()) {
                            c = _client.read();
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
                            }
                        } else {
                            delay(50); //delay a little before checking again
                        }
                        
                        if(millis() - statusResponseStart > 1000 * 30) //wait 30 sec max
                            break;
                            
                    }
                    if ((c == '\n') && (statusCode < 200)) {
                        //c = '\0'; //retry reading
                        break;  //stop if end of response message ('\n') is reached but the status is not catched
                    }
                    
                    if(millis() - statusResponseStart > 1000 * 30) //wait 30 sec max
                        break;
                }
                while ((statusState == 2) && (statusCode < 200));
                if ((c == '\n') && (statusState == 2) && (statusCode == XIVELYLIB_STATUSSUCCESSFUL)) {
                    #if XIVELYLIB_UARTDEBUG == 1
                    Serial.println("Xively response: Ok.");
                    #endif
                    
                    //set the status led and reset errors
                    _xivelyErrors = 0;
                    _isUpdateSuccessful = true;
                } else {
                    #if XIVELYLIB_UARTDEBUG == 1
                    Serial.println("Xively response: Error.");
                    #endif
                    
                    if(_xivelyErrors < XIVELYLIB_ERRORSRESET)
                        _xivelyErrors++;
                }
                
                //check errors
                if(_xivelyErrors >= XIVELYLIB_ERRORSRESET) {
                    _isUpdateSuccessful = false;
                }
                
                #if XIVELYLIB_UARTDEBUG == 1
                Serial.println("Disconnect from xively...");
                #endif
                _client.stop();   
            }
        }
    }
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
