rpi-alarm-1.jpg

This image shows the 6160 Honeywell keypad connected the the Arduino and RPi running my custom code.

rpi_alarm-2.jpg

This image documents the full alarm setup.  The RPi is at the top with a GPIO break-out board hat installed.  My home's sense loops connect to the RPi GPIO pins via the screw terminals on the break-out board.  You will also notice I am using a TP-Link USB wifi adapter with an external antenna.  The Arduino 2560 mega is connected vi a short USB cable.  The Arduino has a protoboard hat with circuit shown in Arduino_Keypad_Wiring.jpg.  The keyboards are connected to this hat via a screw terminal block.  The power to the RPi and Arduino is provided by a DC-DC converter with its output adjusted to 5V.  The DC-DC converter's input is provided by a picoUPS card which is connected to the input DC adapter and a 12V sealed lead acid (SLA) battery. 

Links to some of parts mentioned above:
 - DC-DC converter https://www.amazon.com/gp/product/B009HPB1OI
 - picoUPS: https://www.amazon.com/gp/product/B005TWE4GU
 - battery: https://www.amazon.com/PowerStar3-Warranty-Security-System-Battery/dp/B00G045G1I
