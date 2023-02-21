#include "main.h"

static const Byte mcuTable[112][2] = {                                // MCU: note on/off; to HUI: {zone, port}
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

int sdNote(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    Byte value           = packet->data[packetIndex+1];
    Byte velocity        = packet->data[packetIndex+2];
    const int type       = value == 89 && velocity == 127  ? 1 :     // click
                           value == 101 && velocity == 127 ? 2 :     // scrub/shuttle
                           mcuTable[value][0] != 255       ? 3 :     // other switches
                           0;

    switch (type) {
        case 1:         // click
            sendMcuClick(device);
            break;
        case 2:          // scrub/shuttle
            switch (device->scrubFlag) {
                case 0:  // scrub
                    sendHuiSwitch(13, 5, 1, device);        
                    break;
                case 1:  // shuttle
                    sendHuiSwitch(13, 6, 1, device);        
                    break;
                case 2:  // off
                    sendHuiSwitch(14, 3, 1, device);        
                    break;
                default:
                    break;
            }
            device->scrubFlag = device->scrubFlag == 2 ? device->scrubFlag + 1 : 0;
            break;
        case 3:          // other switch
            sendHuiSwitch(mcuTable[value][0],
                          mcuTable[value][1],
                          velocity == 127 ? 1 : 0, device);
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
    const int type       = number >= 16 && number <= 23                ? 1 :
                           number == 60 && (value == 1 || value == 65) ? 2 :
                           0;
    
    switch (type) {
        case 1:
            sendHuiPan(number, value, device);
            break;
        case 2:
            sendHuiJog(value, device);
            break;
        default:
            break;
    }

    return DEFAULTSKIP;
}

int sdPitchbend(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    Byte channel     = packet->data[packetIndex] - 224;
    Byte low         = packet->data[packetIndex+1];
    Byte high        = packet->data[packetIndex+2];
    Byte message[18] = {176, 15, channel,
                        176, 47, 64,
                        176, channel, high,
                        176, channel+32, low,
                        176, 15, channel,
                        176, 47, 0};
    sendMidi(message, 18, device->targetPort, device->targetEndp);
    return DEFAULTSKIP;
}

/* int sdSysex(const MIDIPacket* packet, int packetIndex, struct Device* device) {
    sd doesn't send sysex
} */
