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

	//static ConfigReader *instance();

	static ConfigReader& instance()
	{
		static ConfigReader instance;
		return instance;
	}

private:

	bool ParseSerialSettings(XMLElement * serialElement);
	bool ParseWebcamSettings(XMLElement * webCamElement);

	// Webcam Config
	unsigned int numOfWebCams;
	float FOVH;
	float width;
	float height;

	// Serial Config
	std::string port;
	unsigned long baudRate;
	unsigned char parity;
	unsigned char stopBits;
	unsigned char byteSize;

	// ConfigReader Singleton
	//static ConfigReader *config_instance;
};