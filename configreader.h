#pragma once

#include <iostream>

#include "tinyxml2.h"

using namespace tinyxml2;

class ConfigReader
{

public:

	ConfigReader();
	~ConfigReader();
	
	bool ParseConfig();
	bool CheckResult(XMLError result);

// Config Reader Singleton Instance
#if (_MSC_VER == 1600) //If using VS2010
	ConfigReader *ConfigReader::instance()
	{
		if (!config_instance)
			config_instance = new ConfigReader();

		return config_instance;
	}
#else
	static ConfigReader& getInstance()
	{
		static ConfigReader instance;
		return instance;
	}
#endif

	//Get Serial Values
	std::string getPort() { return port; }
	unsigned long getBaudRate() { return baudRate; }
	unsigned char getParity() { return parity; }
	unsigned char getStopBits() { return stopBits; }
	unsigned char getByteSize() { return byteSize; }

	//Get Webcam Values
	unsigned int getNumWebCams() { return numOfWebCams; }
	float getFOV() { return baudRate; }
	float getWidth() { return width; }
	float getHeight() { return height; }

private:

	bool ParseSerialSettings(XMLElement * serialElement);
	bool ParseWebcamSettings(XMLElement * webCamElement);

	// Serial Config
	std::string port;
	unsigned long baudRate;
	unsigned char parity;
	unsigned char stopBits;
	unsigned char byteSize;

	// Webcam Config
	unsigned int numOfWebCams;
	float FOVH;
	float width;
	float height;

	// ConfigReader Singleton
#if (_MSC_VER == 1600)
	static ConfigReader *config_instance;
#endif

};