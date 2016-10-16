#include <Windows.h>
#include <string>

using namespace std;

class SerialBuffer
{
public:
	//Critical Section Handling
	inline void LockBuffer()
	{
		::EnterCriticalSection(&m_lock);
	}
	inline void UnLockBuffer()
	{
		::LeaveCriticalSection(&m_lock);
	}

	SerialBuffer();
	virtual ~SerialBuffer();

	//API For Accessing Serial Buffer
	void AddData(char ch);
	void AddData(string& data);
	void AddData(string& data, int length);
	void AddData(char* strData, int length);
	string GetData()
	{
		return m_buffer;
	}

	void        Flush();
	long        Read_N(string& data, long n, HANDLE& hEventToReset);
	bool        Read_Upto(string& data, char terminate, long& bytesRead, HANDLE& hEventToReset);
	bool        Read_Available(string& data, HANDLE& hEventToReset);
	inline long GetSize()
	{
		return m_buffer.size();
	}
	inline bool IsEmpty()
	{
		return m_buffer.size() == 0;
	}



private:
	//Variables
	string              m_buffer;
	CRITICAL_SECTION    m_lock;
	bool                m_lockAlways;
	long                m_currentPos;
	long                m_bytesUnRead;

	//Functions
	void    Init();
	void    ClearAndReset(HANDLE& hEventToReset);
};