#include "main.h"

static const Byte huiTable[30][8] = {    // HUI: vertical: zone, horizontal: port; MCU: note on/off
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

int ptNote(const  MIDIPacket* packet, int packetIndex, struct Device* device) {
    if (packet->data[packetIndex + 1] == 0 && packet->data[packetIndex + 2 == 64]) {              // hui reply
        Byte message[3] = {144, 0, 127};
        sendMidi(message, 3, device->outPort, device->destEndp);
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
        sendMidi(message, 2, device->targetPort, device->targetEndp);
    }

    return DEFAULTSKIP;
}

int ptCC(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    Byte number1    = packet->data[packetIndex+1];
    Byte value1     = packet->data[packetIndex+2];
    Byte number2    = packet->length > 3 ? packet->data[packetIndex+4] : 0;
    Byte value2     = packet->length > 3 ? packet->data[packetIndex+5] : 0;
    int  packetSkip = DEFAULTSKIP;
    int  isWhat     = (number1 >= 0)  && (number1 <=7) && (number2 == number1 + 32) ? 1 :    // fader
                           (number1 == 12) && (number2 == 44) ? 2 :                               // other switch
                           (number1 >= 16) && (number1 <= 23) ? 3 : 0;                            // pan

    switch (isWhat) {
        case 1:        // fader
            sendMcuFader(number1, number2, value1, device);
            break;
        case 2:         // other switch
            packetSkip = sendMcuOther(value1, value2, huiTable, device);
            break;
        case 3:         // pan
            sendMcuPan(number1, value1, device);
            break;
    }

    return packetSkip;
}

/* int ptPitchbend(const MIDIPacket* packet, int packetIndex, struct Device* device){
    pt doesn't send pitchbend
} */

int ptSysex(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    Byte showWhat = packet->data[packetIndex+6];
    Byte sysexArr[7];

    cpyarr(sysexArr, packet, packetIndex, 7);

    switch (showWhat) {
        case 16:        // show track names
            showMcuTrackNames(packet, packetIndex, sysexArr, device);
            break;
        case 17:        // show timecode
            showMcuTimecode(packet, packetIndex, sysexArr, device);
            break;
        case 18:        // show lcd display
            showMcuLcd(packet, packetIndex, sysexArr, device);
            break;
    };

    while (packet->data[++packetIndex] != 247) {}

    return packetIndex;
}
