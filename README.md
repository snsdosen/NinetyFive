# NinetyFive
### Arduino automotive EEPROM reader

This project came to fruition because I needed a quick way to swap ECU from one Alfa 156 to another.
Rather than buying various OBD cables or EEPROM readers I opted to use what is readily available to me at the moment - Arduino Leonardo.

The supported EEPROM is ST95P08/5P08C3 found in Bosch EDC15C7.

The approach is quick and dirty with out using client software, everything is done through command line.
All the tools needed are Arduino IDE, Putty and HxD.

## Hardware:
As of now the only Arduino board I tested this on is **Itead Leonardo**, 
an **Arduino Leonardo** compatible board containing ATmega32u4 microcontroller.

It should work on other Arduino boards with little or no modification.
This EEPROM supports both **3.3V** and **5V** operation.

##### Connection diagram:
| SPI EEPROM  | Arduino|
| ------------ | ------------ |
|1. S  | 10 |
|2. Q  | MISO |
|3. W | 3.3V |
|4. VSS | GND |
|5. D  | MOSI |
|6. C | SCK |
|7. HOLD | 3.3V |
|8. VCC | 3.3V |

## Software:
#### Arduino:
Use a provided "NinetyFive.ino" sketch and upload it to your Arduino devboard.

#### PC:
After you have flashed your Arduino board use Putty to connect to COM port Arduino is attached to.

#### Read data:
Write "read" to serial command line and press return.
Hex dump from the EEPROM will now appear on the screen.
Copy hex values and insert paste it to new file in HxD and save the file with .bin extension.

To write data copy the 1024 hex byte data from HxD and enter "write" to command line.
After the "WAITING DATA INPUT" message press right mouse click to paste and upload data.

Verify data again using "read" command and use HxD data comparison function.

#### Note:
Don't expect this sketch to work perfectly or that it's polished. I've written this over a span of two days and once I read, modified and uploaded new data to EEPROM I completed my initial goal so I did not worry about polishing it too much.

![Screenshot](img/putty_example.png?raw=true "1")
