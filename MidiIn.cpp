/**
\file
  MidiIn.cpp
\brief
  Implementation for MidiIn framework testing portmidi functioning
\project
  (SP24) CS245 Assignment 5
\date
  3/2/2024
\author
  Ari Surprise (a.surprise@digipen.edu)
*/
#include "MidiIn.h"
#include <stdexcept>

/**
@brief
  Enumerate available midi devices
@return
  List of each available index & name formatted "#: name \n" per midi device
*/
std::string MidiIn::getDeviceInfo(void)
{
    Pm_Initialize();
    std::string result;
    int n = Pm_CountDevices();
    for (int i = 0; i < n; ++i) {
        const PmDeviceInfo* info = Pm_GetDeviceInfo(i);
        result += std::to_string(i) + ": " + info->name + "\n";
    }
    Pm_Terminate();
    return result;
}

/**
@brief
  Poll midi events after start() is called (thread requiring a static fn w/data)
@param midiin_ptr
  - MidiIn (pointer) launching the thread, to pass in for parent manipulation
*/
void MidiIn::eventLoop(MidiIn* midiin_ptr)
{
    constexpr float ratio16Bit = 2.0f / ((1 << 14) - 1);
    while (midiin_ptr->thread_running)
    {
        // Check that polling has been set to start()
        if (midiin_ptr->process_events)
        {
            // Check for any message
            if (Pm_Poll(midiin_ptr->input_stream)) {
                // Fetch & process the message
                union {
                    long signal;
                    unsigned char byte[4];
                } data;
                PmEvent event;
                Pm_Read(midiin_ptr->input_stream, &event, 1);
                data.signal = event.message;
                short cmd_n = data.byte[0] >> 4, // [0]nibble[0]
                    channel = data.byte[0] & 0x0F, // [0]nibble[1]
                    b1 = data.byte[1] & 0x7F,
                    b2 = data.byte[2] & 0x7F,
                    b12 = (((b2 << 7) + b1) - (1 << 13)); // 14bit combined val
                // Process signal per command number from byte[0]
                switch (cmd_n)
                {
                case 0x8: // NoteOff(key)
                    midiin_ptr->onNoteOff(channel, b1);
                    break;
                case 0x9: // NoteOn(key, velocity)
                    if (b2 == 0) // 0 velocity note on => note off
                    {
                        midiin_ptr->onNoteOff(channel, b1);
                        break;
                    }
                    midiin_ptr->onNoteOn(channel, b1, b2);
                    break;
              //case 0xA: // ~PolyKeyPressure(aftertouch_pressure)
                case 0xB: // ControlChange(control_num, value)
                    switch (b1) // added functions along control_num
                    {
                    case 1: // ModWheel
                        midiin_ptr->onModulationWheelChange(channel, b2);
                        break;
                    case 7: // Channel Volume
                        midiin_ptr->onVolumeChange(channel, b2);
                        break;
                    }
                    midiin_ptr->onControlChange(channel, b1, b2);
                    break;
                case 0xC: // ProgramChange(prog_num)
                    midiin_ptr->onPatchChange(channel, b1);
                    break;
              //case 0xD: // ~ChannelPressure(aftertouch_pressure)
                case 0xE: // PitchWheel(LSB+MSB)
                    midiin_ptr->onPitchWheelChange(channel, (b12 * ratio16Bit));
                    break;
                default:
                    throw std::runtime_error("Midi command "
                        + std::to_string(cmd_n) + "not recognized");
                }
            }
        }
    }
}

/**
@brief
  Midi Input device processing to poll for given device number's signal flow
@param devno
  Midi device enumerated by the platform (portmidi gets list when run sans args)
*/
MidiIn::MidiIn(int devno)
    : process_events(false), thread_running(false),
    input_stream(nullptr), event_thread(nullptr)
{
    Pm_Initialize();
    PmError value = Pm_OpenInput(&input_stream, devno, 0, 64, 0, 0);
    if (value != pmNoError) {
        Pm_Terminate();
        throw std::runtime_error("failed to open MIDI input device");
    }
    event_thread = new std::thread(eventLoop, this);
    if (!event_thread->joinable())
    {
        throw std::runtime_error("failed to open launch thread");
    }
    thread_running = true;
}

/**
@brief
  Stop MidiIn thread loop's running and clean up resources
@return
*/
MidiIn::~MidiIn(void)
{
    stop();
    thread_running = false;
    if (event_thread->joinable()) { event_thread->join(); }
    delete event_thread;
    Pm_Close(input_stream);
    Pm_Terminate();
}

/**
@brief
  Set the MidiIn even polling to begin
*/
void MidiIn::start()
{
    process_events = true;
}

/**
@brief
  Set the MidiIn even polling to pause
*/
void MidiIn::stop()
{
    process_events = false;
}
