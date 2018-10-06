#pragma once
#define DllExport   __declspec( dllexport )  

#include "Arduino.h"
#include "NibeMessage.h"
#include "NibeHeater.h"
#include "IoContainer.h"


typedef enum
{
	DATABLOCK = 0x68,
	READREQ = 0x69,
	READDATA = 0x6a,
	WRITEREQ = 0x6b
} CommandSpecifier;

typedef void(*pDebugFunc) (char*);

class NibeHeater
{

private:
	NibeMessage *_msgHandler = nullptr;
	IoContainer *_ioContainer = nullptr;
	pDebugFunc _debugFunc = nullptr;

	int _requestIo;
	Message _reqMsg;

public:
	NibeHeater();
	NibeHeater(NibeMessage **ppMsg);
	NibeHeater(NibeMessage **pMsg, IoContainer *pIoContainer);
	void AttachDebug(pDebugFunc debugfunc);
	void Loop();
	bool HandleMessage(Message * pMsg);
	bool Request();
	bool ReadRequest(int idx, Message * pMsg);
	bool WriteRequest(int idx, Message * pMsg);
};

