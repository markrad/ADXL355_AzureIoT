#include <iostream>
#include <string>
#include <stdexcept>
#include <algorithm> 
#include <cctype>

#include "ConfigSettings.h"
#include "parson/parson.h"

using namespace std;

const char *ConfigSettings::CONFIG_FILENAME = "config.json";

ConfigSettings::ConfigSettings(const char *filename)
{
    const string DEVICEID = "DEVICEID=";
    const string DEFAULTPROTOCOL = "MQTT";

    JSON_Value *rootValue = json_parse_file(filename);

    if (rootValue == NULL)
        throw runtime_error("Unable to open configuration file");

    JSON_Object *rootObject = json_value_get_object(rootValue);

    if (rootObject == NULL)
        throw runtime_error("Unable to parse configuration file");

    if (NULL != json_object_get_string(rootObject, "ConnectionString"))
    {
        _connectionString = json_object_get_string(rootObject, "ConnectionString");
    }
    else
    {
        throw runtime_error("Connection string is missing from configuration file");
    }

    if (NULL != json_object_get_string(rootObject, "DeviceId"))
    {
        _deviceId = json_object_get_string(rootObject, "DeviceId");
    }
    else
    {
        // Use the device id from the connection string
        string work = uppercase(GetConnectionString());
        size_t start = uppercase(GetConnectionString()).find(DEVICEID);

        if (start == string::npos)
            throw runtime_error("Bad connection string");

        start += DEVICEID.length();

        size_t end = GetConnectionString().find(';', start);

        if (end == string::npos)
            throw runtime_error("Bad connection string");

        _deviceId = GetConnectionString().substr(start, end - start);
    }

    if (NULL != json_object_get_string(rootObject,  "Protocol"))
    {
        _protocol = json_object_get_string(rootObject,  "Protocol");
    }
    else
    {
        _protocol = DEFAULTPROTOCOL;
    }
}

char ConfigSettings::op_upper(char in)
{
    return toupper(in);
}

string ConfigSettings::uppercase(const string &str)
{
    char x = toupper('f');
    string upper;

    upper.resize(str.length());
    transform(str.begin(), str.end(), upper.begin(), op_upper);

    return upper;
}