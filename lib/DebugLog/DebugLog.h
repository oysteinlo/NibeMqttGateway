#include "Printable.h"

class DebugLog : public Printable
{
private:
	char _szName[20];

public:
	DebugLog(char szName[20]);
	~DebugLog();

	void WriteHexString(const byte *pByte, byte length);
	virtual size_t printTo(Print& p) const = 0;
};

