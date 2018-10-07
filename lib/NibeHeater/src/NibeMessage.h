#pragma once
#include "Arduino.h"
#include "Printable.h"

#define MAX_DATA_LENGTH 100
#define MAX_MSG_BUFFER (MAX_DATA_LENGTH + 5)
#define PRINT_BUF_SIZE (MAX_MSG_BUFFER * 3)

typedef enum
{
	Start,
	Destination,
	Source,
	Command,
	Length,
	Data,
} ByteIdentifier;

typedef struct
{
	byte start;
	byte something1;
	byte nodeid;
	byte command;
	byte length;
	byte data[MAX_DATA_LENGTH];
} Message_t;

union Message 
{
	Message_t msg;
	byte buffer[MAX_MSG_BUFFER];
};


typedef enum 
{
	NONE = -1,
	ENQ = 0x5,
	ACK = 0x6,
	NACK = 0x15,
} Reply;

typedef bool(*pFunc) (byte);
class NibeHeater;

class NibeMessage : public Printable
{
private:
	char _name[10];
	int _nByteIdx = 0;
	Message _msg;// = { 0 };
	NibeHeater *_pNibe = nullptr;

	unsigned long _busTime = 0;
	unsigned long _nInterFrameGap = 100;

	bool _bDataReady = false;
	bool _bInProgress = false;

	char _printBuffer[PRINT_BUF_SIZE];
	pFunc pSendReply;

public:
	NibeMessage();
	NibeMessage(NibeHeater * pNibe, char* pName);

	void AddByte(byte b);

	void SetInterFrameGap(unsigned long gap);

	void Loop();

	void SetReplyCallback(pFunc func);

	unsigned long idleTime();

	void Send(Reply b);
	
	bool SendMessage();

	bool IsDataReady();

	Message * GetMessage();

	byte CheckSum(Message *pMsg = nullptr);

	size_t printTo(Print& p) const override;

	char* LogMessage();

};

