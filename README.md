# RHTmachine
Wild combination of a floppy disk drive control board, a CD frame with step motor, an analogue gauge and a lot of other bits (including a toothpick) to monitor temperature, relative humidity and light. Can be driven by any microcontroller with digital/analogue inputs: Arduino, Galileo, Edison. Source code in this repo is for Arduino Nano clone.

Requires couple of files from my [YAHL](https://github.com/achilikin/Galileo/tree/master/YAHL) library

![RHTmachine](https://rawgithub.com/achilikin/RHTmachine/master/files/rht-machine.svg)

Change default Arduino H fuse from 0xDA to 0xD2 to preserve EEPROM memory.

Commands available via serial port (38400 baud):

```
> help
  mem
  status
  version
  reset stat
  print now
  print log [n]
  config on|off
  set gauge 0-255
  set trigger 1|0
  set time hh:mm:ss
  set contrast 0-255
  echo rht|thist|extra|verbose on|off
>
```
* _mem_: show available memory
* _status_: show current settings
* _version_: show compilation date and program/memory used
* _reset stat_: reset rht poll statistics
* _print now_: print current values
* _print log [n]_: print log for the last 12/24h (or n last records), timing from the current time
* _config on|off_: set configuration mode, if on - disables gauge and needle updates
* _set gauge 0-122_: used for gauge calibration, works only in config mode
* _set trigger 0|1_: set trigger output (pull down npn) to on/off
* _set time hh:mm:ss_: set real-time clock, 24h format 
* _set contrast_: sets OLED display contrast, default is 64
* _echo rht|thist|extra|verbose on|off_: sets different debug echo options

Current version: ```1511-29, 22,758/1175 (74/57%) bytes```

# License: MIT
