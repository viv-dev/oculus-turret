#pragma once //achieves the same as #ifndef header guarding

#include <windows.h>
#include <string>
#include <tchar.h>
#include "SerialBuffer.h"

typedef enum
{
	SS_Unknown,
	SS_UnInitialised,
	SS_Init,
	SS_Started,
	SS_Stopped

} SERIAL_STATE;



using namespace std;

class SerialManager
{
public:

	SerialManager();
	~SerialManager();

	HANDLE GetWaitForEvent()
	{
		return m_dataRx;
	};

	//Handle Buffer Access
	inline void LockThis()
	{
		EnterCriticalSection(&m_lock);
	}
	inline void UnLockThis()
	{
		LeaveCriticalSection(&m_lock);
	}
	inline void InitLock()
	{
		InitializeCriticalSection(&m_lock);
	}
	inline void DelLock()
	{
		DeleteCriticalSection(&m_lock);
	}

	//API For Accessing Recieved Data
	bool IsInputAvailable()
	{
		LockThis();
		bool isData = (!m_serialBuffer.IsEmpty());
		UnLockThis();
		return isData;
	}

	bool IsConnected()
	{
		return m_isConnected;
	}

	void SetDataReadEvent()
	{
		SetEvent(m_dataRx);
	}

	HRESULT         Read_N(string& data, long n, long timeOut);                      //Returns n number of characters from the serial buffer
	HRESULT         Read_Upto(string& data, char delimiter, long* n, long timeOut);  //Reads upto n characters from the serial buffer
	HRESULT         ReadAvailable(string& data);                                     //Reads all available characters in the serial buffer
	HRESULT         Write(const char* data, DWORD size);                             //Writes data to the serial port
	HRESULT         Init(string portName, DWORD baudRate, BYTE parity, BYTE stopBits, BYTE byteSize); //Can be used with default arguments
	HRESULT         Start();
	HRESULT         Stop();
	HRESULT         UnInit();
	HRESULT         CanProcess();

	static unsigned __stdcall ReadThread(void* pvParam);

	//void OnSetDebugOption(long  iOpt,BOOL bOnOff);

private:
	//Variables
	SERIAL_STATE m_state;
	HANDLE m_comPort;
	HANDLE m_threadTerminate;
	HANDLE m_thread;
	HANDLE m_threadStarted;
	HANDLE m_dataRx;
	bool m_isConnected;

	//Functions
	void InvalidateHandle(HANDLE& handle);
	void CloseAndCleanHandle(HANDLE& handle);
	SERIAL_STATE GetCurrentState()
	{
		return m_state;
	}

	SerialBuffer m_serialBuffer;
	CRITICAL_SECTION m_lock;
};