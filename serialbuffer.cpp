// SerialBuffer.cpp: implementation of the SerialBuffer class.
//
//////////////////////////////////////////////////////////////////////

#include <assert.h>
#include "SerialBuffer.h"

SerialBuffer::SerialBuffer()
{
	Init();
}

SerialBuffer::~SerialBuffer()
{
	DeleteCriticalSection(&m_lock);
}

void SerialBuffer::Init()
{
	InitializeCriticalSection(&m_lock);

	m_lockAlways = true;
	m_currentPos = 0;
	m_bytesUnRead = 0;
	m_buffer.erase();
}

void SerialBuffer::AddData(char c)
{
	m_buffer += c;
	m_bytesUnRead += 1;

}

void SerialBuffer::AddData(string& data, int length)
{
	m_buffer.append(data.c_str(), length);
	m_bytesUnRead += length;

}

void SerialBuffer::AddData(char* strData, int length)
{
	assert(strData != NULL);
	m_buffer.append(strData, length);
	m_bytesUnRead += length;

}

void SerialBuffer::AddData(string& data)
{
	m_buffer += data;
	m_bytesUnRead += data.size();

}

void SerialBuffer::Flush()
{
	LockBuffer();

	m_buffer.erase();
	m_bytesUnRead = 0;
	m_currentPos = 0;

	UnLockBuffer();
}

long SerialBuffer::Read_N(string& data, long n, HANDLE& hEventToReset)
{
	assert(hEventToReset != INVALID_HANDLE_VALUE);

	LockBuffer();

	long tempCount = min(n, m_bytesUnRead);
	long actualSize = GetSize();

	data.append(m_buffer, m_currentPos, tempCount);

	m_currentPos += tempCount;
	m_bytesUnRead -= tempCount;

	if (m_bytesUnRead == 0)
		ClearAndReset(hEventToReset);

	UnLockBuffer();

	return tempCount;
}

bool SerialBuffer::Read_Available(string& data, HANDLE& hEventToReset)
{
	LockBuffer();

	data += m_buffer;
	ClearAndReset(hEventToReset);

	UnLockBuffer();

	return (data.size() > 0);
}


void SerialBuffer::ClearAndReset(HANDLE& hEventToReset)
{
	m_buffer.erase();
	m_bytesUnRead = 0;
	m_currentPos = 0;
	::ResetEvent(hEventToReset);

}

bool SerialBuffer::Read_Upto(string& data, char terminate, long& bytesRead, HANDLE& hEventToReset)
{
	LockBuffer();

	bytesRead = 0;
	bool found = false;

	//If there are bytes unread
	if (m_bytesUnRead > 0)
	{

		int actualSize = GetSize();
		int incrementPos = 0;

		for (int i = m_currentPos; i < actualSize; ++i)
		{
			data += m_buffer[i];
			m_bytesUnRead -= 1;

			if (m_buffer[i] == terminate)
			{
				incrementPos++;
				found = true;
				break;
			}

			incrementPos++;
		}

		m_currentPos += incrementPos;

		if (m_bytesUnRead == 0)
			ClearAndReset(hEventToReset);
	}

	UnLockBuffer();

	return found;
}