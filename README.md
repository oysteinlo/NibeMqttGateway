# NibeMqttGateway

This project use ESP-01 with Arduino framework to communicate with Nibe 1145 heatpump. ESP-01 interface the heatpump using RS485 interface intended for modbus converter.
Data from heatpump is decoded and published using MQTT protocol.

- Support reading of cyclic data
- Support read of specific address
- Support writing specific address

The project uses Visual Code with Platform IO and Arduino platform.
