This is the Arduino sketch for the Odin SDR console. It is written for an Arduino Due board.

To build this code you will need to install the following libraries into your documents\arduino\libraries folder:

DueFlashStorage
DueTimer
Encoder
ITEADLIB_Arduino_Nextion



To install the libraries
========================

DueFlashStorage: 
1. download from https://github.com/sebnil/DueFlashStorage
2. unzip into a folder "DueFlashStorage" in your documents\arduino\libraries folder

Encoder: 
1. open the Arduino IDE
2. click "Sketch | Include Library | Manage libraries..." on the menu
3. in the library manager type "encoder" where it sayds "filter your search" and hit enter
4. find "encoder" by Paul Stoffregen and click "install"

Due Timer:
1. open the Arduino IDE
2. click "Sketch | Include Library | Manage libraries..." on the menu
3. in the library manager type "due timer" where it sayds "filter your search" and hit enter
4. find "DueTimer" by Ivan Seidel and click "install"

ITEADLIB_Arduino_Nextion:
1. Download from https://github.com/itead/ITEADLIB_Arduino_Nextion
2. unzip into a folder "ITEADLIB_Arduino_Nextion" in your documents\arduino\libraries folder
3. Copy the three files from the "nextion display/arduino_library_update into that folder
4. (This is the library published by the display manufacturer. Be aware there is some foul language in thr "html" folder - delete the entire "html" folder if you do not want that)


To build the Arduino code
=========================
In the Arduino IDE:
1. Click "board" on the "tools" menu and select "Arduino Due (programming Port)"
2. Click "Verify/compile" on the "sketch" menu to compile
3. (this willl take around a minute and sholus result in a message sayin the % of program space used)


To upload to your arduino
=========================
1. Connect the Arduino programming port to your PC using a USB cable
2. Click "port" from the "tools" menu and select the serial port your board is using
3. Click "Upload" on the "sketch" menu to upload to the arduino