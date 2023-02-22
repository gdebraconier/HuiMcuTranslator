#include <CoreMIDI/CoreMIDI.h>
#include <stdio.h>
#include <mach/mach_time.h>
#include <pthread.h>

#define DEFAULTSKIP 3
#define NOTEOFF     128 ... 143
#define NOTEON      144 ... 159
#define AFTERTOUCH  160 ... 175
#define CC          176 ... 191
#define PITCHBEND   224 ... 239
#define SYSEX       240
#define CLICK       1
#define SCRUBTOGGLE 0
#define SCRUBOFF    0
#define SCRUB       1
#define SHUTTLE     2
#define MCUOTHER    3
#define MCUPAN      1
#define JOGWHEEL    2
#define HUIFADER    1
#define HUIOTHER    2
#define HUIPAN      3
#define HUITRACKS   16
#define HUITIMEC    17
#define HUILCD      18

struct Show {
    int trackNames;
    int lcdDisplay;
    int timeCode;
};

struct Flags {
    int click;
    int scrub;
    int flash;
};

struct Device {
    char*           deviceName;
    char*           packetTitle;
    char*           inEndpName;
    char*           outEndpName;
    MIDIPortRef     inPort;
    MIDIPortRef     outPort;
    MIDIEndpointRef srcEndp;
    MIDIEndpointRef destEndp;
    int             showTrackNames;
    int             showLcdDisplay;
    int             showTimecode;
    int             clickFlag;
    int             scrubFlag;
    int             flashFlag;
    struct Show*    show;
    struct Device*  target;
    struct Flags*   flags;
};

void createInputPortSource (MIDIClientRef  client,
                            CFStringRef    portName,
                            MIDIReadProc   readProc,
                            struct Device* device);

void createOutputPortsDests(MIDIClientRef client,
                            CFStringRef outPort,
                            CFStringRef targetPort,
                            struct Device* device);

int ptNote                 (const MIDIPacket* packet, int packetIndex, struct Device* device);
int ptAfter                (const MIDIPacket* packet, int packetIndex, struct Device* device);
int ptCC                   (const MIDIPacket* packet, int packetIndex, struct Device* device);
// int ptPitchbend(const MIDIPacket* packet, int packetIndex, struct Device* device); pt doesn't send pitchbend
int ptSysex                (const MIDIPacket* packet, int packetIndex, struct Device* device);
int sdNote                 (const MIDIPacket* packet, int packetIndex, struct Device* device);
// int sdAftertouch(const MIDIPacket* packet, int packetIndex, struct Device* device); sd doesn't send aftertouch
int sdCC                   (const MIDIPacket* packet, int packetIndex, struct Device* device);
int sdPB            (const MIDIPacket* packet, int packetIndex, struct Device* device);
// int sdSysex(const MIDIPacket* packet, int packetIndex, struct Device* device); sd doesn't send sysex
void showData              (const MIDIPacket* packet, char* deviceName);
void readProc              (const MIDIPacketList* packetList, void* dev, void* alwaysNil);
char* getObjName           (MIDIEndpointRef endp);
void sendMidi              (Byte message[], int len, MIDIPortRef port, MIDIEndpointRef dest);
MIDIEndpointRef getSource  (char* str);
MIDIEndpointRef getEndpoint(ItemCount (*func1)(), MIDIEndpointRef(*func2)(), char* str);
void sendHuiSwitch         (Byte zone, Byte port, Byte flag, struct Device* device);
void sendHuiKnob           (Byte zone, Byte port, struct Device* device);
void cpyarr                (Byte* target, const MIDIPacket* packet, int ndx, int count);
int arrcmp                 (Byte* arr1, Byte* arr2, Byte len);
void* runLoopStop          (void* arg);
void sendHuiPan            (Byte number, Byte value, struct Device* device);
void sendHuiJog            (Byte value, struct Device* device);
void sendMcuFader          (Byte number1, Byte number2, Byte value1, struct Device* device);
void sendMcuPan            (Byte number1, Byte value1, struct Device* device);
void flashButton           (Byte note, struct Device* device);
void checkFlash            (const MIDIPacket* packet, int packetIndex, struct Device* device);
void clearLCDDisplay       (struct Device* device);
void zeroFaders            (struct Device* device);
void centerPans            (struct Device* device);
void offSwitches           (struct Device* device);
void cleanSd               (struct Device* device);
int sendMcuOther           (Byte value1,
                            Byte value2,
                            const Byte huiTable[][8],
                            struct Device* device);
void showMcuTrackNames     (const MIDIPacket* packet,
                            int packetIndex,
                            Byte sysexArr[],
                            struct Device* device);
void showMcuTimecode       (const MIDIPacket* packet,
                            int packetIndex,
                            Byte sysexArr[],
                            struct Device* device);
void showMcuLcd            (const MIDIPacket* packet,
                            int packetIndex,
                            Byte sysexArr[],
                            struct Device* device);
void sendMcuClick          (struct Device* device);

/*
*
*
*   MAIN
*
*/
int main(int argc, char* argv[]) {
    pthread_t stopThread;
    MIDIClientRef midiClient;

    struct Show   sh;
    struct Flags  fl;
    struct Device pt;
    struct Device sd;
    
    sh.trackNames     = 1;
    sh.lcdDisplay     = 0;
    sh.timeCode       = 0;

    fl.click          = 0;
    fl.scrub          = 0;
    fl.flash          = 0;

    pt.deviceName     = "pt";
    pt.packetTitle    = "pt out: ";
    pt.outEndpName    = "PT OUT";
    pt.inEndpName     = "PT IN";
    pt.show           = &sh;
    pt.target         = &sd;
    pt.flags          = &fl;

    sd.deviceName     = "sd";
    sd.packetTitle    = "sd out: ";
    sd.outEndpName    = "SD OUT";
    sd.inEndpName     = "SD IN";
    sd.show           = &sh;
    sd.target         = &pt;
    sd.flags          = &fl;

    MIDIClientCreate(CFSTR("client"), NULL, NULL, &midiClient);

    createInputPortSource(midiClient, CFSTR("sdInput"), readProc, &sd);
    createInputPortSource(midiClient, CFSTR("ptInput"), readProc, &pt);

    createOutputPortsDests(midiClient, CFSTR("sdOutport"), CFSTR("ptOutport"), &sd);
    createOutputPortsDests(midiClient, CFSTR("ptOutport"), CFSTR("sdOutport"), &pt);

    pthread_create(&stopThread, NULL, runLoopStop, CFRunLoopGetCurrent());

    CFRunLoopRun();

    cleanSd(&sd);

    return 0;
}
/*
*
*
*   END OF MAIN
*
*/

void readProc(const MIDIPacketList* packetList, void* dev, void* alwaysNil) {

    int nPacket, packetIndex, packetSkip, packetLength;
    struct Device* device = (struct Device*)dev;
    MIDIPacket* packet    = (MIDIPacket*)packetList->packet;
    int packetCount       = packetList->numPackets;
    int isFromPt          = (strcmp(device->deviceName, "pt") == 0);

    for (nPacket=0;nPacket<packetCount;nPacket++) {
        packetLength = packet->length;
        packetIndex  = 0;

        while (packetIndex < packetLength) {
            switch (packet->data[packetIndex]) {
                case NOTEOFF:
                    packetSkip = isFromPt ?
                                 ptNote(packet, packetIndex, device) :
                                 sdNote(packet, packetIndex, device);
                    break;
                case NOTEON:
                    packetSkip = isFromPt ?
                                 ptNote(packet, packetIndex, device) :
                                 sdNote(packet, packetIndex, device);
                    break;
                case AFTERTOUCH:            
                    packetSkip = isFromPt ?
                                 ptAfter(packet, packetIndex, device) :
                                 DEFAULTSKIP;    // sd doesn't send aftertouch
                    break;
                case CC:
                    packetSkip = isFromPt ?
                                 ptCC(packet, packetIndex, device) :
                                 sdCC(packet, packetIndex, device);
                    break;
                case PITCHBEND:             
                    packetSkip = isFromPt ?
                                 DEFAULTSKIP :   // pt doesn't send pitchbend
                                 sdPB(packet, packetIndex, device);
                    break;
                case SYSEX:                 
                    packetSkip = isFromPt ?
                                 ptSysex(packet, packetIndex, device) :
                                 DEFAULTSKIP;    // sd doesn't send sysex
                    break;
                default:
                    packetSkip = DEFAULTSKIP;
            }
            packetIndex += packetSkip;
        }

        packet = MIDIPacketNext(packet);
    }
}

void createInputPortSource(MIDIClientRef client,
                           CFStringRef portName,
                           MIDIReadProc readProc,
                           struct Device* device) {

    MIDIInputPortCreate(client, portName, readProc, device, &device->inPort);

    device->srcEndp = getEndpoint(MIDIGetNumberOfSources,
                                  MIDIGetSource,
                                  device->outEndpName);

    MIDIPortConnectSource(device->inPort, device->srcEndp, NULL);
}

void createOutputPortsDests(MIDIClientRef client,
                                   CFStringRef outPort,
                                   CFStringRef targetPort,
                                   struct Device* device) {

    MIDIOutputPortCreate(client, outPort, &device->outPort);

    device->destEndp   = getEndpoint(MIDIGetNumberOfDestinations,
                                      MIDIGetDestination,
                                      device->inEndpName);    

    MIDIOutputPortCreate(client, targetPort, &device->target->outPort);

    device->target->destEndp = getEndpoint(MIDIGetNumberOfDestinations,
                                      MIDIGetDestination,
                                      device->target->inEndpName);
}

void clearLCDDisplays(struct Device* device) {
    Byte clearDisplayArr[18] = {240, 0, 0, 102, 20, 18, 0, 32, 32,
                                32, 32, 32, 32, 32, 32, 32, 32, 247};
    int n;

    for (n=0;n<16;n++) {
        clearDisplayArr[6] = n * 7;
        sendMidi(clearDisplayArr, 18, device->outPort, device->destEndp);
    }
}

void zeroFaders(struct Device* device) {
    int n;
    Byte message[3] = {0, 0, 0};

    for (n=0;n<8;n++) {
        message[0] = 224 + n;
        sendMidi(message, 3, device->outPort, device->destEndp);
    }
}

void centerPans(struct Device* device) {
    int n;
    Byte message[3] = {176, 0, 86};

    for (n=0;n<8;n++) {
        message[1] = n + 48;
        sendMidi(message, 3, device->outPort, device->destEndp);
    }
}

void offSwitches(struct Device* device) {
    int n;
    Byte message[3] = {128, 0, 64};

    for (n=0;n<240;n++) {
        message[1] = n;
        sendMidi(message, 3, device->outPort, device->destEndp);
    }
}

void cleanSd(struct Device* device) {
    clearLCDDisplays(device); // clear displays on streamdeck
    zeroFaders(device); // set all faders streamdeck to zero
    centerPans(device); // center all pans on streamdeck
    offSwitches(device); // turn of all switches on streamdeck
}

void showData(const MIDIPacket* packet, char* deviceName) {
    int k, len = packet->length;

    printf("%s", deviceName);

    for (k=0;k<len;k++) {
        printf("%d ", packet->data[k]);
    }

    printf("\n");
}

MIDIEndpointRef getEndpoint(ItemCount (*func1)(), MIDIEndpointRef(*func2)(), char* str) {
    ItemCount nSrcs = func1();
    ItemCount iSrc;
    MIDIEndpointRef src = 999;
    char* objName;

    for (iSrc=0;iSrc<nSrcs;iSrc++) {
        src = func2(iSrc);
        objName = getObjName(src);
        if (strstr(objName, str)) {
            break;
        }
    }

    if (src == 999) {
        exit(1);
    }

    return src;
}

char* getObjName(MIDIEndpointRef endp) {
    CFStringRef str;
    
    MIDIObjectGetStringProperty(endp, kMIDIPropertyName, &str);
    CFIndex len     = CFStringGetLength(str);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8);
    char* objName   = (char*)malloc(maxSize);
    CFStringGetCString(str, objName, maxSize, kCFStringEncodingUTF8);

    return objName;
}

void sendMidi(Byte message[], int len, MIDIPortRef port, MIDIEndpointRef dest) {
    Byte buffer[1024];
    MIDITimeStamp timeStamp    = mach_absolute_time();
    MIDIPacketList* packetList = (MIDIPacketList*)buffer;
    MIDIPacket* currentPacket  = MIDIPacketListInit(packetList);

    MIDIPacketListAdd(packetList, sizeof(buffer), currentPacket, timeStamp, len, message);
    MIDISend(port, dest, packetList);
}

void cpyarr(Byte* target, const MIDIPacket* packet, int ndx, int count) {
    int n;

    for(n=0;n<count;n++) {
        target[n] = packet->data[ndx + n];
    }
}

int arrcmp(Byte* arr1, Byte* arr2, Byte len) {
    Byte n;
    int retVal = 0;

    for (n=0;n<len;n++) {
        if (arr1[n] != arr2[n]) {
            retVal = 1;
            break;
        }
    }

    return retVal;
}

void flashButton(Byte note, struct Device* device) {
    Byte type       = device->flags->flash == 1 ? 144 : 128;
    Byte velocity   = device->flags->flash == 1 ? 127 : 0;
    Byte message[3] = {type, note, velocity};

    sendMidi(message, 3, device->target->outPort, device->target->destEndp);

}

void checkFlash(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    flashButton(101, device);
    flashButton(91, device);
    flashButton(92, device);
}

void* runLoopStop(void* arg) {
    char str[64];

    printf("Press <Enter> to stop:\n");
    fgets(str, 64, stdin);

    CFRunLoopStop((CFRunLoopRef)arg);

    return NULL;
}

void sendMcuClick(struct Device* device) {
    Byte clickMessage[3] = {device->flags->click == 1 ? 144 : 128,
                            89,
                            device->flags->click == 1 ? 127 : 64};
    sendMidi(clickMessage, 3, device->target->outPort, device->target->destEndp);
    sendHuiSwitch(21, 0, device->flags->click, device);
    device->flags->click = (-1 * device->flags->click) + 1;
}

void sendHuiSwitch(Byte zone, Byte port, Byte flag, struct Device* device) {
    Byte message[6] = {176, 15, zone, 176, 47, flag==1 ? port + 64 : port};
    sendMidi(message, 6, device->target->outPort, device->target->destEndp);
}

void sendHuiKnob(Byte zone, Byte port, struct Device* device) {
    Byte message[3] = {176, zone, port};
    sendMidi(message, 3, device->target->outPort, device->target->destEndp);
}

void sendHuiPan(Byte number, Byte value, struct Device* device) {
    Byte zone    = number + 64 - 16;
    Byte port    = value < 64 ? value + 64 : value - 64;
    Byte message[3] = {176, zone, port};
    sendMidi(message, 3, device->target->outPort, device->target->destEndp);
}

void sendHuiJog(Byte value, struct Device* device) {
    Byte port    = -1 * (value - 66);
    Byte message[3] = {176, 13, port};
    sendMidi(message, 3, device->target->outPort, device->target->destEndp);
}

void sendMcuFader(Byte number1, Byte number2, Byte value1, struct Device* device) {
    Byte faderMessage[3] = {number1 + 224, number2, value1};
    sendMidi(faderMessage, 3, device->target->outPort, device->target->destEndp);
}

void sendMcuPan(Byte number1, Byte value1, struct Device* device) {
    Byte panMessage[3]   = {176, number1 + 48 - 16, value1 + 16};
    sendMidi(panMessage, 3, device->target->outPort, device->target->destEndp);
}

int sendMcuOther(Byte value1, Byte value2, const Byte huiTable[][8], struct Device* device) {
    int packetSkip = DEFAULTSKIP;
    Byte portCheck = value2 < 64 ? value2 : value2 - 64;
    Byte switchMessage[3];
    if (huiTable[value1][portCheck] != 255) {
        switchMessage[0] = value2 < 64 ? 128 : 144;
        switchMessage[1] = value2 < 64 ?
                           huiTable[value1][value2] :
                           huiTable[value1][value2-64];
        switchMessage[2] = value2 < 64 ? 0 : 127;
        sendMidi(switchMessage, 3, device->target->outPort, device->target->destEndp);
        packetSkip = 6;
    }

    return packetSkip;
}

void showMcuTrackNames(const MIDIPacket* packet,
                       int packetIndex,
                       Byte sysexArr[],
                       struct Device* device) {
    Byte huiTrackNameArr[7]  = {240, 0, 0, 102, 5, 0, 16};
    Byte mcuTrackNameArr[12] = {240, 0, 0, 102, 20, 18, 0, 0, 0, 0, 0, 247};

    if (device->show->trackNames == 1 && (arrcmp(huiTrackNameArr, sysexArr, 7) == 0)) {
        mcuTrackNameArr[6] = packet->data[packetIndex+7] * 7;
        for (int n=7;n<11;n++) {
            mcuTrackNameArr[n] = packet->data[packetIndex+n+1];
            sendMidi(mcuTrackNameArr, 12, device->target->outPort, device->target->destEndp);
        }
    }
}

void showMcuTimecode(const MIDIPacket* packet,
                     int packetIndex,
                     Byte sysexArr[],
                     struct Device* device) {
    Byte huiTimecodeArr[7]   = {240, 0, 0, 102, 5, 0, 17};
    Byte mcuTimecodeArr[3]   = {176, 0, 0};

    if (device->show->timeCode&& (arrcmp(huiTimecodeArr, sysexArr, 7) == 0)) {
        int packetLength = packet->length;
        for (int n=0;n<packetLength-8;n++) {
            mcuTimecodeArr[1] = n + 64;
            mcuTimecodeArr[2] = packet->data[packetIndex+n+7];
            if (n==3 || n==5) {
                mcuTimecodeArr[2] += 112;
            }
            if (n==7) {
                mcuTimecodeArr[2] = n + 112;
            }
            sendMidi(mcuTimecodeArr, 3, device->target->outPort, device->target->destEndp);
        }
    }
}

void showMcuLcd(const MIDIPacket* packet,
                int packetIndex,
                Byte sysexArr[],
                struct Device* device) {
    Byte huiDisplayArr[7]    = {240, 0, 0, 102, 5, 0, 18};
    Byte mcuDisplayArr[18]   = {240, 0, 0, 102, 20, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 247};
    
    if (device->show->lcdDisplay == 1 && (arrcmp(huiDisplayArr, sysexArr, 7) == 0)) {
        mcuDisplayArr[6] = packet->data[packetIndex+7] * 10;
        for (int n=7;n<17;n++) {
            mcuDisplayArr[n] = packet->data[packetIndex+n+1];
            sendMidi(mcuDisplayArr, 18, device->target->outPort, device->target->destEndp);
        }
    }  
}

int sdNote(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    const Byte mcuTable[112][2] = {     // MCU: note on/off; to HUI: {zone, port}
        {  0,   7}, {  1,   7}, {  2,   7}, {  3,   7}, {  4,   7}, {  5,   7}, {  6,   7}, {  7,   7},         //  0  0x00
        {  0,   3}, {  1,   3}, {  2,   3}, {  3,   3}, {  4,   3}, {  5,   3}, {  6,   3}, {  7,   3},         //  8  0x08
        {  0,   2}, {  1,   2}, {  2,   2}, {  3,   2}, {  4,   2}, {  5,   2}, {  6,   2}, {  7,   2},         // 16  0x10
        {  0,   1}, {  1,   1}, {  2,   1}, {  3,   1}, {  4,   1}, {  5,   1}, {  6,   1}, {  7,   1},         // 24  0x18
        {  0,   5}, {  1,   5}, {  2,   5}, {  3,   5}, {  4,   5}, {  5,   5}, {  6,   5}, {  7,   5},         // 32  0x20
        {255, 255}, {255, 255}, {255, 255}, { 23,   1}, { 23,   0}, {255, 255}, { 10,   1}, { 10,   3},         // 40  0x28
        { 10,   0}, { 10,   2}, {255, 255}, {255, 255}, {255, 255}, {255, 255}, {255, 255}, {255, 255},         // 48  0x30
        {255, 255}, {255, 255}, {255, 255}, {255, 255}, {255, 255}, {255, 255}, {255, 255}, {255, 255},         // 56  0x38
        {255, 255}, {255, 255}, {255, 255}, {255, 255}, {255, 255}, {255, 255}, {  8,   1}, {  8,   5},         // 64  0x40
        {  8,   0}, {  8,   4}, { 24,   2}, { 24,   4}, { 24,   0}, { 24,   5}, { 24,   1}, { 24,   3},         // 72  0x48
        {  8,   7}, {  8,   3}, { 27,   7}, { 20,   0}, {255, 255}, {255, 255}, { 15,   3}, {255, 255},         // 80  0x50
        {255, 255}, { 21,   0}, {255, 255}, { 14,   1}, { 14,   2}, { 14,   3}, { 14,   4}, { 14,   5},         // 88  0x58
        { 13,   4}, { 13,   0}, { 13,   1}, { 13,   3}, { 13,   2}, { 13,   5}, { 13,   6}, {255, 255},         // 96  0x60
        {  0,   0}, {  1,   0}, {  2,   0}, {  3,   0}, {  4,   0}, {  5,   0}, {  6,   0}, {  7,   0}          // 104 0x68
    };

    Byte value           = packet->data[packetIndex+1];
    Byte velocity        = packet->data[packetIndex+2];
    const int type       = value == 89 && velocity == 127  ? CLICK :  
                           value == 101 && velocity == 127 ? SCRUBTOGGLE :  
                           mcuTable[value][0] != 255       ? MCUOTHER :  
                           0;

    switch (type) {
        case CLICK:
            sendMcuClick(device);
            break;
        case SCRUBTOGGLE: 
            switch (device->flags->scrub) {
                case SCRUBOFF:
                    sendHuiSwitch(13, 5, 1, device);
                    break;
                case SCRUB:  
                    sendHuiSwitch(13, 6, 1, device);    
                    break;
                case SHUTTLE:
                    sendHuiSwitch(14, 3, 1, device);        
                    break;
                default:
                    break;
            }
            device->flags->scrub = device->flags->scrub != SHUTTLE ? device->flags->scrub + 1 : SCRUBOFF;
//            device->target->scrubFlag = device->flags->scrub;
            break;
        case MCUOTHER:      
            sendHuiSwitch(mcuTable[value][0],
                          mcuTable[value][1],
                          velocity == 127 ? 1 : 0,
                          device);
            break;
        default:
            break;
    }

    return DEFAULTSKIP;
}

/* int sdAftertouch(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    sd doesn't send aftertouch
} */

int sdCC(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    Byte number          = packet->data[packetIndex+1];
    Byte value           = packet->data[packetIndex+2];
    const int type       = number >= 16 && number <= 23                ? MCUPAN :
                           number == 60 && (value == 1 || value == 65) ? JOGWHEEL :
                           0;
    
    switch (type) {
        case MCUPAN:
            sendHuiPan(number, value, device);
            break;
        case JOGWHEEL:
            sendHuiJog(value, device);
            break;
        default:
            break;
    }

    return DEFAULTSKIP;
}

int sdPB(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    Byte channel     = packet->data[packetIndex] - 224;
    Byte low         = packet->data[packetIndex+1];
    Byte high        = packet->data[packetIndex+2];
    Byte message[18] = {176, 15, channel,
                        176, 47, 64,
                        176, channel, high,
                        176, channel+32, low,
                        176, 15, channel,
                        176, 47, 0};
    sendMidi(message, 18, device->target->outPort, device->target->destEndp);
    return DEFAULTSKIP;
}

/* int sdSysex(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    sd doesn't send sysex
} */

int ptNote(const  MIDIPacket* packet, int packetIndex, struct Device* device) {
    if (packet->data[packetIndex + 1] == 0 && packet->data[packetIndex + 2 == 64]) {          // hui reply
        Byte message[3] = {144, 0, 127};
        sendMidi(message, 3, device->outPort, device->destEndp);

        if (device->flags->scrub == SHUTTLE) {
            checkFlash(packet, packetIndex, device);
            device->flags->flash = (device->flags->flash == 0) ? 1 : 0;
//            device->target->flashFlag = device->flags->flash;
        }
    }

    return DEFAULTSKIP;
}

int ptAfter(const MIDIPacket* packet, int packetIndex, struct Device* device) {              // vu meters
    Byte pp = 208;
    Byte channel = packet->data[packetIndex+1];
    Byte side = packet->data[packetIndex+2] >> 4 & 15;
    Byte value = packet->data[packetIndex+2] & 15;

    if (value < (channel * 16)) {
        Byte message[2] = {pp, (channel * 16) + value + 1}; 
        sendMidi(message, 2, device->target->outPort, device->target->destEndp);
    }

    return DEFAULTSKIP;
}

int ptCC(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    const Byte huiTable[30][8] = {    // HUI: vertical: zone, horizontal: port; MCU: note on/off
        {104,  24,  16,   8, 255,  32, 255,   0},       //  0 0x00
        {105,  25,  17,   9, 255,  33, 255,   1},       //  1 0x01
        {106,  26,  18,  10, 255,  34, 255,   2},       //  2 0x02
        {107,  27,  19,  11, 255,  35, 255,   3},       //  3 0x03
        {108,  28,  20,  12, 255,  36, 255,   4},       //  4 0x04
        {109,  29,  21,  13, 255,  37, 255,   5},       //  5 0x05
        {110,  30,  22,  14, 255,  38, 255,   6},       //  6 0x06
        {111,  31,  23,  15, 255,  39, 255,   7},       //  7 0x07
        { 72,  70, 100,  81,  73,  71, 255,  80},       //  8 0x08
        {255, 255, 255, 255, 255, 255, 255, 255},       //  9 0x09
        { 48,  46,  49,  47, 255, 255, 255, 255},       // 10 0x0a
        {255, 255, 255, 255, 255, 255, 255, 255},       // 11 0x0b
        {255, 255, 255, 255, 255, 255, 255, 255},       // 12 0x0c
        { 97,  98, 100,  99,  96, 101, 101, 255},       // 13 0x0d
        { 83,  91,  92,  93,  94,  95, 255, 255},       // 14 0x0e
        {255, 255, 255,  86, 255, 255, 255, 255},       // 15 0x0f
        {255, 255, 255, 255, 255, 255, 255, 255},       // 16 0x10
        {255, 255, 255, 255, 255, 255, 255, 255},       // 17 0x11
        {255, 255, 255, 255, 255, 255, 255, 255},       // 18 0x12
        {255, 255, 255, 255, 255, 255, 255, 255},       // 19 0x13
        { 83, 255, 255, 255, 255, 255, 255, 255},       // 20 0x14
        { 89, 255, 255, 255, 255, 255, 255, 255},       // 21 0x15
        {255, 255, 255, 255, 255, 255, 255, 255},       // 22 0x16
        { 43,  42, 255, 255,  41, 255, 255, 255},       // 23 0x17
        { 76,  78,  74,  79,  75,  77, 255, 255},       // 24 0x18
        {255, 255, 255, 255, 255,  79, 255, 255},       // 25 0x19
        {255, 255, 255, 255, 255, 255, 255, 255},       // 26 0x1a
        { 54,  55,  56,  57,  58,  59,  60,  61},       // 27 0x1b
        {255, 255, 255, 255, 255, 255, 255, 255},       // 28 0x1c
        {255, 255, 255, 255, 255, 255, 255, 255}        // 29 0x1d
    };

    Byte number1    = packet->data[packetIndex+1];
    Byte value1     = packet->data[packetIndex+2];
    Byte number2    = packet->length > 3 ? packet->data[packetIndex+4] : 0;
    Byte value2     = packet->length > 3 ? packet->data[packetIndex+5] : 0;
    int  packetSkip = DEFAULTSKIP;
    int  type       = (number1 >= 0)  && (number1 <=7) && (number2 == number1 + 32) ? HUIFADER :    // fader
                      (number1 == 12) && (number2 == 44) ? HUIOTHER :                               // other switch
                      (number1 >= 16) && (number1 <= 23) ? HUIPAN : 0;                              // pan

    switch (type) {
        case HUIFADER:         // fader
            sendMcuFader(number1, number2, value1, device);
            break;
        case HUIOTHER:         // other switch
            packetSkip = sendMcuOther(value1, value2, huiTable, device);
            break;
        case HUIPAN:           // pan
            sendMcuPan(number1, value1, device);
            break;
    }

    return packetSkip;
}

/* int ptPitchbend(const MIDIPacket* packet, int packetIndex, struct Device* device){
    pt doesn't send pitchbend
} */

int ptSysex(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    Byte type = packet->data[packetIndex+6];
    Byte sysexArr[7];

    cpyarr(sysexArr, packet, packetIndex, 7);

    switch (type) {
        case HUITRACKS:     // show track names
            showMcuTrackNames(packet, packetIndex, sysexArr, device);
            break;
        case HUITIMEC:      // show timecode
            showMcuTimecode(packet, packetIndex, sysexArr, device);
            break;
        case HUILCD:        // show lcd display
            showMcuLcd(packet, packetIndex, sysexArr, device);
            break;
    };

    while (packet->data[++packetIndex] != 247) {}

    return packetIndex;
}
