#include "Arduino.h"
#include "NibeMessage.h"
#include "NibeHeater.h"
#include "RemoteDebug.h"

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

#define DEBUG_PRINT rdebugDln	// Telnet debug
extern RemoteDebug Debug;

NibeMessage::NibeMessage() : Printable()
{
}

NibeMessage::NibeMessage(NibeHeater *pNibe, char *pName) : Printable()
{
	_pNibe = pNibe;
	strcpy(_name, pName);
}

void NibeMessage::AddByte(byte b)
{
	_busTime = millis();

	if (_nByteIdx >= MAX_MSG_BUFFER)
	{
		_nByteIdx = MAX_MSG_BUFFER - 1;
		DEBUG_PRINT("Rx buffer empty");
	}

	if (_nByteIdx >= MAX_MSG_BUFFER)
	{
		_nByteIdx = MAX_MSG_BUFFER - 1;
		DEBUG_PRINT("Overrun");
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
					Debug.println(*this);
					_pNibe->HandleMessage(&_msg);
				}
				_bDataReady = true;
			}
			else
			{
				DEBUG_PRINT("Checksum error: %x", b);
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

void NibeMessage::Send(Reply b)
{
	if (pSendReply != nullptr)
	{
		DEBUG_PRINT("Tx %d", b);
		pSendReply(b);
	}
}

bool NibeMessage::SendMessage()
{
	// Return with false if there is sender
	if (pSendReply == nullptr)	return false;
	
	for (int i = 0; i <= _msg.msg.length + 2; i++)
	{
		pSendReply(_msg.buffer[i + 2]);	// When transmitting the two start bytes 0x5c and 0x00 are omitted
	}
	pSendReply(CheckSum(&_msg));	// Add checksum
	Debug.println(*this);

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

size_t NibeMessage::printTo(Print& p) const
{
	size_t n = 0;
	n += p.print(_name);
    for(int i = 0; i < _msg.msg.length + Data + 1; i++) {	// Control data + CRC
        n += p.print(' ');
        n += p.print(_msg.buffer[i], HEX);
    }
    return n;
} 

#ifdef REMOVE
char* NibeMessage::LogMessage()
{
	int idx = 5; //"Data: "
	sprintf (_printBuffer, "Data:");
    for(int i = 0; i < _msg.msg.length + Data + 1; i++) {	// Control data + CRC
   		if (idx + 4 < PRINT_BUF_SIZE) {
			sprintf (_printBuffer + idx, " %02x", _msg.buffer[i]);
			idx = idx + 3;
    	}
		else{
			*(_printBuffer + idx + 1) = 0; // Nullterm
		}
	}
	*(_printBuffer + idx + 1) = 0; // Nullterm
    return _printBuffer;
}
#endif