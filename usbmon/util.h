//
//  util.h
//  lsusb
//
//  Created by Greg Yamanishi on 8/22/23.
//

#ifndef util_h
#define util_h
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#define PRINTF(format, ...) if (debugFlag) printf(format, ##__VA_ARGS__)
#define PRINT_PROPERTIES(entry) if (debugFlag) print_properties(entry)
extern char speedStandard[10][5];
extern char rate[10][15];
extern bool verboseFlag;
extern bool jsonFlag;
extern bool monitorFlag;
extern bool firstTime;
void formatTime(char *dt, time_t ttime);
void formatUSBPath(char *path, UInt32 locationID);
void print_properties(io_registry_entry_t entry);
void printEvent(const char event[], USBDevice d );

void SignalHandler(int sigraised);

// helpers to conver from core foundation format to C format
void replaceString(char *original,char target,char replace);
void ltrim(char *original);
void toString(char *buffer,CFTypeRef cf);
UInt32 toLong(CFNumberRef cf);
UInt8 toShort(CFNumberRef cf);


#endif /* util_h */
