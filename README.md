# usbmon_osx

A command line tool to list and monitor USB devices connected to a OSX host.  It is designed to be passive, with
minimal interference with the running device.  The program is reading and transversing the IORegistry,
and reflecting changes in the device list.  It uses a polling method with updates happening every 10 seconds.

## Requirements
* IOKIT
* CoreFoundation frameworks.
* XCODE

## Usage Flags
```
usbmon [-n][-j][
  -n output current devices and do not monitor events
  -j output in updates json format
  -h help

Events will be a list of devices either being added or removed:
   Date     Time      Host       Event       Path  bcd#   Vendor       Name     Serial     Std     Rate 
2023/08/25 17:49:32 macbookpro:Remove Device 14-1 37412 GenesysLogic USB2.0_Hub@00000000 USB:2.0 Rate:480Mbps
```

The program will continue to run until a ctrl-c is issued.
