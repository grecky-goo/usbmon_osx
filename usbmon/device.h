//
//  device.h
//  lsusb
//
//  Created by Greg Yamanishi on 8/20/23.
//

#ifndef device_h
#define device_h

#include <CoreFoundation/CoreFoundation.h>
typedef struct USBDevice {
    int              next;
    int              prev;
    char             timestamp[30];
    bool             builtIn;
    UInt32           bcdDevice;
    UInt32           locationID;
    UInt8            speed;
    UInt8            Classid;
    UInt8            subClassid;
    char             IOClass[50];
    char             uuid[50];
    char             deviceName[50];
    char             deviceProduct[50];
    char             deviceVendor[50];
    char             deviceSerial[50];
    UInt8            classID[10];
    UInt8            subclassID[10];
    UInt64           terminateCount;
    UInt64           addCount;
} USBDevice;

enum rcode {EXISTING, FIRST, INSERT, PUSH, END, ERROR};

void           initializeUSBDevice(USBDevice *d, UInt32 locationID);
void           cpyUSBDevice(USBDevice *out, USBDevice in);

enum rcode put(USBDevice d);        // put device into dict
enum rcode get(int *d,UInt32 locationID);       // get device from dict
enum rcode putInterface (UInt32 locationID,UInt8 classID,UInt8 subclassID);
void       printDict(void);
void       printBackup(void);
void       makeBackup(void);
void       diff(void);
bool       exist(UInt32 locationID);
bool       existB(UInt32 locationID);
void       print(int i);
void       removals(void);
int        countDuplicates(void);
void       dumpDict(void);
void clearDict(void);

#endif /* device_h */

