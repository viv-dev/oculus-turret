#include "configreader.h"
#include <sstream>


ConfigReader::ConfigReader()
{}

ConfigReader::~ConfigReader()
{}

bool ConfigReader::ParseConfig()
{
	std::string fileName = "./config.xml";
	XMLDocument config;
	XMLError result = config.LoadFile(fileName.c_str());
	
	if (!CheckResult(result))
	{
		if(result == XML_ERROR_EMPTY_DOCUMENT)
			std::cout << "[ConfigReader] Error: xml file empty!" << std::endl;
		else
			std::cout << "[ConfigReader] Error: could not open config.xml file!" << std::endl;
		
		return false;
	}

	XMLNode * rootNode = config.FirstChild();

	if (rootNode == nullptr)
	{
		std::cout << "[ConfigReader] Error: Could not grab root node!" << std::endl;
		return false;
	}

	XMLElement * firstElement = rootNode->FirstChildElement("Serial");

	if (firstElement == nullptr)
	{
		std::cout << "[ConfigReader] Error: Could not grab Serial element!" << std::endl;
		return false;
	}

	if (!ParseSerialSettings(firstElement))
	{
		std::cout << "[ConfigReader] Error: Failed reading serial config settings" << std::endl;
		return false;
	}

	firstElement = rootNode->FirstChildElement("WebCam");

	if (firstElement == nullptr)
	{
		std::cout << "[ConfigReader] Error: Could not grab WebCam element!" << std::endl;
		return false;
	}

	if (!ParseWebcamSettings(firstElement))
	{
		std::cout << "[ConfigReader] Error: Failed reading serial config settings" << std::endl;
		return false;
	}

	std::cout << std::endl;
	
	return true;
}

bool ConfigReader::ParseSerialSettings(XMLElement * serialElement)
{
	XMLError result;

	// Get Serial Port Value
	XMLElement * configElement = serialElement->FirstChildElement("Port");
	
	const char * portText = nullptr;
	portText = configElement->GetText();
	
	if (portText == nullptr)
	{
		std::cout << "[ConfigReader] Error: Read null serial port value!" << std::endl;
		return false;
	}

	port = portText;

	//Get Baud Value
	unsigned int config_baud;
	configElement = serialElement->FirstChildElement("Baud");
	result = configElement->QueryUnsignedText(&config_baud);
	baudRate = (unsigned long)config_baud;

	if (!CheckResult(result))
	{
		std::cout << "[ConfigReader] Error: could not read Baud value!" << std::endl;
		return false;
	}

	//Get Parity Value
	unsigned int config_parity;
	configElement = serialElement->FirstChildElement("Parity");
	result = configElement->QueryUnsignedText(&config_parity);
	parity = (unsigned char)config_parity;

	if (!CheckResult(result))
	{
		std::cout << "[ConfigReader] Error: could not read Parity value!" << std::endl;
		return false;
	}

	//Get StopBits Value
	unsigned int config_bits;
	configElement = serialElement->FirstChildElement("StopBits");
	result = configElement->QueryUnsignedText(&config_bits);
	stopBits = (unsigned char)config_bits;

	if (!CheckResult(result))
	{
		std::cout << "[ConfigReader] Error: could not read StopBits value!" << std::endl;
		return false;
	}

	//Get ByteSize Value
	unsigned int config_byte;
	configElement = serialElement->FirstChildElement("ByteSize");
	result = configElement->QueryUnsignedText(&config_byte);
	byteSize = (unsigned char)config_byte;
	
	if (!CheckResult(result))
	{
		std::cout << "[ConfigReader] Error: could not read ByteSize value!" << std::endl;
		return false;
	}

	std::cout << std::endl
		      << "[ConfigReader] Serial Settings Read: " << std::endl
			  << "Port: " << port.c_str() << std::endl
			  << "Baud: " << config_baud << std::endl
			  << "Parity: " << config_parity << std::endl
			  << "StopBits: " << config_bits << std::endl
			  << "ByteSize: " << config_byte << std::endl;

	return true;
}

bool ConfigReader::ParseWebcamSettings(XMLElement * webCamElement)
{
	XMLError result;

	// Get Serial Port Value
	XMLElement * configElement = webCamElement->FirstChildElement("WebCam");

	//Get NumOfWebCams Value
	configElement = webCamElement->FirstChildElement("NumOfWebCams");
	result = configElement->QueryUnsignedText(&numOfWebCams);

	if (!CheckResult(result))
	{
		std::cout << "[ConfigReader] Error: could not read NumOfWebCams value!" << std::endl;
		return false;
	}

	//Get FOVH Value
	configElement = webCamElement->FirstChildElement("FOVH");
	result = configElement->QueryFloatText(&FOVH);

	if (!CheckResult(result))
	{
		std::cout << "[ConfigReader] Error: could not read FOVH value!" << std::endl;
		return false;
	}

	//Get FOVH Value
	configElement = webCamElement->FirstChildElement("MaxHeight");
	result = configElement->QueryFloatText(&height);

	if (!CheckResult(result))
	{
		std::cout << "[ConfigReader] Error: could not read MaxHeight value!" << std::endl;
		return false;
	}

	//Get FOVH Value
	configElement = webCamElement->FirstChildElement("MaxWidth");
	result = configElement->QueryFloatText(&width);

	if (!CheckResult(result))
	{
		std::cout << "[ConfigReader] Error: could not read MaxWidth value!" << std::endl;
		return false;
	}

	std::cout << std::endl
		<< "[ConfigReader] Webcam Settings Read: " << std::endl
		<< "NumOfWebCams: " << numOfWebCams << std::endl
		<< "FOVH: " << FOVH << std::endl
		<< "Width: " << width << std::endl
		<< "Height: " << height << std::endl;

	return true;
}

bool ConfigReader::CheckResult(XMLError result)
{
	if (result != XML_SUCCESS)
	{
		std::cout << "[ConfigReader] TinyXML Error: " << result << std::endl;
		return false;
	}

	return true;
}