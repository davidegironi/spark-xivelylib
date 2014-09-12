//********************************************************************
// XivelyLib for Spark
// http://github.com/davidegironi/spark-xivelylib
// Copyright (c) Davide Gironi, 2014 
//
// Released under GPLv3.
// Please refer to LICENSE file for licensing information.
//********************************************************************

#include "XivelyLib/XivelyLib.h"

//default xively feed and api
#define XIVELY_FEEDID "000000000"
#define XIVELY_APIKEY "000000000000000000000000000000000000000000000000"

//debug to serial port
#define UARTDEBUG 1

//application version
#define APPVERSION "0.1"
#define APPNAME "XivelyLib\r\nCopyright(c) Davide Gironi, 2014"

//xively status led port
#define XIVELY_STATUSLED_PORT D7

//reload default values to eeprom
#define CONFIG_LOADDEFAULT 1

//xively feed update time (seconds)
#define XIVELY_UPDATETIMERINTERVALSEC 60

//active connection test time (minutes)
#define SPARK_CONNECTIONINTERVALMIN 5

//sensor1
#define SENSOR1_PORT A0
#define SENSOR1_UPDATETIMERINTERVALMS 10000 //timer to update the reading of this sensor
//datastream channel
#define SENSOR1_CHANNEL "1"
//datastream variables
long xivelydatapoint_sensor1tot = 0;
int xivelydatapoint_sensor1totcount = 0;

//sensor2
#define SENSOR2_PORT A1
#define SENSOR2_UPDATETIMERINTERVALMS 5000 //timer to update the reading of this sensor
//datastream channel
#define SENSOR2_CHANNEL "2"
//datastream variables
long xivelydatapoint_sensor2tot = 0;
int xivelydatapoint_sensor2totcount = 0;

//application info
int ping = 1;
char appName[64] = APPNAME;
char appVersion[12] = APPVERSION;

//xively status led
int xively_statusLed = XIVELY_STATUSLED_PORT;

//xively datapoints
xivelyLib_datapoint xively_datapoints;

//storage id
#define CONFIG_VERSION "st1"
//storage start address
#define CONFIG_START 0
//storage data
struct ConfigStruct {
    char version[4];
    char xively_feedid[XIVELYLIB_FEEDID_SIZE]; //set the xively feedid
    char xively_apikey[XIVELYLIB_APIKEY_SIZE]; //set the xively apikey
} config = {
    CONFIG_VERSION,
    XIVELY_FEEDID,
    XIVELY_APIKEY
};

//initialize the xively library
XivelyLib xively(XIVELY_FEEDID, XIVELY_APIKEY);

// Initialize
void setup()
{
    //register spark functions and variables
    Spark.variable("ping", &ping, INT);
    Spark.variable("appName", &appName, STRING);
    Spark.variable("appVersion", &appVersion, STRING);
    Spark.function("feedId", call_setFeedId);
    Spark.function("apiKey", call_setApiKey);
    Spark.variable("feedId", &config.xively_feedid, STRING);
    Spark.variable("apiKey", &config.xively_apikey, STRING);
 
    //start serial port if enabled
    #if UARTDEBUG == 1
    Serial.begin(9600);
    Serial.print("Starting...");
    #endif
 
    //setup input sensor
    pinMode(SENSOR1_PORT, INPUT);
    pinMode(SENSOR2_PORT, INPUT);
    
    //setup xively status led
    pinMode(xively_statusLed, OUTPUT);
    digitalWrite(xively_statusLed, LOW);
    
    //reload default config values
    #if CONFIG_LOADDEFAULT == 1
    strncpy(config.xively_feedid, XIVELY_FEEDID, strlen(XIVELY_FEEDID));
    strncpy(config.xively_apikey, XIVELY_APIKEY, strlen(XIVELY_APIKEY));
    saveConfig();
    #endif
        
    //load config
    loadConfig();
}


// Main loop
void loop()
{
    static unsigned long xivelyUpdateTimer = millis(); //xively update timer
    static unsigned long sparkConnectionTimer = millis(); //spark connection timer

    //do sensor readings
    sensorUpdate();

    //get xively response
    xively.responseListener();
    
    //send data to xively
    if (millis()-xivelyUpdateTimer > 1000*XIVELY_UPDATETIMERINTERVALSEC) {
        //debug xively status
        #if UARTDEBUG == 1
        Serial.print("Xively feed id: ");
        Serial.println(xively.getFeedId());
        Serial.print("Xively api key: ");
        Serial.println(xively.getApiKey());
        Serial.print("Xively update successful: ");
        Serial.println(xively.isUpdateSuccessful());
        #endif
        
        //update values to xively
        xivelyUpdate();
        
        //reset update timer
        xivelyUpdateTimer = millis();
    }
    
    //check if the spark connection is active
    if (millis()-sparkConnectionTimer > 1000*60*SPARK_CONNECTIONINTERVALMIN) {
        if(!Spark.connected()) //reconnect spark
            Spark.connect();
        sparkConnectionTimer = millis();
    }
}

// Load configuration
void loadConfig() {
    if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] && EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] && EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2]) {
        for (unsigned int t=0; t<sizeof(config); t++) {
            *((char*)&config + t) = EEPROM.read(CONFIG_START + t);
        }
    }
}

// Save configuration
void saveConfig() {
  for (unsigned int t=0; t<sizeof(config); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&config + t));
}

// Update sensor values
void sensorUpdate() { 
    static unsigned long sensor1_updateTimer = millis(); //analog update timer
    static unsigned long sensor2_updateTimer = millis(); //analog update timer
    
    //check when to get the sensor readings
    if (millis()-sensor1_updateTimer > SENSOR1_UPDATETIMERINTERVALMS) {
        sensor1_updateTimer = millis();
        
        #if UARTDEBUG == 1
        Serial.println("Aquire sensor1 data... ");
        #endif
        xivelydatapoint_sensor1tot += analogRead(SENSOR1_PORT);
        xivelydatapoint_sensor1totcount++;
    }
    
    //check when to get the sensor readings
    if (millis()-sensor2_updateTimer > SENSOR2_UPDATETIMERINTERVALMS) {
        sensor2_updateTimer = millis();
        
        #if UARTDEBUG == 1
        Serial.println("Aquire sensor2 data... ");
        #endif
        xivelydatapoint_sensor2tot += analogRead(SENSOR2_PORT);
        xivelydatapoint_sensor2totcount++;
    }
}

// Update data to xively
void xivelyUpdate() {
    char numtemp[10];
	double d = 0;
	int datapointsIndex = 0;
	
	//set the datapoints array
	xivelyLib_datapoint datapoints[2];
	for(int i=0; i<sizeof(datapoints)/sizeof(datapoints[0]); i++) {
	    datapoints[i].enabled = false;
	    memset(datapoints[i].id, 0, sizeof(datapoints[i].id));
	    memset(datapoints[i].value, 0, sizeof(datapoints[i].value));
	}
	
    //set sensor1 datapoint
    if(xivelydatapoint_sensor1totcount != 0) {
        d = (double)xivelydatapoint_sensor1tot/(double)xivelydatapoint_sensor1totcount;
        memset(numtemp, 0, sizeof(numtemp));
    	sprintf(numtemp, "%3.1f", d);
    	datapoints[datapointsIndex].enabled = true;
        strncpy(datapoints[datapointsIndex].id, SENSOR1_CHANNEL, strlen(SENSOR1_CHANNEL));
        strncpy(datapoints[datapointsIndex].value, numtemp, strlen(numtemp));
        //reset sensor1 readings
        xivelydatapoint_sensor1tot = 0;
        xivelydatapoint_sensor1totcount = 0;
    }
    //skip to next datapoints index
    datapointsIndex++;
    
    //set sensor2 datapoint
    if(xivelydatapoint_sensor2totcount) {
        d = (double)xivelydatapoint_sensor2tot/(double)xivelydatapoint_sensor2totcount;
        memset(numtemp, 0, sizeof(numtemp));
    	sprintf(numtemp, "%3.1f", d);
    	datapoints[datapointsIndex].enabled = true;
        strncpy(datapoints[datapointsIndex].id, SENSOR2_CHANNEL, sizeof(SENSOR2_CHANNEL));
        strncpy(datapoints[datapointsIndex].value, numtemp, sizeof(numtemp));
        //reset sensor1 readings
        xivelydatapoint_sensor2tot = 0;
        xivelydatapoint_sensor2totcount = 0;
    }
    //skip to next datapoints index
    datapointsIndex++;
    
    //update to xively
    xively.updateDatapoints(datapoints, sizeof(datapoints)/sizeof(datapoints[0]));
    
    //update xively status led
    if(xively.isUpdateSuccessful()) {
        digitalWrite(xively_statusLed, HIGH);
    } else {
        digitalWrite(xively_statusLed, LOW);
    }
}

// Set config xively_feedid
int call_setFeedId(String feedId) {
    char feedIdc[XIVELYLIB_FEEDID_SIZE];
    feedId.toCharArray(feedIdc, sizeof(feedIdc));
    if(xively.validateFeedId(feedIdc)) {
        //update new value to eeprom
        feedId.toCharArray(config.xively_feedid, sizeof(config.xively_feedid));
        saveConfig();
        //update the value to xively
        xively.setFeedId(config.xively_feedid);
        return 1;
    } else {
        return 0;
    }
}

// Set config xively_apikey
int call_setApiKey(String apiKey) {
    char apiKeyc[XIVELYLIB_APIKEY_SIZE];
    apiKey.toCharArray(apiKeyc, sizeof(apiKeyc));
    if(xively.validateApiKey(apiKeyc)) {
        //update new value to eeprom
        apiKey.toCharArray(config.xively_apikey, sizeof(config.xively_apikey));
        saveConfig();
        //update the value to xively
        xively.setApiKey(config.xively_apikey);
        return 1;
    } else {
        return 0;
    }
}
