#include "Arduino.h"
#include "NibeMessage.h"
#include "NibeHeater.h"

/*
*	Frame format:
*	+----+----+----+-----+-----+----+----+-----+
*	| 5C | 00 | 20 | CMD | LEN |  DATA	 | CHK |
*	+----+----+----+-----+-----+----+----+-----+
*
*	Checksum: XOR
*
*	When valid data is received (checksum ok),
*	 ACK (0x06) should be sent to the heat pump.
*	When checksum mismatch,
*	 NAK (0x15) should be sent to the heat pump.
*
*	If heat pump does not receive acknowledge in certain time period,
*	pump will raise an alarm and alarm mode is activated.
*	Actions on alarm mode can be configured. The different alternatives
*	are that the Heat pump stops producing hot water (default setting)
*	and/or reduces the room temperature.
*/

NibeMessage::NibeMessage()
{
}

NibeMessage::NibeMessage(NibeHeater *pNibe)
{
	_pNibe = pNibe;
}

NibeMessage::~NibeMessage()
{
}

void NibeMessage::AddByte(byte b)
{
	_busTime = millis();

	if (_nByteIdx >= MAX_MSG_BUFFER)
	{
		_nByteIdx = MAX_MSG_BUFFER - 1;
		printf("Rx buffer empty\n");
	}

	if (_nByteIdx >= MAX_MSG_BUFFER)
	{
		_nByteIdx = MAX_MSG_BUFFER - 1;
		printf("Overrun\n");
	}
	_msg.buffer[_nByteIdx] = b;

	switch (_nByteIdx)
	{
	case Start:
	case Destination:
	case Source:
	case Command:
		if (b == 0x5c)
		{
			_nByteIdx = 0;
			_msg.msg.data[_nByteIdx] = b;
			_bInProgress = true;
			_bDataReady = false;
			memset(_msg.msg.data, 0, MAX_DATA_LENGTH);
		}
		//if (b == ACK)
		//{
		//	_pNibe->Request();
		//}
		break;
	case Length:
		break;
	default:
		if (_msg.msg.length + Length + 1 == _nByteIdx)
		{
			if (CheckSum() == b)
			{
				// If there is no parent (typically unittest we send ack directly
				if (_pNibe == nullptr)
				{
					Send(ACK);
				}
				else
				{
					_pNibe->HandleMessage(&_msg);
				}
				_bDataReady = true;
			}
			else
			{
				Send(NACK);
			}

			_nByteIdx = 0;
			_bInProgress = false;
		}
		break;
	}

	// Update byte counter only if frame in progress
	if (_bInProgress)
	{
		_nByteIdx++;
	}
}

void NibeMessage::SetInterFrameGap(unsigned long gap)
{
	_nInterFrameGap = gap;
}

void NibeMessage::Loop()
{
	if (_bInProgress)
	{
		if (idleTime() > _nInterFrameGap)
		{
			_bInProgress = false;
			_nByteIdx = 0;
		}
	}
}

byte NibeMessage::CheckSum(Message *pMsg)
{
	byte nCheckSum = 0;

	// Default == nullptr means that we use this object's message
	if (pMsg == nullptr)
	{
		pMsg = &_msg;
	}

	for (int i = 0; i < pMsg->msg.length + 3; i++)
	{
		nCheckSum ^= pMsg->buffer[i + 2];
	}

	// Special handling for 0x5c
	if (nCheckSum == 0x5c)
	{
		nCheckSum = 0xc5;
	}
	return nCheckSum;
}

void NibeMessage::SetReplyCallback(pFunc func)
{
	pSendReply = func;
}

unsigned long NibeMessage::idleTime()
{
	unsigned long dt = millis() - _busTime;

	// Handle wrap aropund
	if (millis() < _busTime)
	{
		dt = dt & 0x0FFFFFFFF;
	}
	return dt;
}

// void NibeMessage::SendAck()
// {
// 	if (pSendReply != nullptr)
// 	{
// 		pSendReply(ACK);
// 	}
// }

void NibeMessage::Send(Reply b)
{
	if (pSendReply != nullptr)
	{
		pSendReply(b);
	}
}

// void NibeMessage::SendNack()
// {
// 	if (pSendReply != nullptr)
// 	{
// 		pSendReply(NACK);
// 	}
// }

bool NibeMessage::SendMessage(Message *pMsg)
{
	// Return with false if there is sender
	if (pSendReply == nullptr)	return false;
	
	for (int i = 0; i <= pMsg->msg.length + 2; i++)
	{
		pSendReply(pMsg->buffer[i + 2]);	// When transmitting the two start bytes 0x5x and 0x00 are omitted
	}
	pSendReply(CheckSum(pMsg));	// Add checksum

	return true;
}

bool NibeMessage::IsDataReady()
{
	return _bDataReady;
}

Message* NibeMessage::GetMessage()
{
	return &_msg;
}
