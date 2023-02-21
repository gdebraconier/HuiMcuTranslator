#include "main.h"

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

void createOutputPortsDestinations(MIDIClientRef client,
                                   CFStringRef outPort,
                                   CFStringRef targetPort,
                                   struct Device* device1,
                                   struct Device* device2) {
    MIDIOutputPortCreate(client, outPort, &device1->outPort);
    device1->destEndp   = getEndpoint(MIDIGetNumberOfDestinations,
                                      MIDIGetDestination,
                                      device1->inEndpName);    
    MIDIOutputPortCreate(client, targetPort, &device1->targetPort);
    device1->targetEndp = getEndpoint(MIDIGetNumberOfDestinations,
                                      MIDIGetDestination,
                                      device2->inEndpName);
}

void showData(const MIDIPacket* packet, char* title) {
    int k, len = packet->length;

    printf("%s", title);

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

void* runLoopStop(void* arg) {
    char str[64];

    printf("Press <Enter> to stop:\n");
    fgets(str, 64, stdin);

    CFRunLoopStop((CFRunLoopRef)arg);

    return NULL;
}

void sendMcuClick(struct Device* device) {
    Byte clickMessage[3] = {device->clickFlag == 1 ? 144 : 128,
                            89,
                            device->clickFlag == 1 ? 127 : 64};
    sendMidi(clickMessage, 3, device->targetPort, device->targetEndp);
    sendHuiSwitch(21, 0, device->clickFlag, device);
    device->clickFlag = -device->clickFlag + 1;
}

void sendHuiSwitch(Byte zone, Byte port, Byte flag, struct Device* device) {
    Byte message[6] = {176, 15, zone, 176, 47, flag==1?port+64:port};
    sendMidi(message, 6, device->targetPort, device->targetEndp);
}

void sendHuiKnob(Byte zone, Byte port, struct Device* device) {
    Byte message[3] = {176, zone, port};
    sendMidi(message, 3, device->targetPort, device->targetEndp);
}

void sendHuiPan(Byte number, Byte value, struct Device* device) {
    Byte zone    = number + 64 - 16;
    Byte port    = value < 64 ? value + 64 : value - 64;
    Byte message[3] = {176, zone, port};
    sendMidi(message, 3, device->targetPort, device->targetEndp);
}

void sendHuiJog(Byte value, struct Device* device) {
    Byte port    = -1 * (value - 66);
    Byte message[3] = {176, 13, port};
    sendMidi(message, 3, device->targetPort, device->targetEndp);
}

void sendMcuFader(Byte number1, Byte number2, Byte value1, struct Device* device) {
    Byte faderMessage[3] = {number1 + 224, number2, value1};
    sendMidi(faderMessage, 3, device->targetPort, device->targetEndp);
}

void sendMcuPan(Byte number1, Byte value1, struct Device* device) {
    Byte panMessage[3]   = {176, number1 + 48 - 16, value1 + 16};
    sendMidi(panMessage, 3, device->targetPort, device->targetEndp);
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
        sendMidi(switchMessage, 3, device->targetPort, device->targetEndp);
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

    if (device->showTrackNames == 1 && (arrcmp(huiTrackNameArr, sysexArr, 7) == 0)) {
        mcuTrackNameArr[6] = packet->data[packetIndex+7] * 7;
        for (int n=7;n<11;n++) {
            mcuTrackNameArr[n] = packet->data[packetIndex+n+1];
            sendMidi(mcuTrackNameArr, 12, device->targetPort, device->targetEndp);
        }
    }
}

void showMcuTimecode(const MIDIPacket* packet,
                     int packetIndex,
                     Byte sysexArr[],
                     struct Device* device) {
    Byte huiTimecodeArr[7]   = {240, 0, 0, 102, 5, 0, 17};
    Byte mcuTimecodeArr[3]   = {176, 0, 0};

    if (device->showTimecode == 1 && (arrcmp(huiTimecodeArr, sysexArr, 7) == 0)) {
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
            sendMidi(mcuTimecodeArr, 3, device->targetPort, device->targetEndp);
        }
    }
}

void showMcuLcd(const MIDIPacket* packet,
                int packetIndex,
                Byte sysexArr[],
                struct Device* device) {
    Byte huiDisplayArr[7]    = {240, 0, 0, 102, 5, 0, 18};
    Byte mcuDisplayArr[18]   = {240, 0, 0, 102, 20, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 247};
    
    if (device->showLcdDisplay == 1 && (arrcmp(huiDisplayArr, sysexArr, 7) == 0)) {
        mcuDisplayArr[6] = packet->data[packetIndex+7] * 10;
        for (int n=7;n<17;n++) {
            mcuDisplayArr[n] = packet->data[packetIndex+n+1];
            sendMidi(mcuDisplayArr, 18, device->targetPort, device->targetEndp);
        }
    }  
}