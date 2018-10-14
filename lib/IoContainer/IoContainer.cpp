#include "IoContainer.h"
#include "RemoteDebug.h"

#define DEBUG_PRINT rdebugDln	// Telnet debug
extern RemoteDebug Debug;

IoContainer::IoContainer(const char *szName, IoElement_t *pIo, size_t size)
{
	strcpy(_szName, szName);
	_pIo = pIo;
	_size = size;

	init();
}


IoContainer::~IoContainer()
{
}

void IoContainer::PublishFuncPtr(pPublish pPub)
{
	_pPub = pPub;
}

void IoContainer::init()
{
	for (int i = 0; i < _size; i++)
	{
		_pIo[i].ulUpdateTime = millis();

		if (_pIo[i].dataType == eText)
		{
			_pIo[i].ioVal.pSzVal = new char(TEXT_IO_MAX);
		}
	}
}

void IoContainer::loop()
{
	cyclicPublish();
}

void IoContainer::cyclicPublish()
{
	for (int i = 0; i < _size; i++)
	{
		if (_pIo[i].bActive)
		{
			if (_pIo[i].ulPublishInterval > 0)
			{
				if (millis() - _pIo[i].ulPublishTime > _pIo[i].ulPublishInterval)
				{
					Publish(i, true);
				}
			}
		}
	}
}

bool IoContainer::Publish(int idx, bool bForce)
{
	bool bOk = false;
	IoElement *pIo = GetIoElement(idx);

	if (_pPub != nullptr && pIo != nullptr)
	{
		if (pIo->eIoDir != W && (!IsPublished(pIo) || bForce))
		{
			char topic[TOPIC_SIZE] = { 0 };
			if (GetTopic(idx, topic))
			{
				char val[VALUE_SIZE] = { 0 }; //TODO check length
				if (GetSzValue(idx, val))
				{
					if (_pPub(topic, val))
					{
						// Special handling for text
						if (pIo->dataType == eText)
						{
							strcpy(pIo->pubIoVal.pSzVal, pIo->ioVal.pSzVal);
						}
						// Otherwise we just copy the union IoVal
						else
						{
							pIo->pubIoVal = pIo->ioVal;	// Remember last published IoVal
						}
						pIo->ulPublishTime = millis();
						bOk = true;
					}
				}
			}
		}
	}
	return bOk;
}

bool IoContainer::IsPublished(IoElement *pIo)
{
	bool bPublished = false;

	switch (pIo->dataType)
	{
	case eBool:
		bPublished = pIo->ioVal.bVal == pIo->pubIoVal.bVal;
		break;
	case eS8:
		bPublished = pIo->ioVal.i8Val == pIo->pubIoVal.i8Val;
		break;
	case eS16:
		bPublished = pIo->ioVal.i16Val == pIo->pubIoVal.i16Val;
		break;
	case eS32:
		bPublished = pIo->ioVal.i32Val == pIo->pubIoVal.i32Val;
		break;
	case eU8:
		bPublished = pIo->ioVal.u8Val == pIo->pubIoVal.u8Val;
		break;
	case eU16:
		bPublished = pIo->ioVal.u16Val == pIo->pubIoVal.u16Val;
		break;
	case eU32:
		bPublished = pIo->ioVal.u32Val == pIo->pubIoVal.u32Val;
		break;
	case eFloat:
		bPublished = fabs(pIo->ioVal.fVal - pIo->pubIoVal.fVal) < pIo->fPublishDeadband;
		break;
		bPublished = strcmp(pIo->ioVal.pSzVal, pIo->pubIoVal.pSzVal) == 0;
	default:
		break;
	}

	return bPublished;
}

bool IoContainer::SetIoVal(int idx, float fVal)
{
	IoVal io;
	io.fVal = fVal;

	return SetIoVal(idx, io);
}

bool IoContainer::SetIoVal(int idx, int iVal)
{
	IoVal io;
	io.i32Val = iVal;

	return SetIoVal(idx, io);
}

bool IoContainer::SetIoVal(int idx, bool bVal)
{
	IoVal io;
	io.bVal = bVal;

	return SetIoVal(idx, io);
}

bool IoContainer::SetIoVal(int idx, IoVal io)
{
	bool bFound = false;

	if (idx < _size && idx >= 0)
	{
		_pIo[idx].ioVal = io;
		_pIo[idx].ulUpdateTime = millis();
		_pIo[idx].bActive = true;
		bFound = Publish(idx);
	}
	return bFound;
}

bool IoContainer::SetIoVal(int idx, char *pVal, size_t length)
{
	bool bFound = false;

	if (idx < _size && idx >= 0)
	{
		IoVal *pIo = (IoVal*)pVal;
		
		if (IsLegal(idx, pIo))
		{
			memcpy(&_pIo[idx].ioVal, pVal, length);
			_pIo[idx].ulUpdateTime = millis();
			_pIo[idx].bActive = true;
			bFound = Publish(idx);
		}
	}
	return bFound;
}


bool IoContainer::IsLegal(int idx, IoVal *pIo)
{
	bool bLegal = true;

	switch (_pIo[idx].type)
	{
		case eTemperature:
		if (pIo->i16Val < -50  || pIo->i16Val > 200)
		{
			bLegal = false;
		}
		
		break;

		default:
		bLegal = true;
		break;
	}
	return bLegal;
}

bool IoContainer::SetIoSzVal(int idx, char *pVal, size_t length)
{
	bool bFound = false;

	if (idx < _size)
	{
		bFound = SetIoSzVal(&_pIo[idx], pVal, length);
	}
	return bFound;
}

bool IoContainer::SetIoSzVal(char *pTag, char *pVal, size_t length)
{
	bool bUpdated = false;

	IoElement *pIoEl = GetIoElement(pTag);
	if (pIoEl != nullptr)
	{
		pVal[length] = '\0';
		bUpdated = SetIoSzVal(pIoEl, pVal, length);
	}

	return bUpdated;
}

bool IoContainer::SetIoSzVal(IoElement *pIoEl, char *pVal, size_t length)
{
	bool bOk = true;

	switch (pIoEl->dataType)
	{
	case eBool:
		pIoEl->ioVal.bVal = atoi((char*)pVal);
		//printf("%s -> %d\n", pIoEl->szTag, pIoEl->ioVal.bVal);
		bOk = true;
		break;
	case eS8:
		pIoEl->ioVal.i8Val = (int8_t)atoi((char*)pVal);
		//printf("%s -> %d\n", pIoEl->szTag, pIoEl->ioVal.i8Val);
		bOk = true;
		break;
	case eS16:
		pIoEl->ioVal.i16Val = (int16_t)atoi((char*)pVal);
		//printf("%s -> %d\n", pIoEl->szTag, pIoEl->ioVal.i16Val);
		bOk = true;
		break;
	case eS32:
		pIoEl->ioVal.i32Val = atoi((char*)pVal);
		//printf("%s -> %d\n", pIoEl->szTag, pIoEl->ioVal.i32Val);
		bOk = true;
		break;
	case eU8:
		pIoEl->ioVal.u8Val = (uint8_t)atol((char*)pVal);
		//printf("%s -> %d\n", pIoEl->szTag, pIoEl->ioVal.u8Val);
		bOk = true;
	case eU16:
		pIoEl->ioVal.u16Val = (uint16_t)atol((char*)pVal);
		//printf("%s -> %d\n", pIoEl->szTag, pIoEl->ioVal.u16Val);
		bOk = true;
	case eU32:
		pIoEl->ioVal.u32Val = atol((char*)pVal);
		//printf("%s -> %d\n", pIoEl->szTag, pIoEl->ioVal.u32Val);
		bOk = true;
		break;
	case eFloat:
		//printf("%s -> %f\n", pIoEl->szTag, pIoEl->ioVal.fVal);
		pIoEl->ioVal.fVal = (float)atof((char*)pVal);
		bOk = true;
		break;
	case eText:
		if (sizeof(pIoEl->ioVal.pSzVal) > length)
		{
			//printf("%s -> %s\n", pIoEl->szTag, pIoEl->ioVal.szVal);
			strcpy(pIoEl->ioVal.pSzVal, (char*)pVal);
			bOk = true;
		}
		break;
	default:
		bOk = false;
		break;
	}

	return bOk;
}

IoElement* IoContainer::GetIoElement(char *pTag)
{
	IoElement *pRetVal = nullptr;

	for (byte i = 0; i < _size; i++)
	{
		char topic[TOPIC_SIZE];
		GetTopic(i, topic);
		if (strcmp(pTag, topic) == 0)
		{
			pRetVal = &_pIo[i];
			//printf(_pIo[i].ioVal.szVal);
			break;
		}
	}

	return pRetVal;
}

int IoContainer::GetIoIndex(unsigned int id)
{
	int nRetval = -1;

	for (int i = 0; i < _size; i++)
	{
		if (_pIo[i].nIdentifer == id)
		{
			nRetval = i;
			break;
		}
	}

	return nRetval;
}

size_t IoContainer::GetIoSize(int idx)
{
	size_t size = 0;
	switch (_pIo[idx].dataType)
	{
	case eBool:
	case eS8:
	case eU8:
		size = 1;
		break;
	case eS16:
	case eU16:
		size = 2;
		break;
	case eS32:
	case eU32:
		size = 4;
		break;
	default:
		size = 0;
		break;
	}
	return size;
}

IoElement* IoContainer::GetIoElement(int idx)
{
	IoElement *pRetVal = nullptr;

	if (idx < _size && idx >= 0)
	{
		pRetVal = &_pIo[idx];
	}

	return pRetVal;
}

IoVal IoContainer::GetIoVal(int idx)
{
	return _pIo[idx].ioVal;
}

char* IoContainer::GetName()
{
	return _szName;
}

bool IoContainer::GetSzValue(int idx, char *pszValue)
{
	bool bFound = true;

	switch (_pIo[idx].dataType)
	{
	case eBool:
		sprintf(pszValue, "%d", (_pIo[idx].ioVal.bVal));
		break;
	case eS8:
		if (_pIo[idx].nfactor > 0)
		{
			int val1 = _pIo[idx].ioVal.i8Val / _pIo[idx].nfactor;
			int val2 = abs(_pIo[idx].ioVal.i8Val % _pIo[idx].nfactor);
			sprintf(pszValue, "%d.%d", val1, val2);
		}
		else
		{
			sprintf(pszValue, "%d", _pIo[idx].ioVal.i8Val);
		}
		break;
	case eS16:
		if (_pIo[idx].nfactor > 0)
		{
			int val1 = _pIo[idx].ioVal.i16Val / _pIo[idx].nfactor;
			int val2 = abs(_pIo[idx].ioVal.i16Val % _pIo[idx].nfactor);
			sprintf(pszValue, "%d.%d", val1, val2);
		}
		else
		{
			sprintf(pszValue, "%d", _pIo[idx].ioVal.i16Val);
		}
		break;
	case eS32:
		if (_pIo[idx].nfactor > 0)
		{
			int val1 = _pIo[idx].ioVal.i32Val / _pIo[idx].nfactor;
			int val2 = abs(_pIo[idx].ioVal.i32Val % _pIo[idx].nfactor);
			sprintf(pszValue, "%d.%d", val1, val2);
		}
		else
		{
			sprintf(pszValue, "%d", _pIo[idx].ioVal.i32Val);
		}
		break;
	case eU8:
		if (_pIo[idx].nfactor > 0)
		{
			int val1 = _pIo[idx].ioVal.u8Val / _pIo[idx].nfactor;
			int val2 = _pIo[idx].ioVal.u8Val % _pIo[idx].nfactor;
			sprintf(pszValue, "%d.%d", val1, val2);
		}
		else
		{
			sprintf(pszValue, "%d", _pIo[idx].ioVal.u8Val);
		}
		break;
	case eU16:
		if (_pIo[idx].nfactor > 0)
		{
			int val1 = _pIo[idx].ioVal.u16Val / _pIo[idx].nfactor;
			int val2 = _pIo[idx].ioVal.u16Val % _pIo[idx].nfactor;
			sprintf(pszValue, "%d.%d", val1, val2);
		}
		else
		{
			sprintf(pszValue, "%d", _pIo[idx].ioVal.u16Val);
		}
		break;
	case eU32:
		if (_pIo[idx].nfactor > 0)
		{
			int val1 = _pIo[idx].ioVal.u32Val / _pIo[idx].nfactor;
			int val2 = _pIo[idx].ioVal.u32Val % _pIo[idx].nfactor;
			sprintf(pszValue, "%d.%d", val1, val2);
		}
		else
		{
			sprintf(pszValue, "%d", _pIo[idx].ioVal.u32Val);
		}
		break;
	case eFloat:
#ifdef WIN32
		sprintf(pszValue, "%f", (_pIo[idx].ioVal.fVal / _pIo[idx].nfactor));
#else
		char str_temp[32];
		dtostrf(_pIo[idx].ioVal.fVal, 4, 2, str_temp);
		sprintf(pszValue, "%s", str_temp);
#endif
		break;
	default:
		bFound = false;
	}
	return bFound;
}

bool IoContainer::GetTopic(int idx, char *pszValue)
{
	pszValue[0] = '\0';
	strcat(pszValue, _szName);
	strcat(pszValue, "/");
	strcat(pszValue, _pIo[idx].szTag);
	pszValue[strlen(pszValue) + 1] = '\0';

	return true;
}

int IoContainer::GetExpiredIoElement(IoDirection eIoDir)
{
	int nRetval = -1;

	for (int i = 0; i < _size; i++)
	{
		if (_pIo[i].eIoDir == eIoDir)
		{
			if (_pIo[i].ulPublishInterval > 0 && (millis() - _pIo[i].ulUpdateTime > _pIo[i].ulPublishInterval))
			{
				nRetval = i;
				break;
			}
		}
	}

	return nRetval;
}
