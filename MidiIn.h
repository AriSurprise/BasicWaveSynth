// MidiIn
// -- MIDI input framework
//    ** Uses C++11 **
// cs245 2/19

#ifndef CS245_MIDIIN_H
#define CS245_MIDIIN_H

#include <string>
#include <thread>
#include <portmidi.h>


class MidiIn {
  public:
    static std::string getDeviceInfo(void);
    MidiIn(int devno);
    void start();
    void stop();
    virtual ~MidiIn(void);
    virtual void onNoteOn(int channel, int note, int velocity) { }
    virtual void onNoteOff(int channel, int note) { }
    virtual void onPitchWheelChange(int channel, float value) { }
    virtual void onVolumeChange(int channel, int level) { }
    virtual void onModulationWheelChange(int channel, int value) { }
    virtual void onControlChange(int channel, int number, int value) { }
    virtual void onPatchChange(int channel, int value) { }
  private:
    PmStream *input_stream;
    std::thread *event_thread;
    bool process_events,
         thread_running;
    static void eventLoop(MidiIn *midiin_ptr);
};


#endif
