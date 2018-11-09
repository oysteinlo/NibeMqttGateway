#pragma once
#include "Arduino.h"

#define TAG_SIZE 32
#define NAME_SIZE 32
#define TOPIC_SIZE TAG_SIZE + NAME_SIZE + 2	// includes '/' and nullterm
#define VALUE_SIZE	16 
#define TEXT_IO_MAX 32

typedef enum IoDataType
{
	eUnknown,
	eBool,
	eS8,
	eU8,
	eS16,
	eU16,
	eS32,
	eU32,
	eFloat,
	eText,
} IoDataType_t;

typedef enum IoType
{
	eDefault,
	eAnalog,
	eNumOfIoTypes
} IoType_t;

typedef enum IoDirection
{
	R,
	W,
	RW,
} IoDirection_t;

typedef union IoVal
{
	bool bVal;
	int8_t i8Val;
	int16_t i16Val;
	int32_t i32Val;
	uint8_t u8Val;
	uint16_t u16Val;
	uint32_t u32Val;
	float fVal;
	char array[4];
	char *pSzVal;
} IoVal_t;

typedef struct IoElement
{
	char szTag[TAG_SIZE];
	uint16_t nIdentifer;
	IoDataType dataType;
	IoDirection eIoDir;
	IoType type;
	unsigned long ulPublishInterval;
	float fPublishDeadband;

	IoVal ioVal;
	IoVal pubIoVal;					// Last published 
	unsigned long ulPublishTime;	
	unsigned long ulUpdateTime;		// Sensor update time
	bool bTrig;
	bool bActive;
} IoElement_t;

typedef bool(*pPublish) (char*, char*);

class IoContainer
{
private:
	IoElement_t *_pIo;
	int _size;
	char _szName[16];
	char _szTag[32];
	pPublish _pPub;		// Pointer to publish function
	IoVal _errorVal[eNumOfIoTypes];
	

	bool SetIoSzVal(IoElement * pIoEl, char * pVal, unsigned int length);
	bool IsLegal(int idx, IoVal *pIo);


public:
	IoContainer(const char *szName, IoElement_t *pIo, size_t size);
	~IoContainer();

	void PublishFuncPtr(pPublish pPub);
	void init();
	bool Publish(int idx, bool bForce = false);
	bool IsPublished(IoElement *pIo);
	void loop();
	void cyclicPublish();

	bool SetIoVal(int idx, float fVal);
	bool SetIoVal(int idx, int iVal);
	bool SetIoVal(int idx, bool bVal);
	bool SetIoVal(int idx, IoVal io);
	bool SetIoVal(int idx, char * pVal, size_t length);
	bool SetIoVal(uint16_t adress, char * pVal, size_t length);
	bool SetIoSzVal(int idx, char * pVal, size_t length);
	bool SetIoSzVal(char *pTag, char *pVal, size_t length);
	void SetErrorVal(IoType_t type, IoVal val);
	
	int GetExpiredIoElement(IoDirection eIoDir);
	IoElement* GetIoElement(char *pTag);
	int GetIoIndex(unsigned int id);
	size_t GetIoSize(int idx);
	IoElement * GetIoElement(int idx);
	IoVal GetIoVal(int idx);
	char * GetName();
	bool GetSzValue(int idx, char * pszValue);
	bool GetTopic(int idx, char * pszValue);
};

