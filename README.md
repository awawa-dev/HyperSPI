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
- SPI transmission is much lighter than serial communication
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

If you are using an ESP board compatible with the Wemos board (ESP8266 Wemos D1/pro, ESP32 MH-ET Live, ESP32-S2 lolin mini), the SPI connection uses the same pinout location on the ESP board! The pin positions of the LED output may vary. Cables (including ground) should not exceed 15cm or it may be necessary to lower the SPI speed. See how easy it is to connect ESP to Rpi:

![obraz](https://user-images.githubusercontent.com/69086569/216763154-ca4aa8fa-5855-43c1-86c2-d401010de675.png)
  
## Default pinout: ESP8266 (can not be changed)
  
| ESP8266     | PINOUT    |
|-------------|-----------|
| Clock (SCK) | GPIO 14   |
| Data (MOSI) | GPIO 13   |
| GROUND      | mandatory |
| LED output  | GPIO 2    | 

  
## Default pinout: ESP32 (can be changed)

| ESP32                     | PINOUT    |
|---------------------------|-----------|
| Clock (SCK)               | GPIO 18   |
| Data (MOSI)               | GPIO 23   |
| SPI Chip Select(e.g. CE0) | GPIO 5    |
| GROUND                    | mandatory |
| LED output                | GPIO 2    |
  

## Default pinout: ESP32-S2 lolin mini (can be changed)

| ESP32-S2 lolin mini       | PINOUT    |
|---------------------------|-----------|
| Clock (SCK)               | GPIO 7    |
| Data (MOSI)               | GPIO 11   |
| SPI Chip Select(e.g. CE0) | GPIO 12   |
| GROUND                    | mandatory |
| LED output                | GPIO 2    |

# Flashing
  
Recommend to use [esphome-flasher](https://github.com/esphome/esphome-flasher/releases)  

ESP32-S2 lolin mini requires using `esptool.py` to flash the firmware.

For **RGBW LED strip** like RGBW SK6812 NEUTRAL white choose: *hyperspi_..._SK6812_RGBW_NEUTRAL.bin*  
  
For **RGBW LED strip** like RGBW SK6812 COLD white choose: *hyperspi_..._SK6812_RGBW_COLD.bin*  
  
For **RGB LED strip** like WS8212b or RGB SK6812 variant choose: *hyperspi_..._WS281x_RGB.bin*  
    
If you want to disable your first LED because it's used as a sacrificial level shifter, please use [HyperHDR v19](https://github.com/awawa-dev/HyperHDR/pull/379) 

For the RGBW firmware the white channel is automatically calculated and R,G,B channels are corrected.  

# Example of supported boards

**Esp8266 Wemos D1 mini (CH340) and Wemos D1 mini pro (CP2104)**  
<p align="center">
<img src="https://user-images.githubusercontent.com/69086569/207572306-2b0bd3dd-fcb2-4f0c-8426-64341cbbadbf.png" /><img src="https://user-images.githubusercontent.com/69086569/207572335-9caf2567-2e23-4ee4-85a4-0f2f82676c16.png" />
</p>
  
**ESP32 MH-ET Live and ESP32-S2 Lolin mini (CDC)**  
<p align="center">
<img src="https://user-images.githubusercontent.com/69086569/207587620-1c4c53c8-426c-486e-a6d9-d429fd1b050d.png" /><img src="https://user-images.githubusercontent.com/69086569/207587635-b7816329-0e29-47ee-a75a-bc6c41cdc51f.png" />
</p>

# Compiling
  
Currently we use PlatformIO to compile the project. Install [Visual Studio Code](https://code.visualstudio.com/) and add [PlatformIO plugin](https://platformio.org/).
This environment will take care of everything and compile the firmware for you. Low-level LED strip support is provided by my highly optimizated (pre-fill I2S DMA modes, turbo I2S parallel mode for up to 2 segments etc) version of Neopixelbus library: [link](https://github.com/awawa-dev/NeoPixelBus).

But there is also an alternative and an easier way. Just fork the project and enable its Github Action. Use the online editor to make changes to the ```platformio.ini``` file, for example change default pin-outs or enable multi-segments support, and save it. Github Action will compile new firmware automatically in the Artifacts archive. It has never been so easy!

Tutorial: https://github.com/awawa-dev/HyperSPI/wiki

# Multi-Segment Wiring (ESP32 and ESP32-S2 only)

Proposed example of building a multisegment:
- Divide a long or dense strip of LEDs into 2 smaller equal parts. So `SECOND_SEGMENT_START_INDEX` in the HyperSerialESP32 firmware is the total number of LEDs divided by 2.
- Build your first segment traditional way e.g. clockwise, so it starts somewhere in middle of the bottom of frame/TV and ends in the middle of the top of frame/TV
- Start the second segment in the opposite direction to the first one e.g. counterclockwise (`SECOND_SEGMENT_REVERSED` option in the HyperSerialESP32 firmware configuration must be enabled). So it starts somewhere in the middle of the bottom of the frame/TV and ends in the middle of the top of the TV/frame. Both segments should be connected at the top but only 5v and ground ( NOT the data line).
- The data line starts for both segments somewhere in the middle of the bottom of the TV/frame (where each of the LED strips starts)
- Configuration in HyperHDR does not change! It's should be configured as one, single continues segment. All is done in HyperSerialESP32 firmware transparently and does not affect LED strip configuration in HyperHDR.

You also must configure data pin in the `platformio.ini`. Review the comments at the top of the file:
* `SECOND_SEGMENT_DATA_PIN` - These is data pin for your second strip

You add these to your board's config. Be sure to put `-D` in front of each setting. 

Examples of final build_flags for 288 LEDs divided into 2 equal segments in the `platformio.ini`:
```
[env:SK6812_RGBW_COLD]
build_flags = -DNEOPIXEL_RGBW -DCOLD_WHITE -DDATA_PIN=2 ${env.build_flags} -DSECOND_SEGMENT_START_INDEX=144 -DSECOND_SEGMENT_DATA_PIN=4 -DSECOND_SEGMENT_REVERSED
...
[env:WS281x_RGB]
build_flags = -DNEOPIXEL_RGB -DDATA_PIN=2 ${env.build_flags} -DSECOND_SEGMENT_START_INDEX=144 -DSECOND_SEGMENT_DATA_PIN=4 -DSECOND_SEGMENT_REVERSED
...
```

# Performance output

The output is only available when HyperHDR is not using the device at the moment, so it should be disabled in the app for a while. Stores the last result when HyperHDR was running in the current session. You can read it from the serial port at a speed of 115200.

![obraz](https://user-images.githubusercontent.com/69086569/216762783-0ce47e57-98a7-474d-aa84-7e5afb42d294.png)







