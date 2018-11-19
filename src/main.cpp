#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "NibeHeater.h"
#include "IoContainer.h"

#define REMOTEDEBUG
#include "DebugLog.h"

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

#define ENABLE_PIN 0 // GPIO_0 RS485 transceiver enablepinRS485 transceiver enablepin
#define HOST_NAME "ESP debug" 


AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

IoElement_t iopoints[] =
	{
							/* Tag			        				Identifer,	DaType	IoDir	Type			Cyclic	Deadband*/
		/*00*/ {"BT1 Outdoor Temperature", 			40004, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*01*/ {"Return temperature", 					40012,		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*02*/ {"Hot water temperature top",		40013, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*03*/ {"Hot water temperature load",		40014, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*04*/ {"Brine in temperature", 				40015, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*05*/ {"Brine out temperature", 				40016, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*06*/ {"Condensor out temperature", 		40017, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*07*/ {"Hot gas temperature", 					40018, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*08*/ {"Liquid line temperature", 			40019, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*09*/ {"Suction temperature", 					40022, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*10*/ {"BT50 Room Temp S1", 						40033, 		eS16, 	R, 		eAnalog, 	60000,	0.5f},
		/*11*/ {"Outdoor temperature avg",			40067, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*12*/ {"Current flow",									40072, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*13*/ {"DegreeMinute32", 						 	40940, 		eS32, 	R, 		eAnalog, 	60000, 	1.0f},
		/*14*/ {"BT50 Room Temp S1 Average", 		40195, 		eS16, 	R, 		eAnalog, 	60000,	0.5f},
		/*15*/ {"Total external hw-electric",		40755, 		eS32, 	R, 		eAnalog, 	60000, 	0.5f},
		/*16*/ {"Heat Meter", 			 						40771, 		eU32, 	R, 		eAnalog, 	60000, 	1.0f},
		/*17*/ {"Adjust Temp indoor",						40874, 		eS16, 	R,		eAnalog, 	60000, 	0.5f},
		/*18*/ {"Adjust Temp outdoor",					40875, 		eS16, 	R,		eAnalog, 	60000, 	0.5f},
		/*19*/ {"AZ1-BT50 Room temp", 					41213, 		eS16, 	R, 		eAnalog, 	60000, 	0.5f},
		/*20*/ {"Smart Home Mode", 							41265, 		eU8, 		RW,		eDefault,	60000, 	0.0f},
		/*21*/ {"Brine Pump State EP14", 			 	41433, 		eU8, 		R, 		eDefault,	60000, 	0.0f},
		/*22*/ {"Software version", 						43001, 		eU16, 	R, 		eDefault,	60000, 	0.0f},
		/*23*/ {"DegreeMinute", 							 	43005, 		eS16, 	R,		eAnalog, 	60000, 	1.0f},
		/*24*/ {"Compressor starts EB100-EP14", 43416, 		eS32, 	R, 		eDefault, 60000, 	0.0f},
		/*25*/ {"Tot.op.time compr", 						43420, 		eS32, 	R, 		eDefault,	60000, 	0.0f},
		/*26*/ {"Tot. HW op.time compr", 				43424, 		eS32, 	R, 		eDefault,	60000, 	0.0f},
		/*27*/ {"Compressor State EP14", 				43427, 		eU8, 		R, 		eDefault, 60000, 	0.0f},
		/*28*/ {"Supply Pump State EP14", 			43431, 		eU8, 		R, 		eDefault,	60000, 	0.0f},
		/*29*/ {"EB100-EP14 Prio", 							44243, 		eU8, 		R, 		eDefault,	60000,	0.0f},
		/*30*/ {"Alarm", 												45001, 		eS16, 	R, 		eDefault, 60000, 	0.0f},
		/*31*/ {"Alarm reset", 			 						45171, 		eU8, 		RW,		eDefault,	60000, 	0.0f},
		/*32*/ {"Hot water mode",							 	47041, 		eS8, 		RW,		eDefault,	60000, 	1.0f},
		/*33*/ {"Periodic hotwater",					 	47050, 		eS8, 		RW,		eDefault,	60000, 	1.0f},
		/*34*/ {"Operational mode",					 		47137, 		eS8, 		RW,		eDefault,	60000, 	1.0f},
		/*35*/ {"Operational mode heat",			 	47138, 		eS8, 		RW,		eDefault,	60000, 	1.0f},
		/*36*/ {"Operational mode brine",			 	47139, 		eS8, 		RW,		eDefault,	60000, 	1.0f},
		/*37*/ {"DM start heating", 					 	47206, 		eS16, 	RW,		eDefault,	60000, 	1.0f},
		/*38*/ {"DM between add. steps", 			 	47209, 		eS16, 	RW,		eDefault,	60000, 	1.0f},
		/*39*/ {"Holiday activated", 						48043, 		eU8, 		RW,		eDefault,	60000, 	0.0f},
		/*40*/ {"Room sensor setpoint S1",			47398, 		eS16,		RW,		eAnalog,	60000, 	0.0f},
	};
const byte numIoPoints = sizeof(iopoints) / sizeof(IoElement_t);
IoContainer io("Nibe", iopoints, numIoPoints);

NibeMessage *pNibeMsgHandler;
NibeHeater nibeHandler(&pNibeMsgHandler, &io);

void processCmdRemoteDebug();
void OtaUpdate() ;

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
 	rdebugpDln(text);
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

 #ifdef DEBUG
  Debug.begin("DEBUG"); // Initiaze the telnet server
  Debug.setResetCmdEnabled(true); // Enable the reset command
  Debug.setCallBackProjectCmds(processCmdRemoteDebug);
  //Debug.setSerialEnabled(true);
	#endif
	
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

#ifdef DEBUG
    Debug.handle();
#endif

  	prevTime = millis();

    yield();
}

#ifdef DEBUG
void processCmdRemoteDebug() {

	String lastCmd = Debug.getLastCommand();

  if (lastCmd == "maxloop") {

	if (Debug.isActive(Debug.ANY)) {
			rdebugAln("Max loop time %u", maxLoopTime);
		}
  }

	if (lastCmd == "update")
	{
		DEBUG_PRINT("Request update");
		OtaUpdate();
	}

	if (lastCmd == "version")
	{
		DEBUG_PRINT("Version x");
	}
}
#endif

void OtaUpdate() {
  t_httpUpdate_return ret = ESPhttpUpdate.update("192.168.10.10", 80, "/firmware.bin");

	DEBUG_PRINT("Start update");

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        DEBUG_PRINT("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        DEBUG_PRINT("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        DEBUG_PRINT("HTTP_UPDATE_OK");
        break;
		}
    
}
