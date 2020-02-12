# Beca thermostat Modbus to MQTT Gateway
This sketch is designed to provide an MQTT gateway to Beca Modbus thermostats. It uses a ESP8266 (in my case Wemos D1 mini) and an RS485 to TTL converter to read the Holding Registers of the thermostat and publish those values to MQTT. Also subscirbes to MQTT and relevant updates are written into the Modbus device (like power status, fan speed, mode, target temperature and screen lock).

Project explained in detail in this video: https://youtu.be/d8nhMimx9-0

## Libraries used
- Modbus Master library: https://github.com/4-20ma/ModbusMaster/blob/master/examples/RS485_HalfDuplex/RS485_HalfDuplex.ino
- ESP Software Serial: https://github.com/plerup/espsoftwareserial
- PubSubClient: handle MQTT communication. Install it from the Arduino Library Manager
- Time by Michael Margolis: also install this from the Arduino Library Manager
- SunMoon: sunrise and sunset calculation: https://github.com/sfrwmaker/sunMoon

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

## MQTT topic
Root topic is defined in the beginning of the sketch and the full topic gets appended to this. I specified "/beca/" as my root, so my topic are e.g. "/beca/read/power". Values read from the device have "read" in the topic, and values sent to the thermostat have "write". Sending messages to the "read" topics get ignored. There is no error handling on the payload of the "write" messages, data received gets sent to the device.
### Read topics (Published topics)
- root + "read/power": device status, Off (0) or On (1)
- root + "read/fan": fan speed, Auto (0), High (1), Medium (2), Low (3)
- root + "read/mode": mode, Cooling (0), Heating (1), Ventillation (2)
- root + "read/time": hour and time on the device in HH:MM format
- root + "read/weekday": weekday on the device: Monday (1), ... Sunday (7)
- root + "read/valve": valve state, Closed (0), Open (1)
- root + "read/lock" : screen lock, Unlocked (0), Locked (1)
- root + "read/ambient": ambient temperature in degrees centigrade, e.g. 24.5
- root + "read/target": target temperature in degrees centigrade, e.g. 20.5
- root + "rssi": wifi signal strength (RSSI)
- root + "uptime": uptime in minutes
### Write topics (Subscribed topics)
- root + "write/power": set device status (same values as above)
- root + "write/fan": set fan speed (same values as above)
- root + "write/mode": set mode (same values as above)
- root + "write/lock" : set screen lock  (same values as above)
- root + "write/target": set target temperature in degrees centigrade, e.g. 20.5
### Error topic (published)
- root + "error": Modbus error message in text (e.g. "Response timed out")

## Test board shown in the video
This is the general layout of the board showing how the Wemos D1 mini and the RS485 board is oriented:
![General board layout](https://github.com/nygma2004/beca_mqtt/blob/master/board01.jpg)

This is the side where the Wemos plugs in. You see the red +5V wires connected to the 5V pin of the Wemos and Vcc of the RS486 board. Black goes to GND on both:
![Wemos side](https://github.com/nygma2004/beca_mqtt/blob/master/board02.jpg)

And the other side of the board where the RS485 plugs. Yellow wires are the data wires, blue are the enable wires:
![Wemos side](https://github.com/nygma2004/beca_mqtt/blob/master/board03.jpg)

## Node-Red
There is an example Node-Red flow (also shown in the video), which shows how to communicate with the device from Node-Red. The flow is in Gitbuh, and also here: https://flows.nodered.org/flow/286a8cea2fbb2a60544f0dd2faefda2a

## PCB
I have also designed a PCB for this board. You can see my PCB project at PCBWay where you can order the PCB but you can also download the gerber files if you want to manufacture elsewhere: https://www.pcbway.com/project/shareproject/ESP8266_Modbus_board.html
You can find the BOM on the PCBWay project page. I purchased most of the components on eBay, but some of the links do not work any more. But only the RS486 to TTL converter is "special" everything else can be found in many different places, like Aliexpress.

## Custom Case
I have design a 3D printable case for the board, you find all the information in the case folder.
