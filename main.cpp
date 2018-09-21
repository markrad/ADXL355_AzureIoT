#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <signal.h>
#include <sys/time.h>

#include "ConfigSettings.h"
#include "CIoTHubDevice.h"
#include "CIoTHubMessage.h"
#include "ADXL355/ADXL355.h"

#include "parson/parson.h"

using namespace std;

// Global set by Ctrl-C handler
static bool quit = false;

void calibrateSensor(ADXL355 &sensor, int fifoReadCount);
void generateMessage(string &out, long fifoData[32][3], int entryCount, ConfigSettings &config);
void appendArray(string &out, long fifoData[32][3], int entryCount, int index);
void signalHandler(int sigNum);

void connectionStatusCallback(CIoTHubDevice &iotHubDevice, IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void *userContext)
{
    cout << "Connection status result="  << result << ";reason=" << reason << endl;
}

void eventConfirmationCallback(CIoTHubDevice &iotHubDevice, IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContext)
{
    cout << "Message confirmed with " << result << " and " << *((int *)userContext) << endl;
}

IOTHUBMESSAGE_DISPOSITION_RESULT MessageCallback(CIoTHubDevice &iotHubDevice, CIoTHubMessage &iotHubMessage, void *userContext)
{
    IOTHUBMESSAGE_CONTENT_TYPE type = iotHubMessage.GetContentType();

    if (type == IOTHUBMESSAGE_STRING)
    {
        cout << "C2D String Message: " << iotHubMessage.GetString() << endl;
    }
    else if (type == IOTHUBMESSAGE_BYTEARRAY)
    {
        const uint8_t *buffer;
        size_t length;

        iotHubMessage.GetByteArray(&buffer, &length);
        char *str = new char[length + 1];
        memcpy(str, buffer, length);
        str[length] = '\0';

        cout << "C2D ByteArray Message: " << str << endl;

        delete [] str;
    }
    else
    {
        cout << "C2D Unknown Message" << endl;
    }

    return IOTHUBMESSAGE_ACCEPTED;
}

int DeviceMethodCallback(CIoTHubDevice &iotHubDevice, const unsigned char *payload, size_t size, unsigned char** response, size_t* resp_size, void* userContext)
{
    const char *fixedResp = "{ \"TestResponse\": \"Success\"}";
    char *str = new char[size + 1];
    memcpy(str, payload, size);
    str[size] = '\0';

    cout << "Test method called" << str << endl;

    *response =  new unsigned char[strlen(fixedResp) + 1];
    strcpy((char *)*response, fixedResp);
    *resp_size = strlen((char *)*response) + 1;

    return 200;
}

int UnknownDeviceMethodCallback(CIoTHubDevice &iotHubDevice, const char *methodName, const unsigned char *payload, size_t size, unsigned char** response, size_t* resp_size, void* userContext)
{
    cout << "Unknown method " << methodName << " called" << endl;
    *resp_size = 0;
    *response = NULL;

    return 404;
}

int main(int, char**)
{
    try
    {
        ConfigSettings config;
        cout << "ConnectionString=" << config.GetConnectionString() << endl;
        cout << "DeviceId=" << config.GetDeviceId() << endl;

        CIoTHubDevice::Protocol protocol;
        string configProtocol = config.GetProtocol();
        
        if (configProtocol == "MQTT")
        {
            protocol = CIoTHubDevice::Protocol::MQTT;
        }
        else if (configProtocol == "MQTTWS")
        {
            protocol = CIoTHubDevice::Protocol::MQTT_WebSockets;
        }
        else
        {
            cerr << "Invalid protocol" << endl;
            return 4;
        }

        CIoTHubDevice *device = new CIoTHubDevice(config.GetConnectionString(), protocol);

        device->SetConnectionStatusCallback(connectionStatusCallback, (void *)255);
        device->SetMessageCallback(MessageCallback, NULL);
        device->SetDeviceMethodCallback("Test", DeviceMethodCallback);
        device->SetUnknownDeviceMethodCallback(UnknownDeviceMethodCallback);
    
        ADXL355 sensor(0x1d);

        sensor.Stop();
        
        if (!sensor.IsI2CSpeedFast())
		{
			sensor.SetI2CSpeed(true);
        }

        sensor.SetRange(ADXL355::RANGE_VALUES::RANGE_2G);
		sensor.SetOdrLpf(ADXL355::ODR_LPF::ODR_31_25_AND_7_813);
		calibrateSensor(sensor, 5);

        signal(SIGINT, signalHandler);
        int i = 0;

        sensor.Start();

        long fifoOut[32][3];
        string message;
        int result;

        while (!quit)
        //for (int i = 0; i < 50; i++)
        {
            if (sensor.IsFifoFull())
            {
                if (-1 != (result = sensor.ReadFifoEntries((long *)fifoOut)))
                {
                    cout << "Retrieved " << result << " entries" << endl;

                    generateMessage(message, fifoOut, result, config);
                    cout << "Sending message " << i++ << endl;
                    cout << message << endl;
                    device->SendEventAsync("{ \"test\" : 10 }", eventConfirmationCallback, &i);
                }
                else
                {
                    cerr << "Fifo read failed" << endl;
                }
            }

            device->DoWork();
			this_thread::sleep_for(chrono::milliseconds(10));
        }

        while (device->WaitingEvents())
        {
            device->DoWork();
        }

        delete device;
    }
    catch (exception &e)
    {
        cerr << "Exception: " << e.what() << endl;
    }
}

void calibrateSensor(ADXL355 &sensor, int fifoReadCount)
{
	long fifoOut[32][3];
	int result;
	int readings = 0;
	long totalx = 0;
	long totaly = 0;
	long totalz = 0;

	memset(fifoOut, 0, sizeof(fifoOut));

	cout << endl << "Calibrating device with " << fifoReadCount << " fifo reads" << endl << endl;

	sensor.Stop();
	sensor.SetTrim(0, 0, 0);
	sensor.Start();
	this_thread::sleep_for(chrono::milliseconds(2000));

	for (int j = 0; j < fifoReadCount; j++)
	{
		cout << "Fifo read number " << j + 1 << endl;

		while (!sensor.IsFifoFull())
		{
			this_thread::sleep_for(chrono::milliseconds(10));
		}

		if (-1 != (result = sensor.ReadFifoEntries((long *)fifoOut)))
		{
			cout << "Retrieved " << result << " entries" << endl;
			readings += result;

			for (int i = 0; i < result; i++)
			{
				totalx += fifoOut[i][0];
				totaly += fifoOut[i][1];
				totalz += fifoOut[i][2];
			}
		}
		else
		{
			cerr << "Fifo read failed" << endl;
		}
	}

	long avgx = totalx / readings;
	long avgy = totaly / readings;
	long avgz = totalz / readings;

	cout
		<< endl
		<< "Total/Average X=" << totalx << "/" << avgx
		<< "; Y=" << totaly << "/" << avgy
		<< "; Z=" << totalz << "/" << avgz
		<< endl << endl;

	sensor.Stop();
	sensor.SetTrim(avgx, avgy, avgz);

	int32_t xTrim;
	int32_t yTrim;
	int32_t zTrim;

	result = sensor.GetTrim(&xTrim, &yTrim, &zTrim);

	if (result == 0)
	{
		cout << "xTrim=" << xTrim << ";yTrim=" << yTrim << ";zTrim=" << zTrim << endl;
	}

	sensor.Start();
	this_thread::sleep_for(chrono::milliseconds(2000));
}

void signalHandler(int sigNum)
{
    cout << endl << "Quitting..." << endl;
    quit = true;
}

void generateMessage(string &out, long fifoData[32][3], int entryCount, ConfigSettings &config)
{
    struct timeval tp;
    
    gettimeofday(&tp, NULL);
    string seconds = to_string(tp.tv_sec);
    string fraction = to_string(tp.tv_usec);

    out = "[{ \"deviceid\" : \"";
    out += config.GetDeviceId();
    out += "\", \"magictime\" : ";
    out += seconds;
    out += ".";
    out += fraction;
    out += ", \"magicx\" : ";
    appendArray(out, fifoData, entryCount, 0);
    out += ", \"magicy\" : ";
    appendArray(out, fifoData, entryCount, 1);
    out += ", \"magicz\" : ";
    appendArray(out, fifoData, entryCount, 2);

    out += "}]";

    return;
}

void appendArray(string &out, long fifoData[32][3], int entryCount, int index)
{
    out += "[";
    
    for (int i = 0; i < entryCount; i++)
    {
        out += to_string(ADXL355::ValueToGals(fifoData[i][index]));

        if (entryCount - i != 1)
            out += ", ";
    }

    out += "]";
}
