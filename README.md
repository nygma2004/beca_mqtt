# Beca thermostat Modbus to MQTT Gateway
This sketch is designed to provide an MQTT gateway to Beca Modbus thermostats. It uses a ESP8266 (in my case Wemos D1 mini) and an RS485 to TTL converter to read the Holding Registers of the thermostat and publish those values to MQTT. Also subscirbes to MQTT and relevant updates are written into the Modbus device (like power status, fan speed, mode, target temperature and screen lock).

## Libraries used
- Modbus Master library: https://github.com/4-20ma/ModbusMaster/blob/master/examples/RS485_HalfDuplex/RS485_HalfDuplex.ino
- ESP Software Serial: https://github.com/plerup/espsoftwareserial

### Wiring
```
Wemod D1 mini (or other ESP)            TTL to RS485
D1                                      DE
D2                                      RE
D5                                      RO
D6                                      DI
5V                                      VCC
G(GND)                                  GND
```
Red Modbus wire goes to the RS486 "A" screw terminals and black to "B".
