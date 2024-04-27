/**
@file
  WabetableSynth.cpp
@brief
  Wavetable synthesizer mapped from midi input (portmidi) to audio (portaudio)
@project
  SP24CS245-A Assignment 9 (4/5/24)
@author
  Ari Surprise (a.surprise@digipen.edu | 0050207)
*/

#define _USE_MATH_DEFINES
#include <cmath> // Math constant definitions, trig functions, etc
#include "WavetableSynth.h" // Class header file

/// Epsilon infinitesimal for narrow float ranges
constexpr float EPSILON = 0.01f;

/// Cents up from middle C note to the A above it, ie A sounding 440 Hz
constexpr float A440_CENTS = 6900.0f;

/// Cents up from middle C note to the A above it, ie A sounding 440 Hz
constexpr float MIX_DOWN = 0.3f;

/// Scalar of 100 cents per semitone
constexpr int CENTS_SCALE = 100;

/// [-200, 200] Cents scale used (currently) both pitch bend & modulation
constexpr float CENTS_RANGE = 2.0f * CENTS_SCALE;
 
/// Precomputed (1/127) for more efficient division of a common divisor
constexpr float RATIO_7BIT = 1.0f / 127.0f;

/// 2*pi; tau; circle diameter constant; revolutions -> radians
constexpr float TWO_PI = (float)(2.0 * M_PI);

/// Mod wheel sin period scale: 5Hz*2pi, sounding a 5 Hz modulation cycle
constexpr float REV_TO_HZ = 5.0f * TWO_PI;

/// Baby Upright Acoustic Grand Piano's A0 keypress recording
WavetableSynth::WaveData grand0("UpGrand_A22_5.wav", 16.0f, 46310, 66775);

/// Baby Upright Acoustic Grand Piano's A1 keypress recording
WavetableSynth::WaveData grand1("UpGrand_A55.wav", 8.0f, 129883, 197134);

/// Baby Upright Acoustic Grand Piano's A2 keypress recording
WavetableSynth::WaveData grand2("UpGrand_A110.wav", 4.0f, 71353, 117383);

/// Baby Upright Acoustic Grand Piano's A3 keypress recording
WavetableSynth::WaveData grand3("UpGrand_A220.wav", 2.0f, 109664, 169738);

/// Baby Upright Acoustic Grand Piano's A4 keypress recording
WavetableSynth::WaveData grand4("UpGrand_A440.wav", 1.0f, 56129, 100326);

/// Baby Upright Acoustic Grand Piano's A5 keypress recording
WavetableSynth::WaveData grand5("UpGrand_A880.wav", 0.5f, 11437, 40303);

/// Baby Upright Acoustic Grand Piano's A6 keypress recording
WavetableSynth::WaveData grand6("UpGrand_A1760.wav", 0.25f, 6344, 13215);

/// Baby Upright Acoustic Grand Piano's A7 keypress recording
WavetableSynth::WaveData grand7("UpGrand_A3520.wav", 0.125f, 14565, 28123);

/// Cello sample from CS245 class materials
WavetableSynth::WaveData cello("Cello.wav",
  4.51280512805128051280512805128051281f, 39763, 42019);

/// Oboe sample from CS245 class materials
WavetableSynth::WaveData oboe("Oboe.wav",
  0.990990990990990990990990990990990990991f, 322, 17455);

WavetableSynth::WavetableSynth(int devno, int R)
  : MidiIn(devno), newest(0), patch(Default), bend(0), vibrato(0), vol(0.5f),
  mod(0), mphase(0), rate((float)R)
{
  int i;
  dphase = REV_TO_HZ / R;
  for (i = 0; i < MAX_NOTES; ++i)
  {
    playing[i] = Note(-1, 0, Voice::Default, (float)R);
  }
  start();
}

WavetableSynth::~WavetableSynth(void)
{
  stop();
}

float WavetableSynth::output(void)
{
  float output = 0.0f;
  int i;
  for (i = 0; i < MAX_NOTES; ++i)
  {
    output += (playing[i].key < 0) ? 0.0f : playing[i].output();
  }
  return output * vol;
}

void WavetableSynth::next(void)
{
  int i;
  double t;
  if (0 != vibrato)
  {
    t = mphase + dphase;
    mphase = (t <= TWO_PI) ? t : (t - TWO_PI);
    mod = vibrato * (float)sin(mphase);
    for (i = 0; i < MAX_NOTES; ++i)
    {
      playing[i].phase.pitchOffset(mod + bend + playing[i].key - A440_CENTS);
    }
  }
  for (i = 0; i < MAX_NOTES; ++i)
  {
    playing[i].next();
  }
}

void WavetableSynth::onModulationWheelChange(int channel, int value)
{
  vibrato = value * CENTS_RANGE * RATIO_7BIT;
}

void WavetableSynth::onNoteOff(int channel, int note)
{
  int i;
  note *= CENTS_SCALE;
  for (i = 0; i < MAX_NOTES; ++i)
  {
    if (playing[i].key == note)
    {
      playing[i].env.sustainOff();
    }
  }
}

void WavetableSynth::onNoteOn(int channel, int note, int velocity)
{
  int i;
  int index = -1;
  note *= CENTS_SCALE;
  // check exhaustively for note playing to early out
  for (i = newest; i < MAX_NOTES; ++i)
  {
    if (playing[i].key == note)
    {
      playing[i].vel = velocity * RATIO_7BIT;
      playing[i].env.reset();
      playing[i].phase.reset();
      return;
    }
    // look for first free note slot concurrently
    if (0 <= index && playing[i].key < 0) { index = i; }
  }
  for (i = 0; i < newest; ++i)
  {
    if (playing[i].key == note)
    {
      playing[i].vel = velocity * RATIO_7BIT;
      playing[i].env.reset();
      playing[i].phase.reset();
      return;
    }
    if (0 <= index && playing[i].key < 0) { index = i; }
  }
  // Look for note to steal if none were open
  if (index < 0)
  {
    index = (newest + 1 == MAX_NOTES) ? 0 : newest + 1;
  }
  playing[index].key = note;
  playing[index].vel = velocity * RATIO_7BIT;
  playing[i].inst = patch;
  playing[index].Play(rate);
  playing[index].env.reset();
  newest = index;
}

void WavetableSynth::onPatchChange(int channel, int value)
{
  int i;
  patch = (Voice)(value % Voice::Max);
  for (i = 0; i < MAX_NOTES; ++i)
  {
    playing[i] = Note(-1, 0, patch, rate);
  }
}

void WavetableSynth::onPitchWheelChange(int channel, float value)
{
  bend = value * CENTS_RANGE; // Reused with vibrato on
  int i;
  for (i = 0; i < MAX_NOTES; ++i)
  {
    playing[i].phase.pitchOffset(bend + playing[i].key - A440_CENTS);
  }
}

void WavetableSynth::onVolumeChange(int channel, int level)
{
  vol = level * RATIO_7BIT;
}

WavetableSynth::WaveData::WaveData(const char* file, float gain,
  size_t start, size_t end)
  : source(file), speed(gain), first(start), last(end), channel(0)
{
}

WavetableSynth::Note::Note(short midid, float velocity, Voice instr, float rate)
  : env(0.01f, 600.0f, 0.8f, 4.0f, rate), key(midid), vel(velocity), inst(instr)
{
  Play(rate);
}

void WavetableSynth::Note::next(void)
{
  if (key == -1) { return; }
  if (env.mode() == ADSR::RELEASE && env.output() < EPSILON)
  {
    key = -1;
    vel = 0;
    phase.pitchOffset(-25600.0f);
    return;
  }
  phase.next();
  env.next();
}

float WavetableSynth::Note::output(void)
{
  return phase.output() * env.output() * MIX_DOWN;
}

void WavetableSynth::Note::Play(float rate)
{
  switch (inst)
  {
  case Grand:
    if (key < 1600) { SetSound(grand0, rate); return; }
    if (key < 3200) { SetSound(grand1, rate); return; }
    if (key < 4800) { SetSound(grand2, rate); return; }
    if (key < 6400) { SetSound(grand3, rate); return; }
    if (key < 8000) { SetSound(grand4, rate); return; }
    if (key < 9600) { SetSound(grand5, rate); return; }
    if (key < 11200) { SetSound(grand6, rate); return; }
    else { SetSound(grand7, rate); return; }
  case Cello:
    SetSound(cello, rate); return;
  case Oboe:
    SetSound(oboe, rate); return;
  default: return;
  }
}

void WavetableSynth::Note::SetSound(WaveData& data, float sampling_rate)
{
  float rate_offset = sampling_rate / (float)data.source.rate();
  phase = Resample(&data.source, data.channel, (rate_offset == 0) ? data.speed :
    data.speed * rate_offset, data.first, data.last);
  phase.pitchOffset(key - A440_CENTS);
}
