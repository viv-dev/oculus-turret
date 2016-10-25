#include <Windows.h>
#include <string>

using namespace std;

class SerialBuffer
{
public:

	SerialBuffer();
	virtual ~SerialBuffer();

	//Functions
	void Init();

	void AddData(char ch);
	void AddData(string& data);
	void AddData(string& data, unsigned int length);
	void AddData(char* strData, unsigned int length);

	long Read(string& data, unsigned int n);
	bool Read(string& data, char delimiter, unsigned int& bytesRead);
	bool Read(string& data);

	void Flush();

	unsigned int GetSize()
	{
		LockBuffer();
		unsigned size = m_buffer.size();
		UnLockBuffer();
		return size;
	}
	
	bool IsEmpty()
	{
		LockBuffer();
		bool empty = m_buffer.size() == 0;
		UnLockBuffer();
		return empty;
	}



private:
	//Critical Section Handling
	inline void LockBuffer()
	{
		EnterCriticalSection(&m_lock);
	}
	inline void UnLockBuffer()
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

	//Variables
	string              m_buffer;
	CRITICAL_SECTION    m_lock;

	const unsigned int MAX_SIZE = 256;

	long m_currentPos;
	long m_bytesUnRead;


};