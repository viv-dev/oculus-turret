
#include <math.h>
#include <assert.h>
#include <Process.h>
#include <string>
#include <iostream>


#include "SerialManager.h"

#define DEGREES_TO_DIGI 0.2929

//C++98 standard library does not have a round function, this is a workaround for VS2010
#if (_MSC_VER == 1600)
float round(float number)
{
	return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

double round(double number)
{
	return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}
#endif

SerialManager::SerialManager() :
	stopped(false)
{
}

SerialManager::~SerialManager()
{
	UnInit();
}

bool SerialManager::Init(string portName, unsigned long baudRate, unsigned char parity, unsigned char byStopBits, unsigned char byteSize)
{

	bool isInit = false;

	std::wstring stemp = std::wstring(portName.begin(), portName.end());
	LPCWSTR winPortName = stemp.c_str();

	//Open the COM Port
	comPort = ::CreateFile(winPortName,
							 GENERIC_READ | GENERIC_WRITE, //access: read and write
							 0,                          //share: 0 cannot share the COM port
							 0,                          //security: 0 none
							 OPEN_EXISTING,              // creation : open_existing
							 FILE_FLAG_OVERLAPPED,       // overlapped operation to minimise blocking
							 0                           // templates : 0 none
	);

	//Check COM port was opened correctly
	if (comPort == INVALID_HANDLE_VALUE)
	{
		cout << "[SerialManager] Failed to open COM Port Reason: " << GetLastError() << endl;
		return false;
	}

	cout << "[SerialManager] COM port opened successfully" << endl;

	//Set COM event masks - notify when a character is recieved (EV_RXCHAR) and when sending is complete (EV_TXEMPTY)
	if (!::SetCommMask(comPort, EV_RXCHAR | EV_TXEMPTY))
	{
		cout << "[SerialManager] Failed to Set Comm Mask Reason: " << GetLastError() << endl;
		return false;
	}

	cout << "[SerialManager] SetCommMask() success" << endl;

	//Create Device Control Block (DCB) to set serial settings
	DCB dcb = { 0 };

	dcb.DCBlength = sizeof(DCB);

	if (!::GetCommState(comPort, &dcb))
	{
		cout << "[SerialManager]  Failed to Get Comm State Reason: " << GetLastError() << endl;
		return false;
	}

	//Configure Serial Communications settings
	dcb.BaudRate = (DWORD)baudRate;
	dcb.ByteSize = (BYTE)byteSize;
	dcb.Parity = (BYTE)parity;

	if (byStopBits == 1)
		dcb.StopBits = ONESTOPBIT;
	else if (byStopBits == 2)
		dcb.StopBits = TWOSTOPBITS;
	else
		dcb.StopBits = ONE5STOPBITS;

	dcb.fDsrSensitivity = 0;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fOutxDsrFlow = 0;

	//Send the command to configure and detect if it was successful
	if (!::SetCommState(comPort, &dcb))
	{
		cout << "[SerialManager]  Failed to Set Comm State Reason: " << GetLastError() << endl;
		return false;
	}

	//Print Out Configured Settings
	cout << "[SerialManager] Current Settings:" << endl;
	cout << "[SerialManager] Baud Rate: " << (int)dcb.BaudRate << endl;
	cout << "[SerialManager] Parity: " << (int)dcb.Parity << endl;
	cout << "[SerialManager] Byte Size: " << (int)dcb.ByteSize << endl;
	cout << "[SerialManager] Stop Bits: " << ((dcb.StopBits == ONESTOPBIT) ? "1" : (dcb.StopBits == TWOSTOPBITS) ? "2" : "1.5") << endl;

	//now set the timeouts ( we control the timeout overselves using WaitForXXX()
	COMMTIMEOUTS timeouts;

	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(comPort, &timeouts))
	{
		cout << "[SerialManager] Error setting time-outs: " << GetLastError() << endl;
		return false;
	}

	isConnected = true;
	isInit = true;

	return isInit;

}


void SerialManager::Start()
{
	stopped = false;
	ReadThread = boost::thread(&SerialManager::ReadLoop, this);
}

void SerialManager::Stop()
{
	if (stopped = false)
	{
		stopped = true;
		ReadThread.join();
	}
	
}

void SerialManager::UnInit()
{
	Stop();
	isConnected = false;
	
	if (!CloseHandle(comPort))
	{
		assert(0);
		cout << "[SerialManager] Failed to close handle " << comPort << " :Last Error: " << GetLastError();
	}

	comPort = INVALID_HANDLE_VALUE;
}

/*
	Since no data is being sent back from the arduino at this point
	no proper serial read mechanism has been implemented.
*/
void SerialManager::ReadLoop()
{

	OVERLAPPED overlappedRead;
	memset(&overlappedRead, 0, sizeof(overlappedRead));
	overlappedRead.hEvent = CreateEvent(NULL, true, FALSE, NULL);

	if (overlappedRead.hEvent == NULL)
	{
		std::cout << "[SerialManager] Error creating overlapped read event!" << std::endl;
		stopped = true;
	}

	while (!stopped)
	{
		DWORD waitResult;
		DWORD eventMask;


		bool packetStarted = false;
		bool packetComplete = false;
		
		unsigned int bufferPointer = 0;
		std::string serialBuffer;
		std::string receivedPacket;

		//Wait for a communications event on the com port
		if (!WaitCommEvent(comPort, &eventMask, &overlappedRead))
			assert(GetLastError() == ERROR_IO_PENDING);

		waitResult = WaitForSingleObject(overlappedRead.hEvent, INFINITE);

		switch (waitResult)
		{
			case WAIT_OBJECT_0:
			{

				DWORD bytesRead = 0;
				bool readReturn = false;
				
				do
				{
					ResetEvent(overlappedRead.hEvent);

					char tmp[1];
					int size = sizeof(tmp);
					memset(tmp, 0, size);
					
					if (!ReadFile(comPort, tmp, size, &bytesRead, &overlappedRead))
					{
						stopped = true;
						break;
					}
					
					if (bytesRead > 0)
					{
						if (packetStarted)
						{
							serialBuffer += tmp;
							bufferPointer++;

							if (bufferPointer >= 256)
							{
								std::cout << "[SerialManager] Packet size exceeded buffer, packet dropped." << std::endl;
								serialBuffer.erase();
								packetStarted = false;
								bufferPointer = 0;
							}
							else if (tmp[0] == '#')
							{
								receivedPacket = serialBuffer;
								serialBuffer.erase();

								bufferPointer = 0;
								packetStarted = false;

								//ProcessPacket(receivedPacket);
							}
							else if (tmp[0] == '^')
							{
								serialBuffer += tmp[0];
								bufferPointer++;
								packetStarted = true;
							}
						}
					}
				} while (bytesRead > 0);

				ResetEvent(overlappedRead.hEvent);
			}
			break;
		}
	}
}

bool SerialManager::Write(const char* data, DWORD dwSize)
{
	int writeReturn = 0;

	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = CreateEvent(0, true, 0, 0);

	DWORD bytesWritten = 0;

	writeReturn = WriteFile(comPort, data, dwSize, &bytesWritten, &overlapped);

	if (writeReturn == 0)
		WaitForSingleObject(overlapped.hEvent, INFINITE);

	CloseHandle(overlapped.hEvent);
	return true;
}

bool SerialManager::IsConnected()
{
	return isConnected;
}

void SerialManager::SendPoseMsg(float yaw, float pitch, float roll)
{
	double digiYaw, digiPitch;
	std::stringstream ss;
	string message;

	digiYaw = yaw + 150.0;

	if (digiYaw > 300)
		digiYaw = 300;

	if (digiYaw < 0)
		digiYaw = 0;

	digiPitch = pitch + 150.0;

	digiYaw = round(digiYaw / DEGREES_TO_DIGI);
	digiPitch = round(digiPitch / DEGREES_TO_DIGI);

	ss << "^POSE," << digiYaw << ","<< digiPitch << "#";

	message = ss.str();

	Write(message.c_str(), message.size());
}

void SerialManager::SendConfMsg(int interpolation)
{
	std::stringstream ss;
	string message;

	ss << "^CONF," << interpolation << "#";

	message = ss.str();

	Write(message.c_str(), message.size());
}

void SerialManager::SendRstMsg()
{
	string message = "^RST,#";

	Write(message.c_str(), message.size());
}