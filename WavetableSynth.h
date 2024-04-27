/**
@file
  WavetableSynth.h
@brief
  Wavetable synthesizer mapped from midi input (portmidi) to audio (portaudio)
@project
  SP24CS245-A Assignment 9 (4/5/24)
@author
  Ari Surprise (a.surprise@digipen.edu | 0050207)
*/

#ifndef CS245_WAVETABLESYNTH_H
#define CS245_WAVETABLESYNTH_H

#include "MidiIn.h" // Base class for inheritance
#include "AudioData.h" // Member for wavetable of buffered audio file data
#include "Resample.h" // Member for pitch moderation of AudioData member
#include "ADSR.h" // Member for gradual volume changes in sampled note playback

/// Use MidiIn functionality to translate polled midi device to audio output
class WavetableSynth : private MidiIn {
  public:

    /**
    @brief
      Initialize audio device for midi polling and sound output
    @param devno
      - System enumeration of available midi input devices to read from
    @param R
      - Samples per second used as synthesized wave read speed baseline
    */
    WavetableSynth(int devno, int R);

    /**
    @brief
      Stop input polling in preparation to shut down audio output
    */
    ~WavetableSynth(void);

    /**
    @brief
      Output waveform sample from current patch at current time setting
    @return
      - [-1,1] value from active waveform phase offset for audio ouptut
    */
    float output(void);

    /**
    @brief
      Advance by the current pitch increment to the next output sample
    */
    void next(void);

    /// Container for attributes for a patch to initialize a Note's Resampler
    struct WaveData
    {
      /**
      @brief
          Construct container of Resampler initialization attributes
      @param file
        - Pointer to the file name/path where AudioData wav file is located on disk
      @param gain
        - Gain factor to have the AudioData sound 440 Hz relative to the file contents
      @param start
        - First sample used in looped portion of audio data
      @param end
        - Last sample used in looped portion of audio data
      */
      WaveData(const char* file, float gain, size_t start = 0, size_t end = 0);

      /// Loaded audio file to be resampled in playing a note for the active patch
      AudioData source;

      /// Which channel a note is effecting (always 0 on this implementation)
      size_t channel;

      /// Start sample number of audio data for looped portion
      size_t first;

      /// End sample number of audio data for looped portion
      size_t last;

      /// Gain factor to have the AudioData sourced (generally), sound at 440 Hz
      float speed;
    };
  private:
    static const int MAX_NOTES = 10;

    /// Sampled waveform data to use for the current patch / active voice
    enum Voice
    {
      Grand, /// Upright baby grand piano samples
      Oboe, /// Oboe sample from CS245 class materials
      Cello, /// Cello sample from CS245 class materials
      Max, /// Total sampled instruments available
      Default = Grand, /// Instrument to assign to synth & notes on startup
    };

    /// Container for attributes of a note being played
    struct Note
    {
      /**
      @brief
          Construct container of attributes tracking a note being played
      @param midid
        - [0,127] midi signal identifying linear semitone key progression
      @param velocity
        - Ratio of how hard from the possible [0,1] range a note is hit
      @param instrument
        - Which patch was active on the instrument when hte note was played
      @param rate
        - Sampling rate of the synthesizer
      */
      Note(short midid = -1, float velocity = 0, Voice instrument = Default,
        float rate = 44100.0f);

      /**
      @brief
        Output waveform sample from current patch at current time setting
      @return
        - [-1,1] value from active waveform phase offset for audio ouptut
      */
      float output(void);

      /**
      @brief
        Advance by the current pitch increment to the next output sample
      */
      void next(void);

      /**
      @brief
        Set the sound to source for the current instrument (and key where split)
      @param rate
        - Sampling rate of the synthesizer
      */
      void Play(float rate);

      /**
      @brief
        Set the phase resampler to source the given waveform data of sound to use
      @param data
        - Audio data and resampling context for the patch for this note to play
      @param sampling_rate
        - Sampling rate of the synthesizer
      */
      void SetSound(WaveData& data, float sampling_rate);

      /// index from midiinput from noteOn call * 100 as a note's ID in cents from A440
      short key;

      /// Volume for note's midi input velocity value when struck
      float vel;

      /// Which voice the note is set to be playing (for note retrigger to carry over)
      Voice inst;

      /// Point into waveform data to use as AudioData output
      Resample phase;

      /// Amplitude envelope with a balance of control and realism (automated)
      ADSR env;
    };

    /**
    @brief
      Callback to modulate voice amplitude of vibrato (pitch warble)
    @param channel
      - Index for the channel into which the given note activates (n/a)
    @param value
      - Ratio from [0,127] out of 127ths of 200 cent range of vibrato
    */
    void onModulationWheelChange(int channel, int value) override;

    /**
    @brief
      Callback to turn note off for the pitch being played
    @param channel
      - Index for the channel into which the given note deactivates (n/a)
    @param note
      - Index [0,127] for key enumeration of note to be turned off
    */
    void onNoteOff(int channel, int note) override;

    /**
    @brief
      Callback to turn note on for the given pitch / velocity
    @param channel
      - Index for the channel into which the given note activates (n/a)
    @param note
      - Index [0,127] for key enumeration of note to be turned on
    @param velocity
      - Value [0,127] indicative of how hard/loud a note is hit/played
    */
    void onNoteOn(int channel, int note, int velocity) override;

    /**
    @brief
      Callback to change voice for the instrument being played
    @param channel
      - Index for the channel into which the given voice changes (n/a)
    @param value
      - [0,14] value for which Voice enumeration should be selected
    */
    void onPatchChange(int channel, int value) override;

    /**
    @brief
      Callback to shift current pitch by up to 200 cents up or down
    @param channel
      - Index for the channel into which the pitch wheel modulates notes (n/a)
    @param value
      - [-1,1] range to be mapped into notes' [-200,200] cent pitch shift
    */
    void onPitchWheelChange(int channel, float value) override;

    /**
    @brief
      Callback to adjust global volume levels
    @param channel
      - Index for the channel into which the volume is adjusted (n/a)
    @param level
      - [0,127] range value to be mapped onto [0,1] volume ratio
    */
    void onVolumeChange(int channel, int level) override;

    /// Note attribute settings per key played
    Note playing[MAX_NOTES];

    /// Scalar of cents per octave for the current pitch bend setting
    float bend;

    /// Sampling rate: samples per second for the synthesizer to play
    float rate;

    /// Scalar of cents per octave for the current vibrato setting
    float vibrato;

    /// Scalad cents of the current vibrato setting's phase
    float mod;

    /// Global volume level to be managed by volume change calls
    float vol;

    /// Fractional index for modulation's sinusoidal phase position
    double mphase;

    /// Fractional phase increment per sample for modulation
    float dphase;

    /// Most recent note index to have had a note turned on
    int newest;

    /// Voice type/patch currently being played
    Voice patch;

};

#endif
