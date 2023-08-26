/*========================================================================
 Name        : device.c
 Author      : Greg Yamanishi
 Version     :
 Copyright   : Your copyright notice
 Description : Maintain a dictionary of devices, in a sorted doubly-linked list
 ==========================================================================*/
#include "device.h"
#include "util.h"

#define ABS(N) ((N<0)?(-N):(N))
extern bool debugFlag;
unsigned long count = 1;
USBDevice dict[100];
int dfree = -1;
int dfirst = -1;
int dcount = -1;
USBDevice backup[100];
int bfree  = -1;
int bfirst = -1;
int bcount = -1;
const USBDevice init = {
    .next           = 0,
    .prev           = 0,
    .timestamp      = "",
    .builtIn        = FALSE,
    .bcdDevice      = 0,
    .locationID     = 0,
    .speed          = 0,
    .Classid        = 0,
    .subClassid     = 0,
    .IOClass        ="unknown",
    .uuid           ="N/A",
    .deviceName     ="N/A",
    .deviceProduct  ="N/A",
    .deviceVendor   ="N/A",
    .deviceSerial   ="N/A",
    .classID        = {0,0,0,0,0,0,0,0,0,0},
    .subclassID     = {0,0,0,0,0,0,0,0,0,0},
    .terminateCount = 0,
    .addCount       = 0
};
/*------------------------------------------------------------
 *  Aallocator for the device stucture
 *------------------------------------------------------------*/
int newDevice(USBDevice d) {
    if (dfree < 0) {
        dfirst = 0;
        dfree = 0;
        dcount = 0;
    }
    cpyUSBDevice(&dict[dfree], d);
    dict[dfree].prev = -1;
    dict[dfree].next = -1;
    dfree++;
    dcount++;
    return dfree-1;
}

void cpyUSBDevice(USBDevice *out, USBDevice in) {
    out->next                 = in.next;
    out->prev                 = in.prev;
    strcpy(out->timestamp,    in.timestamp);
    out->builtIn              = in.builtIn;
    out->bcdDevice            = in.bcdDevice;
    out->locationID           = in.locationID;
    out->speed                = in.speed;
    out->Classid              = in.Classid;
    out->subClassid           = in.subClassid;
    strcpy(out->IOClass,      in.IOClass);
    strcpy(out->uuid,         in.uuid);
    strcpy(out->deviceName,   in.deviceName);
    strcpy(out->deviceProduct,in.deviceProduct);
    strcpy(out->deviceVendor, in.deviceVendor);
    strcpy(out->deviceSerial, in.deviceSerial);
    for (int i=0;i<10;i++) {
        out->classID[i] = in.classID[i];
        out->subclassID[i] = in.subclassID[i];
    }
    out->terminateCount       = in.terminateCount;
    out->addCount             = in.addCount;
}
void initializeUSBDevice(USBDevice *d, UInt32 locationID) {
    PRINTF("usbdevice size:%lu\n",sizeof(USBDevice)*100);
    count++;
    memcpy(d,&init,sizeof(init));
}

#define SWITCH(key) if (FALSE)
#define DEFAULT(msg)
#define CASE(k)   } else if (k) {
enum rcode putInterface (UInt32 locationID,UInt8 classID,UInt8 subclassID) {
    int e;
    enum rcode r;
    r = get(&e,locationID);
    if (r != EXISTING) {
        printf("Device was not created before interface record\n");
        return r;
    } // for
    if (dict[e].bcdDevice == 0)  {
        printf("IOUSBInterface has a 0 BCDDevice\n");
        return ERROR;
    }
    for (int i=0;i<20;i++) {
        // zero signifys the end of the list because ther is no interface classid=0
        if (dict[e].classID[i] == 0 || (dict[e].classID[i] == classID && dict[e].subclassID[i] == subclassID))  {
            dict[e].classID[i]    = classID;
            dict[e].subclassID[i] = subclassID;
            break;
        }
    }
    return EXISTING;
}
enum rcode put(USBDevice d) {
    int current = -1;
    int r       = 0;
    PRINTF("PUT location:0x%x ",d.locationID);
    /*---------------------------------------------------------------------
     * If head is Null, then init the list.
     *--------------------------------------------------------------------*/
    // if empty then fill the record.
    if (dfirst < 0) {
        int new = newDevice(d);
        PRINTF("Adding First node:%d location:0x%x result==>",new,d.locationID);
        return FIRST;
    }
    current = dfirst;
    while (current != -1) {
        // the location exists, so update the payload
        SWITCH() {
            // Found the one we are looking for
            CASE (dict[current].locationID == d.locationID) {
                cpyUSBDevice(&dict[current],d);
                r = EXISTING;
                PRINTF("EXISTING.  found:%x result==>", d.locationID);
                break;
            }
            // if we are inserting at the front of the list (push)
            CASE (dict[current].prev == -1 && (dict[current].locationID > d.locationID)) {
                int new = newDevice(d);
                dict[new].prev = -1;
                dict[new].next = current;
                dict[current].prev = new;
                dfirst = new;
                int n = dict[dfirst].next;
                current = new;
                PRINTF("PUSH first node.  first:%x next:%x result==>", dict[dfirst].locationID, dict[n].locationID);
                dcount++;
                r = PUSH;
                break;
            }
            
            // Inserting the new node in between two existing nodes
            CASE((dict[current].prev != -1) && (dict[current].locationID > d.locationID)) {
                int new = newDevice(d);
                int previous = dict[current].prev;
                dict[previous].next = new;
                dict[current].prev = new;
                dict[new].prev     = previous;
                dict[new].next      = current;
                current = new;
                dcount++;
                PRINTF("INSERT.  prev:%x new:%x next:%x result==>", dict[previous].locationID, dict[new].locationID, dict[current].locationID);
                r = INSERT;
                break;
            }
            // Add to the end of the list
            CASE(dict[current].next == -1) {
                int new = newDevice(d);
                dict[current].next = new;
                dict[new].prev     = current;
                dict[new].next = -1;
                current=new;
                dcount++;
                int p = dict[new].prev;
                PRINTF("END.  prev:%x new:%x  result==>", dict[p].locationID, dict[new].locationID);
                r = END;
                break;
            }
            DEFAULT()
        }
        current  = dict[current].next;
    } // while
    if (debugFlag) {
        int check;
        if ((get(&check, d.locationID)) || check != current) {
            print(current);
            dumpDict();
        };
    }
    return r;
}

enum rcode get(int *current,UInt32 locationID) {
    USBDevice d; initializeUSBDevice(&d,locationID);
    enum rcode r;
    /*---------------------------------------------------------------------
     * If head is Null, then init the list.
     *--------------------------------------------------------------------*/
    if (dfirst == -1) {
        dfirst = 0;
        dfree  = 1;
        *current = 0;
        PRINTF("Adding First node location:0x%x\n",locationID);
        return FIRST;
    }
    PRINTF("Get location:0x%1x ",locationID);
    *current = dfirst;
    /*------------------------------------------------------------------
     * check to see if the list is empty, if is add blank entry and return
     *-----------------------------------------------------------------*/
    for (int i=0;i<1000 && *current!=-1;i++) {
        SWITCH() {
            CASE (dict[*current].locationID == locationID) {
                PRINTF("#%d found node location:0x%x\n",i,locationID);
                r = EXISTING;
                break;
            }
            // if we are inserting at the front of the list (push)
            CASE (dict[*current].prev == -1 && (dict[*current].locationID > locationID)) {
                PRINTF("#%d previous location:-1 current location:0x%1x target:0x%1x\n",i,dict[*current].locationID,locationID);
                int new = newDevice(d);
                dict[new].prev      = -1;
                dict[new].next      = *current;
                dict[*current].prev = new;
                dfirst = new;
                dcount++;
                r = PUSH;
                break;
            }
            // Inserting the new node in between two existing nodes
            CASE (dict[*current].locationID > locationID) {
                int new = newDevice(d);
                int prev = dict[*current].prev;
                dict[prev].next = new;
                dict[new].prev  = prev;
                dict[new].next  = *current;
                dict[*current].prev = new;
                PRINTF("#%d inserting new in the middle.  prev:%x new:%x next:%x\n",
                       i,dict[prev].locationID, dict[new].locationID, dict[*current].locationID);
                r =INSERT;
                break;
            }
            // Add At the end.
            CASE (dict[*current].next == -1) {
                int new = newDevice(d);
                dict[*current].next = new;
                dict[new].prev = *current;
                dict[new].next = -1;
                PRINTF("#%d END.  \n",i);
                r = END;
                break;
            }
            DEFAULT() {
            }
        }
        //switch
        *current  = dict[*current].next;
    }
    return 0;
}
bool exist(UInt32 locationID) {
    int current = dfirst;
    /*------------------------------------------------------------------
     * check to see if the list is empty, if is add blank entry and return
     *-----------------------------------------------------------------*/
    for (int i=0;i<1000 && current!=-1;i++) {
        if (dict[current].locationID == locationID) return TRUE;
        current  = dict[current].next;
    }
    return FALSE;
}
bool existB(UInt32 locationID) {
    int current = bfirst;
    /*------------------------------------------------------------------
     * check to see if the list is empty, if is add blank entry and return
     *-----------------------------------------------------------------*/
    for (int i=0;i<1000 && current!=-1;i++) {
        if (backup[current].locationID == locationID) return TRUE;
        current  = backup[current].next;
    }
    return FALSE;
}

void printDict(void) {
    int current = dfirst;
    PRINTF("*********************** There are %d nodes\n",dcount);
    for (int i=0;i<dfree && current!=-1;i++) {
        print(current);
        current  = dict[current].next;
    }
    PRINTF("***********************\n");
}
void printBackup(void) {
    int current = bfirst;
    PRINTF("*********************** There are %d nodes\n",dcount);
    for (int i=0;i<bfree && current!=-1;i++) {
        printf("\t"); print(current);
        current  = backup[current].next;
    }
    PRINTF("***********************\n");
}
void removals(void ) {
    int current = bfirst;
    PRINTF("*********************** There are %d nodes\n",dcount);
    // int count = 0;
    for (int i=0;i<1000 && current!=-1;i++) {
        if (!exist(backup[current].locationID)) {
            printEvent("Remove Device",backup[current]);
        }
        current = backup[current].next;
    }
    PRINTF("***********************\n");
}
void print(int i) {
    char path[40]; memset(&path,0,40);
    formatUSBPath((char *)&path,dict[i].locationID);
    const char *usbStandard    = speedStandard[dict[i].speed];
    const char *usbRate        = rate[dict[i].speed];
    /*----------------------------------------------------------------------------
     * skip the first node, which is always empty.
     *----------------------------------------------------------------------------*/
    printf("%-15s %-6u %s %s@%s USB:%s Rate:%sMbps\n", path, (unsigned int)dict[i].bcdDevice, dict[i].deviceVendor,dict[i].deviceName, dict[i].uuid,usbStandard,usbRate);
}


void makeBackup(void) {
    bfirst = dfirst;
    bcount = dcount;
    bfree = dfree;
    for (int i=0;i<100;i++) {
        cpyUSBDevice(&backup[i],dict[i]);
    }
}

void clearDict(void) {
    dfirst = -1;
    dcount = -1;
    dfree = -1;
    for (int i=0;i<100;i++) {
        memcpy(&dict[i],&init,sizeof(init));
    }
}

int countDuplicates(void) {
    int count = 0;
    int current = dfirst;
    if (current) {
        for (int i=0;i<1000 && current!=-1;i++) {
            int scan = dict[current].next;
            for (int j=0;j<1000 && scan != -1;j++) {
                if (dict[current].locationID == dict[scan].locationID) count++;
                scan = dict[scan].next;
            }
            current = dict[current].next;
        }
    }
    return count;;
}

void dumpDict(void) {
    printf("DUMP of dict first:%d free:%d count:%d\n",dfirst,dfree,dcount);
    for (int i=0;i<dfree;i++) {
        printf("index:%d prev:%d next%d contents:", i, dict[i].prev, dict[i].next);
        print(i);
    }
}
