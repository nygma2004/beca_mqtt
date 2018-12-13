# Beca thermostat Modbus to MQTT Gateway
This sketch is designed to provide an MQTT gateway to Beca Modbus thermostats. It uses a ESP8266 (in my case Wemos D1 mini) and an RS485 to TTL converter to read the Holding Registers of the thermostat and publish those values to MQTT. Also subscirbes to MQTT and relevant updates are written into the Modbus device (like power status, fan speed, mode, target temperature and screen lock).

## Libraries used
- Modbus Master library: https://github.com/4-20ma/ModbusMaster/blob/master/examples/RS485_HalfDuplex/RS485_HalfDuplex.ino
- ESP Software Serial: https://github.com/plerup/espsoftwareserial

## Wiring
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

## Published topic
Root topic is defined in the beginning of the sketch and the full topic gets appended to this. I specified "/beca/" as my root, so my topic are e.g. "/beca/read/power". Values read from the device have "read" in the topic, and values sent to the thermostat have "write". Sending messages to the "read" topics get ignored. There is no error handling on the payload of the "write" messages, data received gets sent to the device.
### Read topics
- root + "read/power": device status, Off (0) or On (1)
- root + "read/fan": fan speed, Auto (0), High (1), Medium (2), Low (3)
- root + "read/mode": mode, Cooling (0), Heating (1), Ventillation (2)
- root + "read/time": hour and time on the device in HH:MM format
- root + "read/weekday": weekday on the device: Monday (1), ... Sunday (7)
- root + "read/valve": valve state, Closed (0), Open (1)
- root + "read/lock" : screen lock, Unlocked (0), Locked (1)
- root + "read/ambient": ambient temperature in degrees centigrade, e.g. 24.5
- root + "read/target": target temperature in degrees centigrade, e.g. 20.5
### Write topics
- root + "write/power": set device status (same values as above)
- root + "write/fan": set fan speed (same values as above)
- root + "write/mode": set mode (same values as above)
- root + "write/lock" : set screen lock  (same values as above)
- root + "write/target": set target temperature in degrees centigrade, e.g. 20.5
### Error topic
- root + "error": Modbus error message in text (e.g. "Response timed out")
