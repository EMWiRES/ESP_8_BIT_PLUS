# **ESP_8_BIT_PLUS:** ColecoVision game console on your TV with nothing more than a ESP32 and a sense of nostalgia

## Based on [ESP_8_BIT](https://github.com/rossumur/esp_8_bit)

See it in action on [Youtube](https://youtu.be/rVNFPvaSYXc)

Tested on the [ESP32 USB stick with ESP_8_BIT support](http://www.emwires.com/ESP32USB/)

# The Emulated

## ColecoVision
Based on [smsplus](https://www.bannister.org/software/sms.htm) and [DarcNES](https://segaretro.org/DarcNES). Plays **.col** (ColecoVision) ROMs. 

| WiiMote (sideways) | COLECOVISION |
| ---------- | ----------- |
| arrow keys | joystick |
| A,1 | Keypad '1' |
| B,2 | Fire |
| Home | GUI |

# Time to Play

If you would like to upload your own media copy them into the appropriate subfolder: data/colecovision. Make sure that the games have file extension '.col' and put the ColecoVision bios 'coleco.rom' in there.

Note that the SPIFFS filesystem is fussy about filenames, keep them short, no spaces allowed. Use '[ESP32 Sketch Data Upload](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/)' from the 'Tools' menu to copy a prepared data folder to ESP32.
