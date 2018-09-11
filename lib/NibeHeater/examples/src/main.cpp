#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "NibeHeater.h"

#include "IoContainer.h"

// Set LED_BUILTIN if it is not defined by Arduino framework
// #define LED_BUILTIN 13
//#define DEBUG_PRINT printf
#define DEBUG_PRINT


#define ENABLE_PIN 0 // GPIO_0 RS485 transceiver enablepinRS485 transceiver enablepin

const char *ssid = "WiFi2";
const char *password = "lobbenwifi";
const char *mqtt_server = "192.168.10.109";
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiServer telnetServer(23);
WiFiClient serverClient;

long lastMsg = 0;
char msg[50];
int value = 0;
unsigned long startTime = millis();
unsigned long lastReconnectAttempt = 0;


IoElement_t iopoints[] =
	{
		/* Tag								        Identifer,	Type	IoDir	Factor	Cyclic	Deadband*/
		/*00*/ {"Alarm", 							45001, 		eS16, 	R, 		0, 		60000, 	0.1f},
		/*01*/ {"AZ1-BT50 Room temp", 				41213, 		eS16, 	R, 		10, 	60000, 	0.1f},
		/*02*/ {"BT1 Outdoor Temperature", 			40004, 		eS16, 	R, 		10, 	60000, 	0.1f},
		/*03*/ {"Hot water temperature", 			40014, 		eS16, 	R, 		10, 	60000, 	0.1f},
		/*04*/ {"Compressor starts EB100-EP14", 	43416, 		eS32, 	R, 		0, 		60000, 	0.1f},
		/*05*/ {"Compressor State EP14", 			43427, 		eU8, 	R, 		0, 		60000, 	0.1f},
		/*06*/ {"DegreeMinute32", 					40940, 		eS32, 	R, 		10, 	60000, 	0.1f},
		/*07*/ {"Brine in temperature", 			40015, 		eS16, 	R, 		10, 	60000, 	0.1f},
		/*08*/ {"Brine out temperature", 			40016, 		eS16, 	R, 		10, 	60000, 	0.1f},
		/*09*/ {"Condensor out temperature", 		40017, 		eS16, 	R, 		10, 	60000, 	0.1f},
		/*10*/ {"Hot gas temperature", 				40018, 		eS16, 	R, 		10, 	60000, 	0.1f},
		/*11*/ {"Suction temperature", 				40022, 		eS16, 	R, 		10, 	60000, 	0.1f},
		/*12*/ {"Return temperature", 				40012,		eS16, 	R, 		10, 	60000, 	0.1f},
		/*13*/ {"EB100-EP14 Prio", 					44243, 		eU8, 	R, 		0, 		60000,	0.1f},
		/*14*/ {"Tot. HW op.time compr", 			43424, 		eS32, 	R, 		0, 		60000, 	0.1f},
		/*15*/ {"Tot.op.time compr", 				43420, 		eS32, 	R, 		0, 		60000, 	0.1f},
		/*16*/ {"Software version", 				43001, 		eU16, 	R, 		0, 		60000, 	0.1f},
		/*17*/ {"Holiday activated", 				48043, 		eU8, 	R, 		0, 		60000, 	0.1f},
		/*18*/ {"BT50 Room Temp S1", 				40033, 		eS16, 	R, 		10, 	60000,	0.1f},
		/*19*/ {"BT50 Room Temp S1 Average", 		40195, 		eS16, 	R, 		10, 	60000,	0.1f},
	};
const byte numIoPoints = sizeof(iopoints) / sizeof(IoElement_t);
IoContainer io("Nibe", iopoints, numIoPoints);

NibeMessage *pNibeMsgHandler;
NibeHeater nibeHandler(&io, &pNibeMsgHandler);

bool SendReply(byte b)
{
	digitalWrite(ENABLE_PIN, HIGH);
	delay(1);
	Serial.write(b);
	Serial.flush();
	delay(1);
	digitalWrite(ENABLE_PIN, LOW);
	return true;
}

void DebugPrint(const char *c)
{
	if (serverClient && serverClient.connected())
	{ // send data to Client
		serverClient.print(c);
		serverClient.println();
	}
}

void setup_wifi()
{

	delay(10);
	
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		//Serial.print(".");
	}

	DEBUG_PRINT("IP: %s", WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
	io.SetIoSzVal(topic, payload, length);
}

bool publish(char *topic, char *value)
{
	DEBUG_PRINT ("Publish\n");
	return mqttClient.publish(topic, value);
}

void subscribe()
{
	for (int i = 0; i < numIoPoints; i++)
	{
		char topic[TOPIC_SIZE];
		io.GetTopic(i, topic);

		if (iopoints[i].eIoDir != R)
		{
			mqttClient.subscribe(topic);
		}
	}
}

boolean reconnect()
{
	DEBUG_PRINT ("MQTT connect\n");
	//    if (mqttClient.connect("NibeClient", "openhabian", "2048ping"))
	if (mqttClient.connect("NibeClient", "openhabian", "2048ping"))
	{
		// Once connected, publish an announcement...
		char buf[20];
		//sprintf(buf, WiFi.localIP());
		//mqttClient.publish("Nibe/Client", WiFi.localIP().toString());
		// ... and resubscribe
		subscribe();
		DEBUG_PRINT ("Connected\n");
	}
	return mqttClient.connected();
}

unsigned int maxLoopTime = 0;
unsigned int stamp = 0;
void setup()
{
	unsigned long now = 0;
	

	pinMode(ENABLE_PIN, OUTPUT);

	Serial.begin(9600);
	nibeHandler.Init();
	pNibeMsgHandler->SetReplyCallback(SendReply);

	setup_wifi();
	
	mqttClient.setServer(mqtt_server, 1883);
	mqttClient.setCallback(callback);

	telnetServer.begin();
	telnetServer.setNoDelay(true);

	io.PublishFuncPtr(publish);

	now = millis();
	//while (millis() - now < 3000) {}	// Wait for 10 sec
	DEBUG_PRINT ("Setup\n");

	stamp = millis();
}

byte readreq[] = {0x5c, 0x0, 0x20, 0x69, 0x0, 0x49};
byte writereq[] = {0x5c, 0x0, 0x20, 0x6b, 0x0, 0x4b};

//byte testdata[] = { 0x5c, 0x0, 0x20, 0x6a, 0x6, 0xf9, 0xa7, 0x20, 0x1f, 0x0, 0x0, 0x2d };
byte testdata[] = {0x5c, 0x0, 0x20, 0x68, 0x50, // DATA
				   0xc9, 0xaf, 0x0, 0x0,		//45001	0xAFC9	Alarm
				   0xec, 0x9f, 0x76, 0x1,		//40940 0x9FEC	Degree Minutes (32 bit)
				   0xff, 0xff, 0x0, 0x1,
				   0x98, 0xa9, 0xe, 0x2c, //43416 0xA998	Compressor starts EB100-EP14
				   0xff, 0xff, 0x0, 0x0,
				   0xfd, 0xa0, 0x0, 0x80, //41213 0xA0FD	AZ1-BT50 Room temp [°C]
				   0xfc, 0xa0, 0x0, 0x80, //41212 0xA0FC	AZ2-BT50 Room temp [°C]
				   0xf7, 0x9c, 0x0, 0x80, //40183 0x9CF7	AZ30-BT23 Outdoor temp. ERS [°C]
				   0xfb, 0xa0, 0x0, 0x80, //41211	0xA0FB	AZ3-BT50 Room temp [°C]
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0xff, 0xff, 0x0, 0x0,
				   0x59};



void loop()
{
	if (millis() - stamp > maxLoopTime)
	{
		maxLoopTime = millis() - stamp;
	}
	stamp = millis();
	

	// MQTT client connection
	if (WiFi.isConnected())
	{
		if (!mqttClient.connected())
		{
			long now = millis();
			if (now - lastReconnectAttempt > 5000)
			{
				DEBUG_PRINT ("Connecting\n");

				lastReconnectAttempt = now;
				// Attempt to reconnect
				if (reconnect())
				{
					lastReconnectAttempt = 0;
				}
			}
		}
		else
		{
			// Client connected
			mqttClient.loop();
		}
	}

	// Telnet server connection
	if (telnetServer.hasClient())
	{
		if (!serverClient || !serverClient.connected())
		{
			if (serverClient)
			{
				serverClient.stop();
				//Serial.println("Telnet Client Stop");
			}
			serverClient = telnetServer.available();
			//Serial.println("New Telnet client");
			serverClient.flush(); // clear input buffer, else you get strange characters
		}
	}

	while (serverClient.available())
	{   // get data from Client
		//Serial.write(serverClient.read());
	}

	if (millis() - startTime > 5000)
	{ // run every 25000 ms
		startTime = millis();

		DebugPrint("Alive");

		DEBUG_PRINT ("MaxLoop: %u\n", maxLoopTime);

		//  for (int i = 0; i < sizeof(readreq); i++)
		//  {
		//    pNibeMsgHandler->AddByte(readreq[i]);
		//    nibeHandler.Loop();
		//  }

		//  for (int i = 0; i < 100; i++)
		//  {
		//    nibeHandler.Loop();
		//  }

		//  for (int i = 0; i < sizeof(testdata); i++)
		//  {
		//    pNibeMsgHandler->AddByte(testdata[i]);
		//    nibeHandler.Loop();
		//  }
	}

	while (Serial.available())
	{
		pNibeMsgHandler->AddByte(Serial.read());
	}

	nibeHandler.Loop();
	io.loop();


}