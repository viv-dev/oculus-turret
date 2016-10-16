

#include <assert.h>
#include <Process.h>
#include <string>
#include <iostream>
#include "SerialManager.h"

SerialManager::SerialManager()
{
	InvalidateHandle(m_comPort);
	InvalidateHandle(m_threadTerminate);
	InvalidateHandle(m_thread);
	InvalidateHandle(m_threadStarted);
	InvalidateHandle(m_dataRx);

	InitLock();

	m_state = SS_UnInitialised;
}

SerialManager::~SerialManager()
{
	m_state = SS_Unknown;
	DelLock();
}

void SerialManager::InvalidateHandle(HANDLE& hHandle)
{
	hHandle = INVALID_HANDLE_VALUE;
}


void SerialManager::CloseAndCleanHandle(HANDLE& hHandle)
{
	BOOL isClosed = CloseHandle(hHandle);

	if (!isClosed)
	{
		assert(0);
		cout << "[SerialManager] Failed to open Close Handle " << hHandle << " :Last Error: " << GetLastError();
	}

	InvalidateHandle(hHandle);
}

HRESULT SerialManager::Init(string portName, DWORD baudRate, BYTE parity, BYTE byStopBits, BYTE byteSize)
{

	bool isInit = false;

	std::wstring stemp = std::wstring(portName.begin(), portName.end());
	LPCWSTR winPortName = stemp.c_str();

	try
	{
		//Setup Data Recieve event
		m_dataRx = CreateEvent(0, 0, 0, 0);

		//Open the COM Port
		m_comPort = ::CreateFile(winPortName,
								 GENERIC_READ | GENERIC_WRITE, //access: read and write
								 0,                          //share: 0 cannot share the COM port
								 0,                          //security: 0 none
								 OPEN_EXISTING,              // creation : open_existing
								 FILE_FLAG_OVERLAPPED,       // overlapped operation to minimise blocking
								 0                           // templates : 0 none
								 );

		//Check COM port was opened correctly
		if (m_comPort == INVALID_HANDLE_VALUE)
		{
			cout << "[SerialManager] Failed to open COM Port Reason: " << GetLastError() << endl;
			//assert(0);
			return E_FAIL;
		}

		cout << "[SerialManager] COM port opened successfully" << endl;

		//Set COM event masks - notify when a character is recieved (EV_RXCHAR) and when sending is complete (EV_TXEMPTY)
		if (!::SetCommMask(m_comPort, EV_RXCHAR | EV_TXEMPTY))
		{
			assert(0);
			cout << "[SerialManager] Failed to Set Comm Mask Reason: " << GetLastError() << endl;
			return E_FAIL;
		}

		cout << "[SerialManager] SetCommMask() success" << endl;

		//Create Device Control Block (DCB) to set serial settings
		DCB dcb = { 0 };

		dcb.DCBlength = sizeof(DCB);

		if (!::GetCommState(m_comPort, &dcb))
		{
			cout << "[SerialManager]  Failed to Get Comm State Reason: " << GetLastError() << endl;
			return E_FAIL;
		}

		//Configure Serial Communications settings
		dcb.BaudRate = baudRate;
		dcb.ByteSize = byteSize;
		dcb.Parity = parity;

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
		if (!::SetCommState(m_comPort, &dcb))
		{
			assert(0);
			cout << "[SerialManager]  Failed to Set Comm State Reason: " << GetLastError() << endl;
			return E_FAIL;
		}

		//Print Out Configured Settings
		cout << "[SerialManager] Current Settings:" << endl; 
		cout <<	"[SerialManager] Baud Rate: " << (int)dcb.BaudRate << endl;
		cout << "[SerialManager] Parity: " << (int)dcb.Parity << endl;
		cout << "[SerialManager] Byte Size: " << (int)dcb.ByteSize << endl; 
		cout << "[SerialManager] Stop Bits: " << ((dcb.StopBits == ONESTOPBIT)?"1": (dcb.StopBits == TWOSTOPBITS)?"2":"1.5") << endl;

		//now set the timeouts ( we control the timeout overselves using WaitForXXX()
		COMMTIMEOUTS timeouts;

		timeouts.ReadIntervalTimeout = MAXDWORD;
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		timeouts.WriteTotalTimeoutConstant = 0;

		if (!SetCommTimeouts(m_comPort, &timeouts))
		{
			assert(0);
			cout << "[SerialManager] Error setting time-outs: " << GetLastError() << endl;
			return E_FAIL;
		}

		//Create event to start and terminate thread
		m_threadTerminate = CreateEvent(0, 0, 0, 0);
		m_threadStarted = CreateEvent(0, 0, 0, 0);

		m_thread = (HANDLE)_beginthreadex(0, 0, SerialManager::ReadThread, (void*)this, 0, 0);

		DWORD wait = WaitForSingleObject(m_threadStarted, INFINITE);
		assert(wait == WAIT_OBJECT_0);

		CloseHandle(m_threadStarted);
		InvalidateHandle(m_threadStarted);

		m_isConnected = true;
		isInit = true;

	}
	catch (...)
	{
		assert(0);
		isInit = false;
	}

	if (isInit)
		m_state = SS_Init;

	return isInit;

}


HRESULT SerialManager::Start()
{
	m_state = SS_Started;
	return S_OK;

}
HRESULT SerialManager::Stop()
{
	m_state = SS_Stopped;
	return S_OK;
}

HRESULT SerialManager::UnInit()
{
	HRESULT success = S_OK;
	try
	{
		m_isConnected = false;
		SignalObjectAndWait(m_threadTerminate, m_thread, INFINITE, FALSE);
		CloseAndCleanHandle(m_threadTerminate);
		CloseAndCleanHandle(m_thread);
		CloseAndCleanHandle(m_dataRx);
		CloseAndCleanHandle(m_comPort);
	}
	catch (...)
	{
		assert(0);
		success = E_FAIL;
	}
	if (SUCCEEDED(success))
		m_state = SS_UnInitialised;
	return success;
}


unsigned __stdcall SerialManager::ReadThread(void* param)
{

	SerialManager* This = (SerialManager*)param;
	bool canContinue = true;
	DWORD eventMask = 0;

	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = CreateEvent(0, true, 0, 0);

	HANDLE handles[2];
	handles[0] = This->m_threadTerminate;

	DWORD wait;

	SetEvent(This->m_threadStarted);

	while (canContinue)
	{

		BOOL readReturn = ::WaitCommEvent(This->m_comPort, &eventMask, &overlapped);

		if (!readReturn)
			assert(GetLastError() == ERROR_IO_PENDING);

		handles[1] = overlapped.hEvent;

		//Wait until a serial event has been detected or 
		wait = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

		switch (wait)
		{
			case WAIT_OBJECT_0:
			{
				_endthreadex(1);
			}
			break;
			case WAIT_OBJECT_0 + 1:
			{
				DWORD mask;

				if (GetCommMask(This->m_comPort, &mask))
				{
					if (mask == EV_TXEMPTY)
					{
						ResetEvent(overlapped.hEvent);
						canContinue;
					}

				}

				//Read data here...
				int i = 0;

				This->m_serialBuffer.LockBuffer();

				try
				{
					BOOL readReturn = false;

					DWORD bytesRead = 0;
					OVERLAPPED overlappedRead;
					memset(&overlappedRead, 0, sizeof(overlappedRead));
					overlappedRead.hEvent = CreateEvent(0, true, 0, 0);

					do
					{
						ResetEvent(overlappedRead.hEvent);
						char tmp[1];

						int size = sizeof(tmp);
						memset(tmp, 0, size);

						readReturn = ::ReadFile(This->m_comPort, tmp, size, &bytesRead, &overlappedRead);

						if (!readReturn)
						{
							canContinue = FALSE;
							break;
						}
						if (bytesRead > 0)
						{
							This->m_serialBuffer.AddData(tmp, bytesRead);
							i += bytesRead;
						}

					} while (bytesRead > 0);

					CloseHandle(overlappedRead.hEvent);
				}
				catch (...)
				{
					assert(0);
				}

				//If not in start state, then flush queue
				if (This->GetCurrentState() != SS_Started)
				{
					i = 0;
					This->m_serialBuffer.Flush();
				}

				This->m_serialBuffer.UnLockBuffer();

				if (i > 0)
					This->SetDataReadEvent();

				ResetEvent(overlapped.hEvent);
			}
			break;
		}//switch
	}

	return 0;
}

HRESULT SerialManager::CanProcess()
{
	switch (m_state)
	{
		case SS_Unknown:
			assert(0);
			return E_FAIL;
		case SS_UnInitialised:
			return E_FAIL;
		case SS_Started:
			return S_OK;
		case SS_Init:
		case SS_Stopped:
			return E_FAIL;

		default:
			assert(0);
	}

	return false;
}

HRESULT SerialManager::Write(const char* data, DWORD dwSize)
{
	HRESULT success = CanProcess();

	if (success != S_OK) return success;

	int writeReturn = 0;

	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = CreateEvent(0, true, 0, 0);

	DWORD bytesWritten = 0;

	writeReturn = WriteFile(m_comPort, data, dwSize, &bytesWritten, &overlapped);
	if (writeReturn == 0)
		WaitForSingleObject(overlapped.hEvent, INFINITE);

	CloseHandle(overlapped.hEvent);
	std::string szData(data);

	return S_OK;
}

HRESULT SerialManager::Read_Upto(std::string& data, char terminator, long* count, long timeOut)
{
	HRESULT success = CanProcess();
	if (!success) return success;

	try
	{
		std::string tmp;
		tmp.erase();

		long bytesRead;

		bool found = m_serialBuffer.Read_Upto(tmp, terminator, bytesRead, m_dataRx);

		if (found)
			data = tmp;
		else
		{
			long iRead = 0;
			bool canContinue = true;

			while (canContinue)
			{
				cout << "[SerialManager] Read_Upto () making blocking read call." << endl;
				DWORD wait = ::WaitForSingleObject(m_dataRx, timeOut);

				if (wait == WAIT_TIMEOUT)
				{
					data.erase();
					success = E_FAIL;
					return success;

				}

				bool found = m_serialBuffer.Read_Upto(tmp, terminator, bytesRead, m_dataRx);


				if (found)
				{
					data = tmp;
					return S_OK;
				}

			}
		}
	}
	catch (...)
	{
		assert(0);
	}
	return success;

}

HRESULT SerialManager::Read_N(std::string& data, long count, long  timeOut)
{
	HRESULT success = CanProcess();

	if (FAILED(success))
	{
		assert(0);
		return success;
	}

	try
	{

		std::string tmp;
		tmp.erase();


		int iLocal = m_serialBuffer.Read_N(tmp, count, m_dataRx);

		if (iLocal == count)
			data = tmp;
		else
		{
			long iRead = 0;
			int iRemaining = count - iLocal;
			while (1)
			{
				DWORD wait = WaitForSingleObject(m_dataRx, timeOut);

				if (wait == WAIT_TIMEOUT)
				{
					data.erase();
					success = E_FAIL;
					return success;

				}

				assert(wait == WAIT_OBJECT_0);


				iRead = m_serialBuffer.Read_N(tmp, iRemaining, m_dataRx);
				iRemaining -= iRead;


				if (iRemaining == 0)
				{
					data = tmp;
					return S_OK;
				}
			}
		}
	}
	catch (...)
	{
		assert(0);
	}
	return success;

}
/*-----------------------------------------------------------------------
-- Reads all the data that is available in the local buffer..
does NOT make any blocking calls in case the local buffer is empty
-----------------------------------------------------------------------*/

HRESULT SerialManager::ReadAvailable(std::string& data)
{

	HRESULT success = CanProcess();
	if (FAILED(success)) return success;
	try
	{
		std::string szTemp;
		bool readReturn = m_serialBuffer.Read_Available(szTemp, m_dataRx);

		data = szTemp;
	}
	catch (...)
	{
		assert(0);
		success = E_FAIL;
	}
	return success;

}





