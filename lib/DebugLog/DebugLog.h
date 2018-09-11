
class DebugLog
{
private:
	char _szName[20];

public:
	DebugLog(char szName[20]);
	~DebugLog();

	void WriteHexString(const byte *pByte, byte length);
};

