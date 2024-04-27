/**
\file
	Resample.cpp
\brief
	Implementation for audio data's sampling speed/pitch modulation
\project
	(SP24) CS245 Assignment 4
\date
	2/16/2024
\author
	Ari Surprise (a.surprise@digipen.edu)
*/

#include "Resample.h"

/// Reciprocal for 100 cents per semitone * 12 semitones per octave
constexpr double OCTAVE_CENTILES = 1.0 / 1200.0;

/**
@brief
	Driver of a given AudioData to set fractional sampling increment
@param ad_ptr
	- Address of AudioData to be Resampled at new rates
@param channel
	- Channel within AudioData to be Resampled (make instances per channel)
@param factor
	- Sampling increment gain factor relative to 1.0 for normal playback
@param loop_bgn
	- Frame subscript within AudioData at which looping should begin
@param loop_end
	- Frame subscript within AudioData at which looping should end
*/
Resample::Resample(const AudioData* ad_ptr, unsigned channel,
	float factor, unsigned loop_bgn, unsigned loop_end)
	: audio_data(ad_ptr), ichannel(channel), findex(0.8),
	speedup(factor), multiplier(factor), iloop_bgn(loop_bgn),
	iloop_end(loop_end)
{}


/**
@brief
	Get current interpolated output value for the driven AudioData
@return
	Resampled lerped output value of AudioData at the current time
*/
float Resample::output(void)
{
	size_t i = (int)findex, e = i + 1;
	double index = findex;
	unsigned channels = audio_data->channels();
	if (iloop_bgn < iloop_end && iloop_end < findex)
	{
		unsigned interval = iloop_end - iloop_bgn;
		double iters = ((findex - iloop_bgn) / interval);
		interval = (unsigned)iters * interval;
		index = findex - interval;
		i = (int)index;
		e = audio_data->frames() == i ? iloop_bgn : i + 1;
	}
	if (e < audio_data->frames())
	{
		double t1 = index - i, t0 = 1.0 - t1;
		i = i * channels + ichannel;
		e = e * channels + ichannel;
		float init = audio_data->data()[i];
		float end = audio_data->data()[e];
		float result = (float)((t0 * init) + end * t1);
		return result;
	}
	if (audio_data->frames() == i && findex - i < 0.001)
	{
		return audio_data->data()[i * channels + ichannel];
	}
	return 0.0f;
}


/**
@brief
	Advance to the next output sample index by the AudioData's sampling rate
*/
void Resample::next(void)
{
	findex += speedup;
}


/**
@brief
	Modify the baseline speedup factor by which AudioData is being Resampled
@param cents
	- -1.0 => -0.01 semitone by which AudioData pitch shifts down from base
*/
void Resample::pitchOffset(float cents)
{
	float pitch = (float)pow(2.0, cents * OCTAVE_CENTILES);
	speedup = multiplier * pitch;
}


/**
@brief
	Set current time to 0 (looping, speed and pitch-offset unaffected)
*/
void Resample::reset(void)
{
	findex = 0.0;
}
