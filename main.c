#include "main.h"

int main(void) {
    pthread_t stopThread;
    MIDIClientRef midiClient;

    struct Device pt;
    pt.deviceName     = "pt";
    pt.packetTitle    = "pt out: ";
    pt.outEndpName    = "PT OUT";
    pt.inEndpName     = "PT IN";
    pt.showTrackNames = 1;
    pt.showLcdDisplay = 0;
    pt.showTimecode   = 0;
    pt.clickFlag      = 0;
    pt.scrubFlag      = 0;

    struct Device sd;
    sd.deviceName     = "sd";
    sd.packetTitle    = "sd out: ";
    sd.outEndpName    = "SD OUT";
    sd.inEndpName     = "SD IN";
    sd.clickFlag      = 0;
    sd.scrubFlag      = 0;

    MIDIClientCreate(CFSTR("client"), NULL, NULL, &midiClient);

    createInputPortSource(midiClient, CFSTR("sdInput"), readProc, &sd);
    createInputPortSource(midiClient, CFSTR("ptInput"), readProc, &pt);

    createOutputPortsDestinations(midiClient, CFSTR("sdOutport"), CFSTR("sdTargetPort"), &sd, &pt);
    createOutputPortsDestinations(midiClient, CFSTR("ptOutport"), CFSTR("ptTargetPort"), &pt, &sd);

    pthread_create(&stopThread, NULL, runLoopStop, CFRunLoopGetCurrent());

    CFRunLoopRun();

    return 0;
}
