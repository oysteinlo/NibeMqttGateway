#include "Printable.h"
#include "RemoteDebug.h"

#define rdebugApln(fmt) if (Debug.isActive(Debug.ANY)) 			Debug.println(fmt)
#define rdebugpPln(fmt) if (Debug.isActive(Debug.PROFILER)) 	Debug.println(fmt)
#define rdebugpVln(fmt) if (Debug.isActive(Debug.VERBOSE)) 		Debug.println(fmt)
#define rdebugpDln(fmt) if (Debug.isActive(Debug.DEBUG)) 		Debug.println(fmt)
#define rdebugpIln(fmt) if (Debug.isActive(Debug.INFO)) 		Debug.println(fmt)
#define rdebugpWln(fmt) if (Debug.isActive(Debug.WARNING)) 		Debug.println(fmt)
#define rdebugpEln(fmt) if (Debug.isActive(Debug.ERROR)) 		Debug.println(fmt)

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

