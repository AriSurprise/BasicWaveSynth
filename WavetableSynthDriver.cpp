// WavetableSynthDriver.cpp
// -- driver for the WavetableSynth class
// cs245 2024.03
//
// usage:
//   WavetableSynthDriver [<devno>] [<rate>]
// where:
//   <devno> -- (optional) is the device number to use for MIDI input.
//              If not specified, a list of device is displayed.
//   <rate>  -- (optional) sampling rate for the synthesizer output
//
// To compile from the Visual Studio 2015 command prompt:
//   cl /EHsc /Iinclude WavetableSynthDriver.cpp WavetableSynth.cpp
//      cs245_proj2.lib portaudio_x86.lib portmidi.lib pthreadVC2.lib
//
// From the Linux command line:
//   g++ -I include WavetableSynthDriver.cpp WavetableSynth.cpp AudioData.cpp
//       Wave.cpp Resample.cpp MidiIn.cpp ADSR.cpp -lportaudio -lportmidi -pthread

#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <portaudio.h>
#include "WavetableSynth.h"
using namespace std;


/////////////////////////////////////////////////////////////////
// PortAudio callback
/////////////////////////////////////////////////////////////////
int onWrite(const void *vin, void *vout, unsigned long frames,
            const PaStreamCallbackTimeInfo *tinfo,
            PaStreamCallbackFlags flags, void *user) {
  float *out = reinterpret_cast<float*>(vout);
  WavetableSynth& synth = *reinterpret_cast<WavetableSynth*>(user);

  for (unsigned long i=0; i < frames; ++i) {
    out[i] = synth.output();
    synth.next();
  }

  return paContinue;
}


/////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
  const int RATE = 44100;

  if (argc == 1) {
    cout << MidiIn::getDeviceInfo() << endl;
    cout << "Re-run the program with the specified device to use as input" << endl;
    return 0;
  }

  if (argc != 2 && argc != 3) {
    return -1;
  }

  int devno = atoi(argv[1]);
  float rate = (argc == 3) ? float(atof(argv[2])) : 44100;
  WavetableSynth *synth = 0;
  try {
    synth = new WavetableSynth(devno,int(rate));
  }
  catch (exception &e) {
    cout << e.what() << endl;
    return -1;
  }
  catch (...) {
    cout << "failed to open device" << endl;
    return -1;
  }

  Pa_Initialize();
  PaStreamParameters params;
  params.device = Pa_GetDefaultOutputDevice();
  params.channelCount = 1;
  params.sampleFormat = paFloat32;
  params.suggestedLatency = max(0.02,Pa_GetDeviceInfo(params.device)
                                     ->defaultLowOutputLatency);
  params.hostApiSpecificStreamInfo = 0;
  PaStream *output_stream;
  Pa_OpenStream(&output_stream,0,&params,rate,0,0,onWrite,synth);
  Pa_StartStream(output_stream);

  cin.get();

  Pa_StopStream(output_stream);
  Pa_CloseStream(output_stream);
  Pa_Terminate();
  delete synth;

  return 0;
}

