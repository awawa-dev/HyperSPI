# HyperSPI
SPI bridge for AWA protocol to control a LED strip from HyperHDR.  
Diagnostic and performance data available at the serial port output [read more](#performancedebug-output).  
Raspberry Pi acts as a master, ESP8266/ESP32/ESP32-S2/rp2040(Raspberry Pi Pico) is in slave mode. 
  
| LED strip / Device             | rp2040 / Pico | ESP8266<br>(limited performance) |    ESP32 / ESP32-S2 mini    
|--------------------------------|:-------:|:-----------:|:-------:|
| SK6812 cold white              |   yes   |   yes   |     yes     |
| SK6812 neutral white           |   yes   |   yes   |     yes     |
| WS281x                         |   yes   |   yes   |     yes     |
  
  
# Why this project was created?

- SPI is very faster. HyperSPI works best at speed over 20Mb
- SPI doesn't have any data integration check. But AWA protocol does have one
- you don't need to have 2Mb capable serial port on your ESP board
- SPI transmission is much lighter than serial communication
- There is a hardware limitation for the Rpi current design...even if you connect your grabber using USB2.0 mode, working serial port driver (used by Adalight) results in quite a large drop in overall USB transfer. So we can replace Adalight with a pure SPI data transfer as an alternative
- I needed it and I was able to implemented it 😉

# Hardware connection  

If you are using an ESP board compatible with the Wemos board (ESP8266 Wemos D1/pro, ESP32 MH-ET Live, ESP32-S2 lolin mini), the SPI connection uses the same pinout location on the ESP board! The pin positions of the LED output may vary. Cables (including ground) should not exceed 15-20cm or it may be necessary to lower the SPI speed. 

The photos below use the same home-made adapter throughout, so you can see a repeating pattern and the cable colors should help you locate the correct pins. However, always consult the GPIO diagram for your boards to confirm that you have connected the cables correctly, because if you make a mistake and connect to the 5V GPIO line, it may damage both devices.  

As you can also notice, the pinout of the SPI0 interface is identical for the entire Raspberry Pi SBC family: 3, 4, 5, Zero 2W, etc.  

<table>  
  <tr>
    <td colspan="2"><p align="center">See how easy it is to connect Raspberry Pi Pico (rp2040) to Raspberry Pi 5 using SPI</p></td>    
  </tr>
  <tr>
    <td colspan="2"><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/7f24f87e-f7e0-43f3-a568-39d0f6beced1"/></td>
  </tr>
  <tr>
    <td><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/170f5718-df88-4ef2-9ed8-7d92d913aeec"/></td>
    <td><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/9235c689-062e-4c62-b632-8c506b0e2e97"/></td>
  </tr>
  <tr>
    <td colspan="2"><p align="center">or if you prefer ESP32/ESP32-S2/Esp8266</p></td>
  </tr>
  <tr>
    <td><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/1e500ca3-e93d-4082-af59-b701e6274a29"/></td>
    <td><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/6e9ea441-312e-46d8-99b0-d33fc0b9f923"/></td>
  </tr>
  <tr>
    <td><img src="https://user-images.githubusercontent.com/69086569/216763154-ca4aa8fa-5855-43c1-86c2-d401010de675.png"/></td>
    <td><img src="https://user-images.githubusercontent.com/69086569/231207350-a670bfea-a96d-4d21-9e8f-f2ca027105da.png"/></td>
  </tr> 
</table>

# Example of supported boards

<p align="center">
<b>Adafruit RP2040 Scorpio and ItsyBitsy with built-in level shifter (recommended!)</b><br/>  
<img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/c7ef2768-357e-47ca-aa1c-543769eeb360" width="250" /><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/3e55948d-fd3d-44cb-908c-9aad8ce2715d" width="250" />
</p>

<p align="center">
<b>Esp8266 Wemos D1 mini (CH340) and Wemos D1 mini pro (CP2104)</b><br/>
<img src="https://user-images.githubusercontent.com/69086569/207572306-2b0bd3dd-fcb2-4f0c-8426-64341cbbadbf.png" width="250" /><img src="https://user-images.githubusercontent.com/69086569/207572335-9caf2567-2e23-4ee4-85a4-0f2f82676c16.png" width="250" />
</p>

<p align="center">
<b>ESP32 MH-ET Live and ESP32-S2 Lolin mini (CDC)</b><br/>
<img src="https://user-images.githubusercontent.com/69086569/207587620-1c4c53c8-426c-486e-a6d9-d429fd1b050d.png" width="250"/><img src="https://user-images.githubusercontent.com/69086569/207587635-b7816329-0e29-47ee-a75a-bc6c41cdc51f.png" width="250"/>
</p>

## Default pinout (can be changed for esp32, esp32-s2 and rp2040 Pico)
  
|    PINOUT   |  ESP8266  |   ESP32   | ESP32-S2 | Pico (rp2040) | Adafruit rp2040<br/>Scorpio & ItsyBitsy
|-------------|-----------|-----------|-----------|-----------|-----------|
| Clock (SCK) | GPIO 14   | GPIO 18   | GPIO 7    | GPIO 2    | GPIO 26 (A0) |
| Data (MOSI) | GPIO 13   | GPIO 23   | GPIO 11   | GPIO 4    | GPIO 28 (A2) |
| SPI Chip Select(e.g. CE0) | not used    | GPIO 5    | GPIO 12   | GPIO 5    | GPIO 29 (A3) |
| GROUND      | mandatory | mandatory | mandatory | mandatory | mandatory |
| LED output  | GPIO 2    | GPIO 2    | GPIO 2    | GPIO 14 | GPIO16 / GPIO14 |

> [!CAUTION]
> The ground connection between both GPIOs is as important as the other SPI data connections. The ground cable should be of a similar length as them and run directly next to them.

# Flashing the firmware

There are two versions of the firmware for ESP32 and ESP32-S2. The 'factory' (in the `recovery_firmware.zip` archive) and the 'base' one. Factory firmware should be flashed to offset 0x0, base firmware to offset 0x10000.

## Flashing ESP32-S2 Lolin mini

Requires using `esptool.py` to flash the firmware e.g.  
 - `esptool.py write_flash 0x10000 hyperspi_esp32_s2_mini_SK6812_RGBW_COLD.bin` or
 - `esptool.py write_flash 0x0 hyperspi_esp32_s2_mini_SK6812_RGBW_COLD.factory.bin`

Troubleshooting: ESP32-S2 Lolin mini recovery procedure if the board is not detected or is malfunctioning.  
1. Put the board into dfu mode using board buttons: press `Rst` + `0` buttons, then release `Rst`, next release `0`  
Do not reset or disconnect the board until the end of the recovery procedure.
2. Execute `esptool.py erase_flash`  
3. Flash 'factory' version of the firmware e.g.  
`esptool.py write_flash 0x0 hyperspi_esp32_s2_mini_SK6812_RGBW_COLD.factory.bin`  
4. **`esptool.py` is not able to automatically reset esp32-s2 when in dfu mode. Reconnect or hard reset it manually.** The board should be detected as a COM port in the system.

## Flashing generic Esp8266/ESP32

Recommend to use [esphome-flasher](https://github.com/esphome/esphome-flasher/releases)  
Or use `esptool.py` e.g.  
 - `esptool.py write_flash 0x10000 hyperspi_esp32_SK6812_RGBW_COLD.bin` or
 - `esptool.py write_flash 0x0 hyperspi_esp32_SK6812_RGBW_COLD.factory.bin`

For **RGBW LED strip** like RGBW SK6812 NEUTRAL white choose: *hyperspi_..._SK6812_RGBW_NEUTRAL.bin*  
For **RGBW LED strip** like RGBW SK6812 COLD white choose: *hyperspi_..._SK6812_RGBW_COLD.bin*  
For **RGB LED strip** like WS8212b or RGB SK6812 variant choose: *hyperspi_..._WS281x_RGB.bin*  

## Flashing Pico boards

It's very easy and you don't need any special flasher.  

Use firmware from the `PicoRp2040Boards.zip` archive. Adafruit boards have their own custom firmware package inside the archive: `Adafruit_ItsyBitsy_RP2040.zip` and `Adafruit_Feather_RP2040_Scorpio.zip`

Put your Pico board into DFU mode:  
* If your Pico board has only one button (`boot`) then press & hold it and connect the board to the USB port. Then you can release the button.
* If your Pico board has two buttons, connect it to the USB port. Then press & hold `boot` and `reset` buttons, then release `reset` and next release `boot` button.  

In the system file explorer you should find new drive (e.g. called `RPI-RP2` drive) exposed by the Pico board. Drag & drop (or copy) the selected firmware to this drive. 
The Pico will reset automaticly after the upload and after few seconds it will be ready to use.
  
# Software configuration (HyperHDR v17 and above)

**In HyperHDR `Image Processing→Smoothing→Update frequency` you should do not exceed the maximum capacity of the device. Read more here: [testing performance](https://github.com/awawa-dev/HyperSPI#performance-output)**

Select esp8266 protocol for ESP proprietary SPI protocol, esp32 for ESP32 boards, `rp2040 (Pico)` for Pico boards or 'standard' for other devices.    
Make sure you set "Refresh time" to zero, "Baudrate" should be set to high but realistic value like ```25 000 000```.  
Enabling "White channel calibration" is optional, if you want to fine tune the white channel balance of your sk6812 RGBW LED strip.
  
<img src="https://user-images.githubusercontent.com/69086569/193319124-0054f367-3d30-4e50-8c52-3683c7bbc50e.png" width="800"/>

# Benchmark results

## ESP32 & ESP32-S2 parallel multi-segment mode

| LED strip / Device                                                                      | ESP32 MH-ET LIVE mini<br/>HyperSPI v9 |  ESP32-S2 Lolin mini<br/>HyperSPI v9 |
|-----------------------------------------------------------------------------------------|-----------------------|----------------------|
| 300LEDs sk6812<br>Refresh rate/continues output=100Hz<br>SECOND_SEGMENT_START_INDEX=150 |          100          |          100         |
| 600LEDs sk6812<br>Refresh rate/continues output=83Hz<br>SECOND_SEGMENT_START_INDEX=300  |           83          |           83         |
| 900LEDs sk6812<br>Refresh rate/continues output=55Hz <br>SECOND_SEGMENT_START_INDEX=450 |         54-55         |           55         |

## ESP32

| LED strip / Device                             | ESP32 MH ET Live<br/>HyperSPI v9 | ESP32-S2 Lolin mini<br/>HyperSPI v9 |
|------------------------------------------------|-----------------|--------------------|
| 300LEDs RGBW<br>Refresh rate/continues output=83Hz  |        83       |          83        |
| 600LEDs RGBW<br>Refresh rate/continues output=43Hz  |      42-43      |          42        |
| 900LEDs RGBW<br>Refresh rate/continues output=28Hz  |       28        |          28        |

## ESP8266

| LED strip / Device                             | ESP8266 Wemos D1 Pro<br/>HyperSPI v9 |
|------------------------------------------------|---------------------|
| 300LEDs RGBW<br>Refresh rate/continues output=70Hz  |          70         |
| 600LEDs RGBW<br>Refresh rate/continues output=33Hz  |          33         |
| 900LEDs RGBW<br>Refresh rate/continues output=22Hz  |          22         |

# Compiling

## ESP8266 / ESP32

Currently we use PlatformIO to compile the project. Install [Visual Studio Code](https://code.visualstudio.com/) and add [PlatformIO plugin](https://platformio.org/).
This environment will take care of everything and compile the firmware for you. Low-level LED strip support is provided by my highly optimizated (pre-fill I2S DMA modes, turbo I2S parallel mode for up to 2 segments etc) version of Neopixelbus library: [link](https://github.com/awawa-dev/NeoPixelBus).

## Pico rp2040

Use Pico SDK and Visual Code to open ```rp2040``` folder. Edit ```rp2040\CMakeLists.txt``` configuration file if you need to apply changes.

## Github Action

But there is also an alternative and an easier way. Just fork the project and enable its Github Action. Use the online editor to make changes:
- esp8266/ESP32 boards: to the ```platformio.ini``` file
- rp2040 Pico boards: to the ```rp2040\CMakeLists.txt``` file

for example change default pin-outs or enable multi-segments support, and save it. Github Action will compile new firmware automatically in the Artifacts archive. It has never been so easy! **Just remember to follow the steps in the correct order otherwise the Github Action may not be triggered the first time after saving the changes.**

Tutorial: https://github.com/awawa-dev/HyperSPI/wiki

# Multi-Segment Wiring (ESP32, ESP32-S2 and Pico rp2040 boards only)

## ESP32

Using parallel multi-segment allows you to double your Neopixel (e.g. sk6812 RGBW) LED strip refresh rate by dividing it into two smaller equal parts. Both smaller segments are perfectly in sync so you don't need to worry about it. Proposed example of building a multisegment:
- Divide a long or dense strip of LEDs into 2 smaller equal parts. So `SECOND_SEGMENT_START_INDEX` in the HyperSPI firmware is the total number of LEDs divided by 2.
- Build your first segment traditional way e.g. clockwise, so it starts somewhere in middle of the bottom of frame/TV and ends in the middle of the top of frame/TV
- Start the second segment in the opposite direction to the first one e.g. counterclockwise (`SECOND_SEGMENT_REVERSED` option in the HyperSPI firmware configuration must be enabled). So it starts somewhere in the middle of the bottom of the frame/TV and ends in the middle of the top of the TV/frame. Both segments could be connected if possible at the top but only 5v and ground ( NOT the data line).
- The data line starts for both segments somewhere in the middle of the bottom of the TV/frame (where each of the LED strips starts)
- Configuration in HyperHDR does not change! It's should be configured as one, single continues segment. All is done in HyperSPI firmware transparently and does not affect LED strip configuration in HyperHDR.

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
Implementation example:
- The diagram of the board for WS2812b/SK6812 including ESP32 and the SN74AHCT125N 74AHCT125 [level shifter](https://github.com/awawa-dev/HyperHDR/wiki/Level-Shifter).

# Pico rp2040

Edit ```rp2040\CMakeLists.txt``` file and recompile the project.

![HyperSPI](https://user-images.githubusercontent.com/85223482/222923979-f344349a-1f8b-4195-94ca-51721923359e.png)

# Performance/debug output

**The output is only available when HyperHDR is not using the device at the moment, so it should be disabled in the app for a while.** Stores the last result when HyperHDR was running in the current session. You can read it from the serial port at a speed of 115200.

On Linux you can `screen` command.  
First install it: `sudo apt install screen`. Adjust USB port if necessary and connect to the serial port:  
`screen /dev/ttyACM0 115200`  
If you want to exit screen press `Ctrl-a` then `k` and confirm exit.

You can also use `Putty` on Windows
![obraz](https://user-images.githubusercontent.com/69086569/216762783-0ce47e57-98a7-474d-aa84-7e5afb42d294.png)  

For testing maximum performance in HyperHDR enable `Image Processing→Smoothing→Continuous output`, high `Update frequency` in the same tab and set any color in the `Remote control` tab as an active effect. After testing you need to disable `Continuous output`and set `Update frequency` according to your results.

  
 









