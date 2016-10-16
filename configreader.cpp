#include "configreader.h"

ConfigReader::ConfigReader()
{}

ConfigReader::~ConfigReader()
{}


/*ConfigReader *ConfigReader::instance()
{
	if (!config_instance)
		config_instance = new ConfigReader();

	return config_instance;
}*/

bool ConfigReader::ParseConfig()
{
	XMLDocument config;
	XMLError result = config.LoadFile("./config.xml");
	
	if (!CheckResult(result))
	{
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

	/*firstElement = rootNode->FirstChildElement("WebCam");

	if (firstElement == nullptr)
	{
		std::cout << "[ConfigReader] Error: Could not grab WebCam element!" << std::endl;
		return false;
	}

	if (!ParseWebcamSettings(firstElement))
	{
		std::cout << "[ConfigReader] Error: Failed reading serial config settings" << std::endl;
		return false;
	}*/
	
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
	}

	std::string port = portText;

	//Get Baud Value
	unsigned int config_baud;
	configElement = serialElement->FirstChildElement("Baud");
	result = configElement->QueryUnsignedText(&config_baud);
	baudRate = (unsigned long)config_baud;

	//Get Parity
	unsigned int config_parity;
	configElement = serialElement->FirstChildElement("Parity");
	result = configElement->QueryUnsignedText(&config_parity);
	parity = (unsigned char)config_parity;

	//Get StopBits
	unsigned int config_bits;
	configElement = serialElement->FirstChildElement("StopBits");
	result = configElement->QueryUnsignedText(&config_bits);
	stopBits = (unsigned char)config_bits;

	//Get StopBits
	unsigned int config_byte;
	configElement = serialElement->FirstChildElement("ByteSize");
	result = configElement->QueryUnsignedText(&config_byte);
	byteSize = (unsigned char)config_byte;
	
	std::cout << "[ConfigReader] Serial Settings Read: " << std::endl
			  << "Port: " << port.c_str() << std::endl
			  << "Parity: " << parity << std::endl
			  << "StopBits: " << stopBits << std::endl
			  << "ByteSize: " << byteSize << std::endl;
}

bool ConfigReader::ParseWebcamSettings(XMLElement * webCamElement)
{

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