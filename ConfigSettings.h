#ifndef _CONFIGSETTINGS_H
#define _CONFIGSETTINGS_H

#include <string>

#include "parson/parson.h"

class ConfigSettings
{
private:
    static const char *CONFIG_FILENAME;

    std::string _connectionString;
    std::string _deviceId;
    std::string _protocol;

public:
    ConfigSettings(const char *filename = CONFIG_FILENAME);

    std::string GetConnectionString() { return _connectionString; }
    std::string GetDeviceId() { return _deviceId; }
    std::string GetProtocol() { return _protocol; }

private:
    static char op_upper(char in);
    static std::string uppercase(const std::string &str);
};

#endif
