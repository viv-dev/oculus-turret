// SerialBuffer.cpp: implementation of the SerialBuffer class.
//
//////////////////////////////////////////////////////////////////////

#include <assert.h>
#include "SerialBuffer.h"


//TO DO - implement a way to clear the string as data is read otu
//TO DO - stop the 

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
	m_currentPos = 0;
	m_bytesUnRead = 0;
	m_buffer.erase();
	InitLock();
}

void SerialBuffer::AddData(char c)
{
	LockBuffer();

	m_buffer += c;
	m_bytesUnRead += 1;

	UnLockBuffer();
}

void SerialBuffer::AddData(string& data, unsigned int length)
{
	LockBuffer();

	m_buffer.append(data.c_str(), length);
	m_bytesUnRead += length;

	UnLockBuffer();

}

void SerialBuffer::AddData(char* data, unsigned int length)
{
	LockBuffer();

	assert(data != NULL);
	m_buffer.append(data, length);
	m_bytesUnRead += length;

	UnLockBuffer();
}

void SerialBuffer::AddData(string& data)
{
	LockBuffer();

	if (m_buffer.size() < MAX_SIZE)
	{
		m_buffer += data;
		m_bytesUnRead += data.size();
	}

	UnLockBuffer();

}

long SerialBuffer::Read(string& data, unsigned int n)
{
	LockBuffer();

	long tempCount = min(n, m_bytesUnRead);

	data.append(m_buffer, m_currentPos, tempCount);

	m_currentPos += tempCount;
	m_bytesUnRead -= tempCount;

	UnLockBuffer();

	return tempCount;
}

bool SerialBuffer::Read(string& data)
{
	LockBuffer();

	data += m_buffer;
	Flush();
	
	UnLockBuffer();

	return (data.size() > 0);
}

bool SerialBuffer::Read(string& data, char delimiter, unsigned int& bytesRead)
{
	LockBuffer();

	bytesRead = 0;
	bool found = false;

	if (m_bytesUnRead > 0)
	{

		int actualSize = m_buffer.size();
		int incrementPos = 0;

		for (int i = m_currentPos; i < actualSize; ++i)
		{
			data += m_buffer[i];
			m_bytesUnRead -= 1;

			if (m_buffer[i] == delimiter)
			{
				incrementPos++;
				found = true;
				break;
			}

			incrementPos++;
		}

		m_currentPos += incrementPos;
	}

	UnLockBuffer();

	return found;
}

void SerialBuffer::Flush()
{
	LockBuffer();

	m_buffer.erase();
	m_bytesUnRead = 0;
	m_currentPos = 0;

	UnLockBuffer();
}