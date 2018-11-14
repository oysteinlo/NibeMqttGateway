#pragma once
#include "Printable.h"

#define REMOTEDEBUG
#ifdef REMOTEDEBUG
#define BUFFER_PRINT 300
#include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug


#define rdebugApln(fmt) if (Debug.isActive(Debug.ANY)) 			Debug.println(fmt)
#define rdebugpPln(fmt) if (Debug.isActive(Debug.PROFILER)) 	Debug.println(fmt)
#define rdebugpVln(fmt) if (Debug.isActive(Debug.VERBOSE)) 		Debug.println(fmt)
#define rdebugpDln(fmt) if (Debug.isActive(Debug.DEBUG)) 		Debug.println(fmt)
#define rdebugpIln(fmt) if (Debug.isActive(Debug.INFO)) 		Debug.println(fmt)
#define rdebugpWln(fmt) if (Debug.isActive(Debug.WARNING)) 		Debug.println(fmt)
#define rdebugpEln(fmt) if (Debug.isActive(Debug.ERROR)) 		Debug.println(fmt)

#define DEBUG_PRINT rdebugDln	// Telnet debug

extern RemoteDebug Debug;
#else
#define rdebugWln
#define rdebugDln
#define rdebugApln
#define rdebugpPln
#define rdebugpVln
#define rdebugpDln
#define rdebugpIln
#define rdebugpWln
#define rdebugpEln
#define DEBUG_PRINT
#endif

class DebugLog : public Printable
{
private:
	char _szName[20];

public:
	DebugLog(char szName[20]);
	~DebugLog();

	virtual size_t printTo(Print& p) const = 0;
};

