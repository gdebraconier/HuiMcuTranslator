#ifndef MAIN_H
#define MAIN_H

#include <CoreMIDI/CoreMIDI.h>
#include <stdio.h>
#include <mach/mach_time.h>
#include <pthread.h>

#define DEFAULTSKIP 3

struct Device {
    char*           deviceName;
    char*           packetTitle;
    char*           inEndpName;
    char*           outEndpName;
    MIDIPortRef     inPort;
    MIDIPortRef     outPort;
    MIDIPortRef     targetPort;
    MIDIEndpointRef srcEndp;
    MIDIEndpointRef destEndp;
    MIDIEndpointRef targetEndp;
    int             showTrackNames;
    int             showLcdDisplay;
    int             showTimecode;
    int             clickFlag;
    int             scrubFlag;
};

void createInputPortSource(        MIDIClientRef  client,
                                   CFStringRef    portName,
                                   MIDIReadProc   readProc,
                                   struct Device* device);

void createOutputPortsDestinations(MIDIClientRef client,
                                   CFStringRef outPort,
                                   CFStringRef targetPort,
                                   struct Device* device1,
                                   struct Device* device2);

int ptNote(const MIDIPacket* packet, int packetIndex, struct Device* device);

int ptAfter(const MIDIPacket* packet, int packetIndex, struct Device* device);

int ptCC(const MIDIPacket* packet, int packetIndex, struct Device* device);

// int ptPitchbend(const MIDIPacket* packet, int packetIndex, struct Device* device);

int ptSysex(const MIDIPacket* packet, int packetIndex, struct Device* device);

int sdNote(const MIDIPacket* packet, int packetIndex, struct Device* device);

// int sdAftertouch(const MIDIPacket* packet, int packetIndex, struct Device* device);

int sdCC(const MIDIPacket* packet, int packetIndex, struct Device* device);

int sdPitchbend(const MIDIPacket* packet, int packetIndex, struct Device* device);

// int sdSysex(const MIDIPacket* packet, int packetIndex, struct Device* device);

void showData(const MIDIPacket* packet, char* title);

void readProc(const MIDIPacketList* packetList, void* dev, void* alwaysNil);

char* getObjName(MIDIEndpointRef endp);

void sendMidi(Byte message[], int len, MIDIPortRef port, MIDIEndpointRef dest);

MIDIEndpointRef getSource(char* str);

MIDIEndpointRef getEndpoint(ItemCount (*func1)(), MIDIEndpointRef(*func2)(), char* str);

void sendHuiSwitch(Byte zone, Byte port, Byte flag, struct Device* device);

void sendHuiKnob(Byte zone, Byte port, struct Device* device);

void cpyarr(Byte* target, const MIDIPacket* packet, int ndx, int count);

int arrcmp(Byte* arr1, Byte* arr2, Byte len);

void* runLoopStop(void* arg);

void sendHuiPan(Byte number, Byte value, struct Device* device);

void sendHuiJog(Byte value, struct Device* device);

void sendMcuFader(Byte number1, Byte number2, Byte value1, struct Device* device);

void sendMcuPan(Byte number1, Byte value1, struct Device* device);

int sendMcuOther(Byte value1,
                 Byte value2,
                 const Byte huiTable[][8],
                 struct Device* device);

void showMcuTrackNames(const MIDIPacket* packet,
                       int packetIndex,
                       Byte sysexArr[],
                       struct Device* device);

void showMcuTimecode(const MIDIPacket* packet,
                     int packetIndex,
                     Byte sysexArr[],
                     struct Device* device);

void showMcuLcd(const MIDIPacket* packet,
                int packetIndex,
                Byte sysexArr[],
                struct Device* device);

void sendMcuClick(struct Device* device);

#endif