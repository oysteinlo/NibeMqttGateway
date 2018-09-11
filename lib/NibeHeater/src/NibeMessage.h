#pragma once
#include "Arduino.h"


#define MAX_DATA_LENGTH 100
#define MAX_MSG_BUFFER MAX_DATA_LENGTH + 5

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

class NibeMessage
{
private:
	int _nByteIdx = 0;
	Message _msg;// = { 0 };
	NibeHeater *_pNibe = nullptr;

	unsigned long _busTime = 0;
	unsigned long _nInterFrameGap = 100;

	bool _bDataReady = false;;
	bool _bInProgress = false;;

	pFunc pSendReply;

public:
	NibeMessage();
	NibeMessage(NibeHeater * pNibe);
	~NibeMessage();

	void AddByte(byte b);

	void SetInterFrameGap(unsigned long gap);

	void Loop();

	void SetReplyCallback(pFunc func);

	unsigned long idleTime();

	//void SendAck();

	void Send(Reply b);
	

	//void SendNack();

	bool SendMessage(Message * pMsg);

	bool IsDataReady();

	Message * GetMessage();

	byte CheckSum(Message *pMsg = nullptr);

};

