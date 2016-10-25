#pragma once //achieves the same as #ifndef header guarding

#include <windows.h>
#include <string>
#include <tchar.h>

#include <opencv2/opencv.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include "SerialBuffer.h"

using namespace std;

class SerialManager
{
public:

	SerialManager();
	~SerialManager();
                                 
	bool Write(const char* data, DWORD size);
	bool Init(string portName, unsigned long baudRate, unsigned char parity, unsigned char stopBits, unsigned char byteSize);
	bool IsConnected();
	void Start();
	void Stop();
	void UnInit();

	void SendPoseMsg(float yaw, float pitch, float roll);
	void SendConfMsg(int interpolation);
	void SendRstMsg();
private:

	void ReadLoop();
	boost::thread ReadThread;
	bool stopped;

	//Variables
	HANDLE comPort;
	bool isConnected;
};