#include "main.h"

void readProc(const MIDIPacketList* packetList, void* dev, void* alwaysNil) {
    int nPacket, packetIndex, packetSkip, packetLength, first, type;
    struct Device* device = (struct Device*)dev;
    MIDIPacket* packet    = (MIDIPacket*)packetList->packet;
    int packetCount       = packetList->numPackets;
    int isFromPt          = (strcmp(device->deviceName, "pt") == 0);
//    int isFromSd          = (strcmp(device->deviceName, "sd") == 0);

    for (nPacket=0;nPacket<packetCount;nPacket++) {
        packetLength = packet->length;
        packetIndex  = 0;

        while (packetIndex < packetLength) {
            first = packet->data[packetIndex];
            type  = first >= 128 && first <= 143 ? 1 :    // note off
                    first >= 144 && first <= 159 ? 2 :    // note on
                    first >= 160 && first <= 175 ? 3 :    // aftertouch (for vu-meters)
                    first >= 176 && first <= 191 ? 4 :    // cc
                    first >= 224 && first <= 239 ? 5 :    // pitchbend
                    first == 240                 ? 6 :    // sysex
                    0;                                    // none of the above

            switch (type) {
                case 1:
                    packetSkip = isFromPt ? ptNote(packet, packetIndex, device) :
                                            sdNote(packet, packetIndex, device);
                    break;
                case 2:
                    packetSkip = isFromPt ? ptNote(packet, packetIndex, device) :
                                            sdNote(packet, packetIndex, device);
                    break;
                case 3:                     // sd doesn't send aftertouch
                    packetSkip = isFromPt ? ptAfter(packet, packetIndex, device) :
                                            DEFAULTSKIP; 
                    break;
                case 4:
                    packetSkip = isFromPt ? ptCC(packet, packetIndex, device) :
                                            sdCC(packet, packetIndex, device);
                    break;
                case 5:                     // pt doesn't send pitchbend
                    packetSkip = isFromPt ? DEFAULTSKIP :
                                            sdPitchbend(packet, packetIndex, device);
                    break;
                case 6:                     // sd doesn't send sysex
                    packetSkip = isFromPt ? ptSysex(packet, packetIndex, device) :
                                            DEFAULTSKIP; 
                    break;
                default:
                    packetSkip = DEFAULTSKIP;
            }
            packetIndex += packetSkip;
        }

//        showData(packet, device->packetTitle);
        packet = MIDIPacketNext(packet);
    }
}
