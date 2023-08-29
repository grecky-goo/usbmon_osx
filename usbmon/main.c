//
//  main.c
//  lsusb
//
//  Created by Greg Yamanishi on 8/19/23.
//

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include "device.h"
#include "util.h"
#include <time.h>


extern bool debugFlag;    // debug flag from util.h

mach_port_t masterPort;
io_registry_entry_t currentNode;
sig_t Handler;

static char timeString[20];
void SignalHandler(int sigraised);
io_registry_entry_t rootNode;
USBDevice d;


#define SWITCH(key) if (FALSE)
#define DEFAULT(msg)    } else msg;
#define CASE(k)   } else if (!strcmp(key, k)) {
#define IOUSBDEVICE        1
#define IOUSBINTERFACE     2
#define IOUSBROOTHUBDEVICE 3
#define STRCPY(val1,val2) { toString(val2); strcpy(val1,buf); free(buf); }
// load data from the registry entry -> USBDevice structure
void marshall(io_registry_entry_t entry) {
    CFMutableDictionaryRef properties;
    initializeUSBDevice(&d, -1);
    CFIndex count;
    CFTypeRef cfkeys[100];
    CFTypeRef cfvalues[100];
    char key[50];
    // extract registry entry data including properties ( key:value)
    IORegistryEntryGetName(entry, d.deviceName);
    replaceString(d.deviceName,' ','_');
    IORegistryEntryCreateCFProperties(entry, &properties, kCFAllocatorDefault, kNilOptions);
    count     = CFDictionaryGetCount(properties);
    CFDictionaryGetKeysAndValues(properties, (const void **)&cfkeys,(const void **)&cfvalues);
    // loop through the key:value pairs and load the sturcture
    for (int i = 0; i < count; i++) {
        memset(key,0,50);
        toString(key,cfkeys[i]);
        // SWITCH(key) {
        SWITCH(key) {
            // these paramters will change as different records appear.  They are not to be used in diff.
            CASE("IOClassNameOverride")  toString(d.IOClass,cfvalues[i]);
            CASE("bInterfaceClass")      d.Classid = toShort(cfvalues[i]);
            CASE("bInterfaceSubClass")   d.subClassid = toShort(cfvalues[i]);
            CASE("bcdDevice")            d.bcdDevice = toLong(cfvalues[i]);
            CASE("USB_Product_Name")     toString(d.deviceProduct,cfvalues[i]);
            CASE("USB_Vendor_Name")      toString(d.deviceVendor,cfvalues[i]);
            CASE("locationID")           d.locationID = toLong(cfvalues[i]);
            CASE("Built-In")             d.builtIn = CFBooleanGetValue(cfvalues[i]);
            CASE("Device_Speed")         d.speed = toShort(cfvalues[i]);
            CASE("USB_Serial_Number")    toString(d.uuid,cfvalues[i]);
        }
        
    } // for
    // DEBUG Messages
    PRINT_PROPERTIES(entry);
    PRINTF("MARSHALL location:%x,Type:%s,deviceName:%s,bcdDevice:%u,vendor:%s,product:%s,serial:%s,speed:%d\n",
           d.locationID,d.IOClass,d.deviceName,(unsigned int)d.bcdDevice, d.deviceVendor,
           d.deviceProduct,d.uuid,d.speed);
    // clean up
    CFDictionaryRemoveAllValues(properties);
    CFRelease(properties);
}

/*---------------------------------------------------------------------------------------
 *  registryEntry takes a registry entry, parses all the key/values in the entry and
 *  marshalls the data into a record.
 *--------------------------------------------------------------------------------------*/
static void registryEntry(io_registry_entry_t entry) {
    // Load the device info, into the USBDevice structure
    marshall(entry);
    if (d.locationID == -1)  return;        // There are IOService entries that we ignore
    
    char key[20];
    // Based on the type of entry (IOClass), do a diff of the information.
    strcpy(key,d.IOClass);
    SWITCH(key) {
        CASE("unknown")
        CASE("IOUSBRootHubDevice") {          // No diff on root hub because it never changes
            PRINTF("FOUND IOUSBRootHubDevice\n");
            if (!firstTime && !existB(d.locationID)) {
                printEvent("Add Device",d);
            }
            if (put(d) == ERROR) printf("Error saving ROOT Hub registry entry.\n");
            if (countDuplicates()) {
                printf("DUPLICATES after HUB\n");
                print_properties(entry);
                dumpDict();
                printDict();
                fflush(stdout);
                exit(0);
            }
        } // CASE IOUSBRootHubDevice
        CASE("IOUSBDevice") {
            PRINTF("FOUND IOUSBDevice\n");
            if (d.bcdDevice == 0)  {
                printf("IOUSBDevice has a BCDDevice=0\n");
                 print_properties(entry);
            }
            if (!firstTime && !existB(d.locationID)) {
                printEvent("Add Device   ",d);
            }
            if (put(d) == ERROR) printf("Error saving IODevice registry entry.\n");
            if (countDuplicates()) {
                printf("DUPLICATES after device *****\n");
                print_properties(entry);
                int dbug;
                if (get(&dbug,d.locationID)) {
                    print(dbug);
                }
                dumpDict();
                fflush(stdout);
                exit(0);
            }
        } // case IOUSBDevice
        CASE("IOUSBInterface") {
            PRINTF("FOUND IOUSBInterface\n");
            if (putInterface(d.locationID,d.Classid, d.subClassid)) {
                print_properties(entry);
                dumpDict();
                fflush(stdout);
                exit(0);
            }
        } // case IOUSBInterface
     } // switch
}
/*--------------------------------------------------------------------------
 * loadEntry Does a BFS transversaal of the IO registry (recursively)
 *--------------------------------------------------------------------------*/
static void loadRegistry(io_registry_entry_t entry) {
    io_name_t name;
    io_iterator_t childIterator;
    io_object_t child;
    IORegistryEntryGetName(entry, name);
    PRINTF("Starting %s \n", name);
    registryEntry( entry);
    IORegistryEntryGetChildIterator(entry, kIOServicePlane, &childIterator);
    IOObjectRelease(entry);
    while ((child = IOIteratorNext(childIterator))) {
        io_name_t name;
        IORegistryEntryGetName(child, name);
        loadRegistry(child);
    }
    IOObjectRelease(childIterator);
    PRINTF("Finishing %s \n", name);
}
/*--------------------------------------------------------------------------
 * findNode transverses the tree looking for a name.  Mainly used to find the starting
 * place in the tree
 *--------------------------------------------------------------------------*/
static io_registry_entry_t findNode(io_registry_entry_t entry, const io_name_t name) {
    io_name_t nodeName;
    IORegistryEntryGetName(entry, nodeName);
   // printf("checking name:%s  ",nodeName);
    if (strcmp(name,nodeName) == 0) {
        //printf("\nfound node:%s\n",name);
        return entry;
    }
    io_iterator_t childIterator;
    io_object_t child;
    IORegistryEntryGetChildIterator(entry, kIOServicePlane, &childIterator);
    IOObjectRelease(entry);
    while ((child = IOIteratorNext(childIterator))) {
        entry = findNode(child,name);
        if (!entry) continue;
        // We have found the entry so return
        return entry;
    }
    IOObjectRelease(childIterator);
    
    return 0;
}
/*--------------------------------------------------------------------------
 *  Upon control-c clean up all the data structures and exit.
 *--------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
 *  Print help text
 *----------------------------------------------------------------------*/
void usage(void) {
  printf("lsusb [-h|--help][-n | -j][-v] \n" \
    "\t-h|--help (optional) help\n" \
    "\t-n        (optional) do not monitor events\n" \
    "\t-j        (optional) output in json format\n" \
    "\t-v        (optional) output ALL apple USB events\n");
    printf("\nOutput Format:\n");
    printf("   Date     Time      Host       Event       Path  bcd#   Vendor       Name     Serial     Std     Rate \n");
    printf("2023/08/25 17:49:32 macbookpro:Remove Device 14-1 37412 GenesysLogic USB2.0_Hub@00000000 USB:2.0 Rate:480Mbps\n");
}

void monitor(void) {
    // copy current status dictionary to last dictionary.  clear current status.
    IOMainPort(MACH_PORT_NULL, &masterPort);
    rootNode = IORegistryGetRootEntry(masterPort);
    // locate the start of the USB tree.
    rootNode = findNode(rootNode, "AppleUSBLegacyRoot");
    clearDict();
    loadRegistry(rootNode);
    // check to see if anything was removed
    removals();
    // store the backup copy of the device dictionary
    makeBackup();

  }

int main( int argc, char *const *argv) {
    /*-------------------------------------------------------------------
     * Initialize data structures.
     *------------------------------------------------------------------*/
    Handler = signal( SIGINT, SignalHandler );
    if( Handler == SIG_ERR ) printf( "Warning control-c will not work. Handler setup failed.\n" );
    // prepare IORegistry access
    monitor();
    firstTime = FALSE;
    /*-------------------------------------------------------------------
     * parameter parsing
     *------------------------------------------------------------------*/
    int opt;
    bool helpFlag = FALSE;
    while((opt = getopt(argc,argv,"njvdh")) != -1) {
        switch (opt) {
                // Print out the usb device tree then exit
            case 'n':
                monitorFlag = FALSE;
                break;
            case 'j':
                printf("-j: output will be in json format.\n");
                jsonFlag = TRUE;
                break;
            case 'v':
                printf("-v: log all the apple event notices.");
                verboseFlag = TRUE;
                break;
            case 'd':
                debugFlag = TRUE;
                break;
            case 'h':
                helpFlag = TRUE;
                return  0;
          
            default:
                printf("unknown option:%c",opt);
                return 0;
        }  // switch
    }  // while
    if (jsonFlag && !monitorFlag) {
        printf("json output is only for monitoring.  Choose between -n and -j.\n");
    }
    /*-----------------------------------------------------------------
     * Handle ctrl-c
     *-----------------------------------------------------------------*/
     if (!monitorFlag) {
        printDict();
        return(0);
    }
    if (helpFlag) {
        usage();
        return(0);
    }
    if (!jsonFlag) {
        printDict();
        formatTime((char *)&timeString,time(0));
        printf("%s : --------- Begin USB Event Monitoring ------------------\n",timeString);
    }
    fflush(stdout);
    while (true) {
        monitor();
        sleep(10);
   }
    
    
}
