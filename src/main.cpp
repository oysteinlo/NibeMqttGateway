#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "NibeHeater.h"
#include "IoContainer.h"

#define BUFFER_PRINT 300
#include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug

#define WIFI_SSID "WiFi2"
#define WIFI_PASSWORD "lobbenwifi"

#define MQTT_HOST IPAddress(192, 168, 10, 10)

#if ASYNC_TCP_SSL_ENABLED
#define MQTT_SECURE true
#define MQTT_SERVER_FINGERPRINT {0x7e, 0x36, 0x22, 0x01, 0xf9, 0x7e, 0x99, 0x2f, 0xc5, 0xdb, 0x3d, 0xbe, 0xac, 0x48, 0x67, 0x5b, 0x5d, 0x47, 0x94, 0xd2}
#define MQTT_PORT 8883
#else
#define MQTT_PORT 1883
#endif

#define DEBUG_PRINT rdebugDln	// Telnet debug
//#define DEBUG_PRINT printf

#define ENABLE_PIN 0 // GPIO_0 RS485 transceiver enablepinRS485 transceiver enablepin


#define HOST_NAME "ESP debug" 


AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

RemoteDebug Debug;

IoElement_t iopoints[] =
	{
		/* Tag								        				Identifer,	Type	IoDir	Factor	Cyclic	Deadband*/
		/*00*/ {"Alarm", 												45001, 		eS16, 	R, 		0, 		60000, 	0.0f},
		/*01*/ {"AZ1-BT50 Room temp", 					41213, 		eS16, 	R, 		10, 	60000, 	0.5f},
		/*02*/ {"BT1 Outdoor Temperature", 			40004, 		eS16, 	R, 		10, 	60000, 	0.5f},
		/*03*/ {"Hot water temperature", 				40014, 		eS16, 	R, 		10, 	60000, 	0.5f},
		/*04*/ {"Compressor starts EB100-EP14", 43416, 		eS32, 	R, 		0, 		60000, 	0.0f},
		/*05*/ {"Compressor State EP14", 				43427, 		eU8, 		R, 		0, 		60000, 	0.0f},
		/*06*/ {"DegreeMinute32", 						 	40940, 		eS32, 	R, 		10, 	60000, 	1.0f},
		/*07*/ {"Brine in temperature", 				40015, 		eS16, 	R, 		10, 	60000, 	0.5f},
		/*08*/ {"Brine out temperature", 				40016, 		eS16, 	R, 		10, 	60000, 	0.5f},
		/*09*/ {"Condensor out temperature", 		40017, 		eS16, 	R, 		10, 	60000, 	0.5f},
		/*10*/ {"Hot gas temperature", 					40018, 		eS16, 	R, 		10, 	60000, 	0.5f},
		/*11*/ {"Suction temperature", 					40022, 		eS16, 	R, 		10, 	60000, 	0.5f},
		/*12*/ {"Return temperature", 					40012,		eS16, 	R, 		10, 	60000, 	0.5f},
		/*13*/ {"EB100-EP14 Prio", 							44243, 		eU8, 		R, 		0, 		60000,	0.0f},
		/*14*/ {"Tot. HW op.time compr", 				43424, 		eS32, 	R, 		0, 		60000, 	0.0f},
		/*15*/ {"Tot.op.time compr", 						43420, 		eS32, 	R, 		0, 		60000, 	0.0f},
		/*16*/ {"Software version", 						43001, 		eU16, 	R, 		0, 		60000, 	0.0f},
		/*17*/ {"Holiday activated", 						48043, 		eU8, 		RW,		0, 		60000, 	0.0f},
		/*18*/ {"BT50 Room Temp S1", 						40033, 		eS16, 	R, 		10, 	60000,	0.5f},
		/*19*/ {"BT50 Room Temp S1 Average", 		40195, 		eS16, 	R, 		10, 	60000,	0.5f},
		/*20*/ {"DegreeMinute", 							 	43005, 		eS16, 	R, 		10, 	60000, 	1.0f},
		/*21*/ {"DM start heating", 					 	47206, 		eS16, 	R, 		0, 		60000, 	1.0f},
		/*22*/ {"DM between add. steps", 			 	47209, 		eS16, 	R, 		0, 		60000, 	1.0f},
		/*23*/ {"Heat Meter", 			 						40771, 		eU32, 	R, 		10, 	60000, 	1.0f},
		/*24*/ {"Supply Pump State EP14", 			43431, 		eU8, 		R, 		 0, 	60000, 	0.0f},
		/*25*/ {"Brine Pump State EP14", 			 	41433, 		eU8, 		R, 		 0, 	60000, 	0.0f},
		/*26*/ {"Alarm reset", 			 						45171, 		eU8, 		RW,		 0, 	60000, 	0.0f},
	};
const byte numIoPoints = sizeof(iopoints) / sizeof(IoElement_t);
IoContainer io("Nibe", iopoints, numIoPoints);

NibeMessage *pNibeMsgHandler;
NibeHeater nibeHandler(&pNibeMsgHandler, &io);

void processCmdRemoteDebug();

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

void DebugPrint(char text[])
{
  DEBUG_PRINT("%s", text);
}

void subscribe()
{
	for (int i = 0; i < numIoPoints; i++)
	{
		char topic[TOPIC_SIZE];
		io.GetTopic(i, topic);

		if (iopoints[i].eIoDir != R)
		{
			mqttClient.subscribe(topic, 0);
		}
	}
}

void connectToWifi() {
  DEBUG_PRINT("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  WiFi.hostname(HOST_NAME);
  // print your WiFi shield's IP address:
  
}

void connectToMqtt() {
  DEBUG_PRINT("Connecting to MQTT...");
  mqttClient.setCredentials("openhabian", "2048ping");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
	Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  DEBUG_PRINT("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  DEBUG_PRINT("Connected to MQTT.");
  subscribe();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  DEBUG_PRINT("Disconnected from MQTT.");

  if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
    DEBUG_PRINT("Bad server fingerprint.");
  }

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  DEBUG_PRINT("Subscribed to packet %d", packetId);
}

void onMqttUnsubscribe(uint16_t packetId) {
  DEBUG_PRINT("Unsubscribed to packet %d", packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  DEBUG_PRINT("Message recieved %s-%s", topic, payload);
  io.SetIoSzVal(topic, payload, len);
}

void onMqttPublish(uint16_t packetId) {
  DEBUG_PRINT("Published packet %d", packetId);
}

bool publish(char *topic, char *value)
{
	DEBUG_PRINT("Publish %s - %s", topic, value);
	return mqttClient.publish(topic, 0, false, value);
}

void setup() {
	pinMode(ENABLE_PIN, OUTPUT);	// RS485 enable pin

  Serial.begin(9600);
	nibeHandler.SetReplyCallback(SendReply);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
#if ASYNC_TCP_SSL_ENABLED
  mqttClient.setSecure(MQTT_SECURE);
  if (MQTT_SECURE) {
    mqttClient.addServerFingerprint((const uint8_t[])MQTT_SERVER_FINGERPRINT);
  }
#endif

 
  Debug.begin("DEBUG"); // Initiaze the telnet server
  Debug.setResetCmdEnabled(true); // Enable the reset command
  Debug.setCallBackProjectCmds(&processCmdRemoteDebug);
  //Debug.setSerialEnabled(true);
	// DebugLog p("T");
  // nibeHandler.AttachDebug(p);
	io.PublishFuncPtr(publish);
	connectToWifi();

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



unsigned int maxLoopTime = 0;
unsigned int startTime = 0;
unsigned int prevTime = 0;
void loop() {


    

    unsigned int now = millis();
    if (now - prevTime > maxLoopTime)
    {
        maxLoopTime = now - prevTime;
    }

#if 0
  if (now - startTime > 5000)
	{ // run every 25000 ms
		startTime = now;
		
		 for (unsigned int i = 0; i < sizeof(readreq); i++)
		 {
		   pNibeMsgHandler->AddByte(readreq[i]);
		   nibeHandler.Loop();
		 }

		 for (int i = 0; i < 100; i++)
		 {
		   nibeHandler.Loop();
		 }

		for (unsigned int i = 0; i < sizeof(testdata); i++)
		{
		  pNibeMsgHandler->AddByte(testdata[i]);
		  nibeHandler.Loop();
		}
	}
  
#else

    while (Serial.available())
	  {
			pNibeMsgHandler->AddByte(Serial.read());
	  }
#endif

	  nibeHandler.Loop();
	  io.loop();

    Debug.handle();

  	prevTime = millis();

    yield();
}

void processCmdRemoteDebug() {

	String lastCmd = Debug.getLastCommand();

  if (lastCmd == "maxloop") {

	if (Debug.isActive(Debug.ANY)) {
			rdebugAln("Max loop time %u", maxLoopTime);
		}
  }
}