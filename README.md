# HyperSPI
SPI bridge for AWA protocol to control a LED strip from HyperHDR (version v17 and above).  
Diagnostic data available at the serial port output (@115200 speed).  
Rpi acts as a master, ESP8266/ESP32 is in slave mode. 
  
| LED strip / Device             | ESP8266 |    ESP32    |
|--------------------------------|:-------:|:-----------:|
| SK6812 cold white              |   yes   |     yes     |
| SK6812 neutral white           |   yes   |     yes     |
| WS281x                         |   yes   |     yes     |
  
  
# Why this project was created?

- SPI is much faster. HyperSPI works best at speed over 20Mb.
- SPI doesn't have any data integration check. But AWA protocol does have one.
- you don't need to have 2Mb capable serial port on your ESP board.
- I needed it and I was able to implemented it ;)
- There is a hardware limitation for the Rpi current design...even if you connect your grabber to the USB3.0 in the USB2.0 mode the adalight running driver causes quite a big USB transfer drop. So we can replace Adalight with a pure SPI data transfer as an alternative.

See what's happening for USB2.0 bus... my problematic Ezcap 320 @ 50 fps fell back to USB2.0 mode and did not like the CH340G serial port driver at all (real USB3.0 should not be affected, but not tested it):  
![slow](https://user-images.githubusercontent.com/69086569/129419155-f6366c27-ea2e-42a9-aa85-ffade3747700.jpg)  
  
That's how the grabbers works when other device is disconnected from the USB port.
![fast](https://user-images.githubusercontent.com/69086569/129419160-c546a0ea-4990-4215-a0a9-8fb1288e0ac9.jpg)
  
# Software configuration (HyperHDR v17 and above)
Select esp8266 protocol for ESP proprietary SPI protocol, esp32 for ESP32 boards or 'standard' for other devices.    
Make sure you set "Refresh time" to zero, "Baudrate" should be set to high but realistic value like ```25 000 000```.  
Enabling "White channel calibration" is optional, if you want to fine tune the white channel balance of your sk6812 RGBW LED strip.
  
![obraz](https://user-images.githubusercontent.com/69086569/193319124-0054f367-3d30-4e50-8c52-3683c7bbc50e.png)

# Hardware connection  
  
## ESP8266  (only 3 SPI cables are needed: MOSI, MISO, SCLK + common ground)  
  
**ESP8266 SPI input:** GPIO 14 for Clock (SCK), GPIO 12 for Data (MISO), GPIO 13 for Data (MOSI)  
**LED output:** GPIO 2  
  
![cables](https://user-images.githubusercontent.com/69086569/129419654-84087cc5-b74f-4d8d-84c3-54ef3d845627.jpg)  
![rpi](https://user-images.githubusercontent.com/69086569/129419668-17621117-0e3a-4cfc-a5b3-02932824889e.jpg)  
![esp](https://user-images.githubusercontent.com/69086569/129419687-dbd0d5b0-1b45-4ce5-8666-6469b0970952.jpg)  
  
## ESP32 (need 3 SPI cables + common ground)  
  
**Warning**: HyperSPI set SPI_MODE0 on default due to a bug with a SPI bit shift [link](https://github.com/espressif/esp-idf/search?q=dma+spi+bit+shift&type=issues), you may need to change it to SPI_MODE3 for slower speeds  
  
**ESP32 SPI input:** VSPI interface, GPIO 5 for SPI Chip Select, GPIO 18 for Clock (SCK), GPIO 23 for Data (MOSI)  
**LED output (non-SPI):** GPIO 2  
  
![esp2](https://user-images.githubusercontent.com/69086569/130372512-d3dd4dde-5069-4ad9-8649-7ea8e874ee07.jpg)
![esp](https://user-images.githubusercontent.com/69086569/130372517-dfc61fd4-e700-49f1-b3a1-b56c8468837f.jpg)

## ESP32-S2 mini (need 3 SPI cables + common ground)  
  
**ESP32-S2 SPI input:** FSPI interface, GPIO 12 for SPI Chip Select, GPIO 7 for Clock (SCK), GPIO 11 for Data (MOSI)  
**LED output (non-SPI):** GPIO 2  

# Compiling
  
Compile the sketch using Arduino IDE. You need:  
- https://github.com/espressif/arduino-esp32 (boards for ESP32)
- https://github.com/esp8266/Arduino/ (boards for ESP8266)  
- Makuna/NeoPixelBus (install from Arduino IDE: manage libraries)  
- ESP32DMASPI (install from Arduino IDE: manage libraries)  
  
**Options (first lines of the sketch):**  
  
For RGB strip like WS8212b comment it with a '//', leave it for RGBW SK6812:  
*#define   THIS_IS_RGBW*  
  
For RGBW cold white LED strip version if above declaration is defined, delete it or comment it with '//' for RGBW neutral:  
*#define   COLD_WHITE*  
  
Skip first led in the strip, that is used as level shifter:  
*bool      skipFirstLed = true;*  
  
Don't change LED's count as it is dynamic.  







