/**
@file
	ADSR.cpp
@brief
	Implementation for DLS style ADSR envelope of volume change periods
@project
	SP24CS245-A Assignment 7 (3/15/24)
@author
	Ari Surprise (a.surprise@digipen.edu | 0050207)
*/

#define _USE_MATH_DEFINES
#include <cmath>
#include "ADSR.h"

// constexpr float BITDEPTH = 16;// |
//		    												  v

/// (96/20)*ln(10) factor SNR 0 volume decay factor point; k := EXP_DECAY / time
constexpr float EXP_DECAY = (float)(M_LN10 * 96/20);
// SNR := 20*bits*ln(2); k := (ln(10)*SNR)/(20*t) => ln(10)*bits*ln(2)/t

/**
@brief
	Get the linear decay factor for the given duration and sample rate
@param t
  - Duration in seconds for the given period to see elapsed
@param R
  - Samples per second used as incremental interval of time
@param d
  - Decay time (edge case context when d=0)
@param s
  - Sustain level (edge case context when d=0)
*/
float linearDegradeRate(float t, float R, float d, float s)
{
	if (t == 0 || R == 0) { return 1; }
	return 1/(t*R);
}

/**
@brief
	Get the exponential decay factor for the given duration and sample rate
@param t
  - Duration in seconds for the given period to see elapsed
@param R
  - Samples per second used as incremental interval of time
*/
float expDegradeRate(float t, float R)
{
	if (t == 0) { return 0; }
	if (R == 0) { return 1.0f; }
	float k = EXP_DECAY / t;
	return (float)pow(M_E, -k/R);
}

/**
@brief
	Envelope to adjust audio volume over time in 4 phases
@param a
  - Attack duration in seconds for the volume to ramp to full from 0
@param d
  - Decay duration in seconds for the volume to drop from full to 0
@param s
  - Sustain ratio out of 1.0 as 100% volume to maintain while holding notes
@param r
  - Release duration in seconds for the volume to drop from full to 0
@param R
  - Samples per second used as incremental interval of time
*/
ADSR::ADSR(float a, float d, float s, float r, float R)
	: current_mode(ATTACK), envelope(a == 0? 1.0f : 0),
	sustain_level(s), attack_increment(linearDegradeRate(a, R, d, s)),
	decay_factor(expDegradeRate(d, R)),
	release_factor(expDegradeRate(r, R))
{
}

/**
@brief
	Set the envelope into the release state
*/
void ADSR::sustainOff(void)
{
	current_mode = RELEASE;
}

/**
@brief
	Set the envelope back to the original state at 0.0 time
*/
void ADSR::reset(void)
{
	envelope = 0.0f;
	current_mode = ATTACK;
}

/**
@brief
	Get the envelope amplitude for the current time
@return
  - [0.0, 1.0] scale value from current evnelope time for audio amplitude
*/
float ADSR::output(void)
{
	return envelope;
}

/**
@brief
	Advance the envelope to the next time sample increment & envelope settings
*/
void ADSR::next(void)
{
	switch (current_mode)
	{
	case ATTACK:
		envelope += attack_increment;
		if (1.0 <= envelope)
		{
			envelope = 1.0;
			current_mode = DECAY;
		}
		break;
	case DECAY:
		envelope *= decay_factor;
		if (envelope <= sustain_level)
		{
			envelope = sustain_level;
		}
		break;
	case SUSTAIN:
		break;
	case RELEASE: default:
		envelope *= release_factor;
		break;
	}
}

/**
@brief
	Get the current time's envelope mode: { Attack, Decay, Sustain, Release }
@return
    Mode of envelope's progressive change per sample currently in use
*/
ADSR::Mode ADSR::mode(void)
{
	return current_mode;
}
