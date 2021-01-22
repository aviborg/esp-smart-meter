# esp-smart-meter
A reliable ESP8266 application to read out data from COSEM/DLMS based smart meters using OBIS code/structure.

## Background
Being able to read out user data from different utility meters in your household is being introduced in many European countries. 
For electricity metering the Netherlands and Norway are among the pioneers both basing the meters on the COSEM/DLMS standard. These countries have chosen different hardware interfaces though. The Dutch standard send serial data over UART with a RJ12 interface on their electricity meters, whereas the Norwegians send serial data over a M-Bus interface with a RJ45 connector on their meters. Common names for the user interface port are H1, P1 or HAN-port.

The purpose of this project is to make an application for use on the Swedish market, but it should be possible to use it on any smart meter following the COSEM/DLMS standard. So this software should be compatible with Norwegian and Dutch smart meters or any smart meter using this standard.

In Sweden smart meters are supposed to follow the Dutch standard, but initially smart meters in Sweden were compatible to the Norwegian standard. So there exist a mixture of smart meters in Sweden adhering to different standars.

Also many implementations on github are custom built to the specific meter that particular user happened to have. As the meter data is structured, similar to for example json data, the goal of this project is to write an application that parse serial data from any smart meter to json format.

## Architecture
The basic idea is to use an NodeMCU which have the ESP8266 chip, any ESP-chip should work as the needed functionality is one serial RX-pin and WiFi-connectivity.

Serial data from the HAN-port is sent at 115200 baud, a message of 512 bytes would then take 40 ms to receive. This has to be accounted for when using the ESP8266 chip which has a default serial buffer size of 256 bytes. If the serial buffer is not read in within 20 ms it will be full and data will be lost. If too much time is spent on serving the webserver it might cause buffer overrun. In this project the serial buffer is increased to 4096 bytes to ensure there will be space for the data, and minimize the risk of overrun.

The basic design principle is to have a major schedule frame checking the serial buffer during each cycle and save data to a json-file which is accessible from the web interface. Then there are minor frames to do other tasks when there are no serial data to process.

When data is recieved it is parsed to a data.json file accessible from the web interface. This data.json file can then be fetched by a home automation server of any flavour.

Other functionality is mDNS to make the server accessible by hostname, for example http://emeter/ instead of ip address http://192.168.1.233
This does not work on all OS'es and webbrowsers.

A hard reset functions is bound to the flash button/input. If held for 5 seconds it will erase settings and reset the chip.

OTA functionality.

For first time setup and connection to your local WiFi the chip will act as a access point which can be connected to from a mobile phone. 

If data cannot be parsed correctly, failed data will be dumped to a log.txt file accessible through the web interface.

## Hardware
The serial input cannot be connected directly to the HAN-port, some HW is required inbetween. 

Usually you need to contact your electricity provider to enable the HAN-port before you'll be able to readout data.

If the electricity meter is placed inside a grounded metal cabinet you might consider to place the chip outside of the cabinet as radio signals are attenuated by metal boxes.
### RJ45
The norwegian standard use a RJ45 connector and send data over M-Bus. This bus use the same pin for data and power supply. There is also an additional ground pin. Thus, even though the RJ45 have 8 pins, only 2 are used. There is a chip that can be used for M-Bus applications: TSS721. This project is not tested with this HW-interface, you may find ideas how to use this chip here: https://github.com/roarfred/AmsToMqttBridge/tree/master/Electrical

### RJ12
RJ11 is the connector that is used for those rare landline telephones still in service. It has 6 pin positions but only 4 electrical pins, RJ12 is the same size but it has 6 electrical pins. Make sure you get RJ12 connecter as the extra pins are needed! Actually only 4 pins would be enough but the required pins are at the edges so that's why you need the RJ12. In the specification, 5 pins are defined but 2 of these are ground, so one of these could be skipped. This enables the use of a 4 wire telephone cable.

The pins are as this:

1. 5 V power supply
2. Data request, short this pin to 5 V to request data continously.
3. Power ground
4. Not used
5. Data, open collector
6. Data ground

Pin 5 being an open collector means that a logical 1 will be equal to connection to ground, whereas a logical 0 will be floating. Another complication to this is that the ESP8266 only works with max 3.3 V on its RX-input. The NodeMCU has a voltage regulator so you can power it with the 5 V pin. But if you use a bare ESP-chip you need a voltage regulator to step down the power to 3.3 V. In order to invert and step down the data signal you need a NPN-transistor, 2 resistors and a capacitance. The capacitance is optional, but it is good practice to have one on the power supply input to reduce current spikes when the ESP is transmitting.
Below is a schematic and an example the setup.
![Schematic](img/nodemcuschematic_bb.png "Schematic")
![Schematic](img/nodemcuwiring_bb.png "Schematic")
![Electronics](img/DSC_0101.JPG "Electronics")





