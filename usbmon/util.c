//
//  util.c
//  lsusb
//
//  Created by Greg Yamanishi on 8/22/23.
//
#include "device.h"
#include "util.h"
#include <time.h>


bool debugFlag = FALSE;
char speedStandard[10][5] = {
    "1.0",        // kUSBDeviceSpeedLow
    "1.1",        // kUSBDeviceSpeedFull
    "2.0",        // kUSBDeviceSpeedHigh
    "3.0",        // kUSBDeviceSpeedSuper
    "3.1",        // kUSBDeviceSpeedSuperPlus
    "3.2",        //  kUSBDeviceSpeedSuperPlusBy2
    "4.0"};       // tbd
char rate[10][15] = {
    "1.5",        // kUSBDeviceSpeedLow
    "12",         // kUSBDeviceSpeedFull
    "480",        // kUSBDeviceSpeedHigh
    "5000",       // kUSBDeviceSpeedSuper
    "10000",      // kUSBDeviceSpeedSuperPlus
    "20000",      //  kUSBDeviceSpeedSuperPlusBy2
    "80000"};
bool verboseFlag = FALSE;
bool monitorFlag = TRUE;
bool jsonFlag    = FALSE;
bool firstTime = TRUE;
struct tm *local_time;

void formatTime(char *dt, time_t ttime) {
    local_time = localtime(&ttime);
    snprintf(dt, 20, "%4d/%02d/%02d %02d:%02d:%02d", 1900+local_time->tm_year, local_time->tm_mon+1, local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
}

void formatUSBPath(char *path, UInt32 locationID) {
    if (locationID == 0) {
        path[0] = '0';
        path[1] = '0';
        return;
    }
    char hex[25];
    snprintf(hex, 20, "%lx", (unsigned long)locationID);
    path[0] = hex[0];
    path[1] = hex[1];
    if (hex[2] == '0') {
        return;
    }
    path[2] = '-';
    int i = 2;
    int p = 3;
    while (TRUE) {
        path[p] = hex[i];
        i++;
        if (hex[i] == '0' || hex[i] == 0 || i > sizeof(hex)) {
            break;
        }
        p++;
        path[p] = '.';
        p++;
    }
    
}


static void print_cf_string(CFStringRef cf_string) {
    char buffer[256];
    CFStringGetCString(cf_string, (char *)&buffer, 256, CFStringGetSystemEncoding());
    printf("%s", buffer);
}
static void print_cf_number(CFNumberRef cf_number) {
    int number;
    // 0TODO rather test CFNumberType, than approximate to int
    CFNumberGetValue(cf_number, kCFNumberIntType, &number);
    printf("%d", number);
}

static void print_cf_type(CFTypeRef cf_type) {
    CFTypeID type_id;
    type_id = (CFTypeID) CFGetTypeID(cf_type);
    if (type_id == CFStringGetTypeID()) {
        print_cf_string(cf_type);
    } else if (type_id == CFNumberGetTypeID()) {
        print_cf_number(cf_type);
    } else {
        // unknown Type (TODO)
        printf("<");
        print_cf_string(CFCopyTypeIDDescription(type_id));
        printf(">");
    }
}

void print_properties(io_registry_entry_t entry) {
    CFMutableDictionaryRef properties;
    CFIndex count;
    CFTypeRef keys[100];
    CFTypeRef values[100];
    int i;
    IORegistryEntryCreateCFProperties(entry, &properties, kCFAllocatorDefault, kNilOptions);
    count     = CFDictionaryGetCount(properties);
    CFDictionaryGetKeysAndValues(properties,(const void **) &keys, (const void **) &values);
    for (i = 0; i < count; i++) {
        printf("\t");
        print_cf_type(keys[i]);
        printf(": ");
        print_cf_type(values[i]);
        printf("\n");
    }
    CFRelease(properties);
}

void printEvent(const char event[], USBDevice d ) {
    char path[40]; memset(&path,0,40);
    char timeString[20]; memset(&timeString,0,20);
    const char *usbStandard    = speedStandard[d.speed];
    const char *usbRate        = rate[d.speed];
    char host[40];
    formatUSBPath((char *)&path, d.locationID);
    formatTime((char *)&timeString,time(0));
    
    gethostname(host,40);
    if (jsonFlag) {
        printf("{\"Port:\":\"%s\",\"Serial\":\"%s\",\"Event\":\"%s\",\"bcdDevice\":\"%u\",\"Speed\":%s,\"Vendor\":\"%s\",\"Name\":\"%s\"},",
               path,d.uuid,event,d.bcdDevice, usbRate,d.deviceVendor,d.deviceName);
    } else {
        printf("%s %s:%s %s %u %s %s@%s USB:%s Rate:%sMbps\n",
               timeString, host, event, path, d.bcdDevice, d.deviceVendor,d.deviceName, d.uuid,usbStandard,usbRate);
    }
    fflush(stdout);
    //if (usbStandard) free(usbStandard);
    //if (usbRate) free(usbRate);
}

void SignalHandler(int sigraised) {
     exit(0);
}

void replaceString(char *original,char target,char replace) {
    for (int i=0;i<strlen(original);i++) {
        if (original[i] == target) original[i] = replace;
    }
}
void ltrim(char *original) {
    char *trim = original;
    size_t len = strlen(original);
    if (!len) return;
    int l = 0;
    while (l < len && isspace(trim[l]) ) l++;
    memcpy(original,trim+l,len-l+1);
}
void toString(char *buffer,CFTypeRef cf) {
    CFStringGetCString(cf, buffer, 50, CFStringGetSystemEncoding());
    ltrim(buffer);
    replaceString(buffer,' ','_');
}
UInt32 toLong(CFNumberRef cf) {
    UInt32 number;
    CFNumberGetValue(cf, kCFNumberLongType , &number);
    return number;
}
UInt8 toShort(CFNumberRef cf) {
    UInt8 number;
    CFNumberGetValue(cf, kCFNumberShortType , &number);
    return number;
}


