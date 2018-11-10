# NibeMqttGateway

This project use ESP-01 with Arduino framework to communicate with Nibe 1145 heatpump. ESP-01 interface the heatpump using RS485 interface intended for modbus converter.
Data from heatpump is decoded and published using MQTT protocol.

- Support reading of cyclic data
- Support read of specific address
- Support writing specific address

The project uses Visual Code with Platform IO and Arduino platform.

Known limitations/bugs:
- 32 bit values are not coded/decoded correctly
- Published messages that are subscribed will be written back to heatpump.

Use at own risk
