/**
*
*	This application listening data from Nibe F1145/F1245 heat pumps (RS485 bus)
*	and send valid frames to configurable IP/port address by UDP packets.
*	Application also acknowledge the valid packets to heat pump.
*
*	Serial settings: 9600 baud, 8 bits, Parity: none, Stop bits 1
*
*	MODBUS module support should be turned ON from the heat pump.
*
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
*
*
*/
// NibeHeater.cpp : Defines the entry point for the console application.
//

#include "NibeHeater.h"


NibeHeater::NibeHeater()
{
	_msgHandler = new NibeMessage(this);
}

NibeHeater::NibeHeater(NibeMessage **ppMsg)
{
	_msgHandler = *ppMsg = new NibeMessage(this);
}

NibeHeater::NibeHeater(IoContainer *pIoContainer, NibeMessage **ppMsg)
{
	_ioContainer = pIoContainer;
	_msgHandler = *ppMsg = new NibeMessage(this);
}

void NibeHeater::AttachDebug(pDebugFunc debugfunc)
{
	_debugFunc = debugfunc;
}

void NibeHeater::Loop()
{
	_msgHandler->Loop();
}

//http://www.varmepumpsforum.com/vpforum/index.php?topic=39325.60

bool NibeHeater::HandleMessage(Message *pMsg)
{
	bool bOk = true;	// Return true of message is handled (set to false in default case)

	if (_debugFunc != nullptr)
	{
		int i = 0;
		char buff[100];
		for (i = 0; i<pMsg->msg.length; i++)
		{
			sprintf (buff +(i*3), "%x", pMsg->buffer[i]);
		}
		_debugFunc(buff);//("Testdebug"); //->WriteHexString(pMsg->buffer, pMsg->msg.length + 5);
	}

	switch (pMsg->msg.command)
	{
	case DATABLOCK:
	case READDATA:
	{
		_msgHandler->Send(ACK);

		const int datalength = 4;
		for (int i = 0; i < pMsg->msg.length; i += datalength)
		{
			#ifdef WIN32
			unsigned int adress = word(*(pMsg->msg.data + i), *(pMsg->msg.data + i + 1));
			#else
			unsigned int adress = word(*(pMsg->msg.data + i + 1), *(pMsg->msg.data + i));
			#endif
			
			char data[4] =
			{
				*(pMsg->msg.data + i + 2) ,
				*(pMsg->msg.data + i + 3)
			};

			int idx = _ioContainer->GetIoIndex(adress);
			size_t size = _ioContainer->GetIoSize(idx);
			if (size == 4)
			{
				i += datalength;
				adress = word(*(pMsg->msg.data + i), *(pMsg->msg.data + i + 1));
				if (adress == 0xffff)
				{
					data[2] = *(pMsg->msg.data + i + 2);
					data[3] = *(pMsg->msg.data + i + 3);
				}
			}
			_ioContainer->SetIoVal(idx, data, size);
		}
	}
	break;
	case READREQ:
		if (ReadRequest(_ioContainer->GetExpiredIoElement(R), &_reqMsg))
		{
			//_msgHandler->Send(ENQ);
			_msgHandler->SendMessage(&_reqMsg);
		}
		else
		{
			_msgHandler->Send(ACK);
		}
	break;
	case WRITEREQ:
		if (WriteRequest(_ioContainer->GetExpiredIoElement(W), &_reqMsg))
		{
			//_msgHandler->Send(ENQ);
			_msgHandler->SendMessage(&_reqMsg);
		}
		else
		{
			_msgHandler->Send(ACK);
		}
		break;
	default:
		_msgHandler->Send(ACK);	// This is an unknown message command, but we still send ack
		bOk = false;
	}

	return bOk;
}

bool NibeHeater::Request()
{
	return _msgHandler->SendMessage(&_reqMsg);
}

bool NibeHeater::ReadRequest(int idx, Message *pMsg)
{
	bool bOk = false;
	IoElement *pIo = _ioContainer->GetIoElement(idx);

	if (pIo != nullptr && pMsg != nullptr)
	{
		//C0 69 02 66 B8
		pMsg->msg.nodeid = 0xc0;
		pMsg->msg.command = READREQ;
		pMsg->msg.length = _ioContainer->GetIoSize(idx);
		//#ifdef WIN32
		pMsg->msg.data[0] = pIo->nIdentifer & 0x00ff;
		pMsg->msg.data[1] = pIo->nIdentifer >> 8;
		// #else
		// pMsg->msg.data[1] = pIo->nIdentifer & 0x00ff;
		// pMsg->msg.data[0] = pIo->nIdentifer >> 8;
		// #endif
		bOk = true;
	}
	
	return bOk;
}

bool NibeHeater::WriteRequest(int idx, Message *pMsg)
{
	bool bOk = false;
	IoElement *pIo = _ioContainer->GetIoElement(idx);

	if (pIo != nullptr)
	{
		size_t dataSize = _ioContainer->GetIoSize(idx);
		
		// C0 6B 06 66 B8 CE FF 00 00 42
		pMsg->msg.nodeid = 0xc0;
		pMsg->msg.command = WRITEREQ;
		pMsg->msg.length = 2 + dataSize;	// Adress (2) + data
		#ifdef WIN32
		pMsg->msg.data[0] = pIo->nIdentifer & 0x00ff;
		pMsg->msg.data[1] = pIo->nIdentifer >> 8;
		#else
		pMsg->msg.data[1] = pIo->nIdentifer & 0x00ff;
		pMsg->msg.data[0] = pIo->nIdentifer >> 8;
		#endif		memcpy(&pMsg->msg.data[2], &pIo->ioVal, dataSize);

		bOk = true;
	}

	return bOk;
}

