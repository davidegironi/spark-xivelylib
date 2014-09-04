//********************************************************************
// XivelyLib for Spark
// http://github.com/davidegironi/spark-xivelylib
// Copyright (c) Davide Gironi, 2014 
//
// Released under GPLv3.
// Please refer to LICENSE file for licensing information.
//********************************************************************

#ifndef __XIVELYLIB_H
#define __XIVELYLIB_H

#define XIVELYLIB_UARTDEBUG 0
#define XIVELYLIB_UARTDEBUGREPONSE 0

#define XIVELYLIB_FEEDID_SIZE 14
#define XIVELYLIB_APIKEY_SIZE 49
#define XIVELYLIB_DATAPOINTID_SIZE 24
#define XIVELYLIB_DATAPOINTVALUE_SIZE 24

#define XIVELYLIB_URI "api.xively.com"
#define XIVELYLIB_PORT 8081

#define XIVELYLIB_STATUSSUCCESSFUL 200

#define XIVELYLIB_ERRORSRESET 5

#include "application.h"

typedef struct xivelyLib_datapoint
{
    char id[XIVELYLIB_DATAPOINTID_SIZE];
    char value[XIVELYLIB_DATAPOINTVALUE_SIZE];
    bool enabled;
} xivelyLib_datapoint;

class XivelyLib
{
    public:
    	XivelyLib(char *feedId, char *apiKey);
        void setEnabled(bool enabled);
        bool isEnabled();
        void setFeedId(char *feedId);
        char *getFeedId();
        bool validateFeedId(char *feedId);
        void setApiKey(char *apiKey);
        char *getApiKey();
        bool validateApiKey(char *feedId);
        bool isUpdateSuccessful();
        bool updateDatapoints(xivelyLib_datapoint *datapoints, int size);
    private:
        volatile bool _enabled;
        char _feedId[XIVELYLIB_FEEDID_SIZE];
        char _apiKey[XIVELYLIB_APIKEY_SIZE];
        volatile bool _isUpdateSuccessful;
};

#endif