#include "Arduino.h"
#include "DebugLog.h"

DebugLog::DebugLog(char szName[20])
{
}


DebugLog::~DebugLog()
{
}

size_t printTo(Print& p)
{
    //TODO
    return 0;
}

#ifdef DEBUG
RemoteDebug Debug;    
#endif
