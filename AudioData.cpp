/**
\file
	AudioData.cpp
\brief
	Implementation for read/write of basic audio data manipulation
\project
	(SP24) CS245 Assignment 3
\date
	2/9/2024
\author
	Ari Surprise (a.surprise@digipen.edu)
*/

#include <iostream> // debug
#include <sstream>	// informed error message construction
#include<fstream>   // wav file open / close
#include "AudioData.h"

#define BITS_TO_BYTES >> 3
#define BYTES_TO_BITS << 3

// Header field tags' byte offsets into the file per relevant datum
static const unsigned TAG_LEN = 4;


/**
\brief
	wave file's header chunk of fields for (basic) data format settings
*/
struct FMTChunk
{
	FMTChunk(unsigned sampling_rate = 44100u, unsigned channel_count = 1u,
		unsigned bits_per_sample = 16u)
	{
		channels = channel_count;
		sample_rate = sampling_rate;
		sample_bits = bits_per_sample;
		byte_align = channels * sample_bits BITS_TO_BYTES;
		data_rate = sampling_rate * byte_align;
	}
	int8_t TAG[TAG_LEN] = { 'f', 'm', 't', ' ' };
	uint32_t size = 16u; // Basic header data fields
	uint16_t code = 1u; // Uncompressed audio
	uint16_t channels = 1u;
	uint32_t sample_rate = 44100u;
	uint32_t data_rate = 88200u;
	uint16_t byte_align = 2u;
	uint16_t sample_bits = 16u;
	//uint16_t extension_size = 0;
	//uint16_t valid_sample_bits = 16u;
	//uint32_t channel_mask = 0;
	//uint128_t data_coded_GUID = 0;
};


/**
\brief
	wave file's header root chunk of fields for riff format adherence & data
*/
struct WavHeader {
	WavHeader(unsigned frames = 0, unsigned sample_rate = 44100u,
		unsigned channels = 1u, uint16_t bits = 16u)
		: fmt(sample_rate, channels, bits)
	{
		data_size = frames * fmt.byte_align;
		riff_size = 36u + data_size;
	}
	int8_t RIFF_TAG[TAG_LEN] = { 'R', 'I', 'F', 'F' };
	uint32_t riff_size = 44136u;
	int8_t WAVE_TAG[TAG_LEN] = { 'W', 'A', 'V', 'E' };
	FMTChunk fmt;
	int8_t DATA_TAG[TAG_LEN] = { 'd', 'a', 't', 'a' };
	uint32_t data_size = 44100u; // 1 second of defaulted rate of samples
};


/**
\brief
	Obtain the maxima between 2 float point values
@param a
	- first number for comparison
@param b
	- second number for comparison
\return
	the greater value of the 2 between a and b (a if b <= a)
*/
inline float MaxF(float a, float b)
{
	return a > b ? a : b;
};

/**
\brief
	Obtain the absolute value of an input float point value
@param val
	- value to have its absolute value returned
\return
	the unsigned magnitude of the input value
*/
inline float AbsF(float val)
{
	return MaxF(val, -val);
};

/**
\brief
	Get whether the first 4 characters of the buffer equal those in the tag
@param buffer
	- value read in for scanning (minimum 4 char array)
@param valid_tag
	- value used to compare against buffer's characters for equality
	\return
	true iff [buffer[0] == valid_tag[0], ..., buffer[3] == valid_tag[3]]
*/
bool IsValidTag(const int8_t* buffer, const int8_t* valid_tag)
{
	for (int i = 0; i < TAG_LEN; i++)
	{
		if (buffer[i] != valid_tag[i]) { return false; }
	}
	return true;
};

/**
\brief
Create a default AudioData (0 data values), of the appropriate length for given inputs
\param nframes
 - number of samples to be contained, per nchannels, for silence lasting nframes/R seconds
\param R
 - optional sampling rate dictating how many snapshots are stored per second of audio data; how fast stored data should be interpreted to be read
\param nchannels
 - optional number of channels to create data for (1 => mono, 2 => L/R interleaved stereo, 5 => 4.1 surround, etc)
*/
AudioData::AudioData(unsigned nframes, unsigned R, unsigned nchannels)
	: frame_count(nframes), sampling_rate(R), channel_count(nchannels)
{
	fdata.resize(frame_count * channel_count);
}

/**
\brief
 Create AudioData to hold wave data read in from a file
\param fname
 - path string (absolute or relative to running directory), to the wave file to be read
*/
AudioData::AudioData(const char* fname)
	: frame_count(1), sampling_rate(44100), channel_count(2)
{
	FILE* wf;
	int8_t buffer1;
	int8_t buffer2[2];
	int8_t buffer4[4];
	size_t bytesRead;
	WavHeader wav;
	uint32_t file_size;
	uint32_t fmt_pos;
	uint32_t data_pos;
	std::stringstream message;

	// Confirm file read access
	fopen_s(&wf, fname, "rb");
	if (!wf)
	{
		message << "file '" << fname << "' not found";
		throw std::runtime_error(message.str());
	}
	fseek(wf, 0, SEEK_END);
	file_size = ftell(wf);

	// Validate RIFF format file
	fseek(wf, offsetof(WavHeader, RIFF_TAG), SEEK_SET);
	fread(buffer4, TAG_LEN, 1, wf);
	if (!IsValidTag(&buffer4[0], &wav.RIFF_TAG[0]))
	{
		message << "Invalid WAVE data: incorrect RIFF tag";
		throw std::runtime_error(message.str());
	}
	fread(buffer4, sizeof(uint32_t), 1, wf);
	wav.riff_size = *(uint32_t*)buffer4;
	fread(buffer4, TAG_LEN, 1, wf);
	if (!IsValidTag(&buffer4[0], &wav.WAVE_TAG[0]))
	{
		message << "Invalid WAVE data: incorrect WAVE tag";
		throw std::runtime_error(message.str());
	}

	// Find/read format chunk
	fmt_pos = offsetof(WavHeader, WAVE_TAG);
	do
	{
		bytesRead = fread(buffer4, TAG_LEN, 1, wf);
		if (bytesRead <= 0)
		{
			message << "Invalid/corrupt WAVE: missing format chunk";
			throw std::runtime_error(message.str());
		}
		fmt_pos += TAG_LEN;
		if (IsValidTag(&buffer4[0], &wav.fmt.TAG[0])) { break; }
	} while (true);
	fread(buffer4, sizeof(uint32_t), 1, wf);
	wav.fmt.size = *(uint32_t*)buffer4;
	if (wav.fmt.size < 16 && wav.fmt.size != 8)
	{
		// Second clause to continue with what I believe to be a field misuse
		// ie wav.fmt.size duplicate to wav.fmt.bits_per_sample, not chunk allocation
		message << "Invalid/corrupt WAVE: format chunk size " << wav.fmt.size
			<< " not recognized";
		throw std::runtime_error(message.str());
	}
	fread(buffer2, sizeof(uint16_t), 1, wf);
	wav.fmt.code = *(uint16_t*)buffer2;
	if ((wav.fmt.code) != 1)
	{
		message << "Invalid/corrupt WAVE: compressed formats unsupported";
		throw std::runtime_error(message.str());
	}
	fread(buffer2, sizeof(uint16_t), 1, wf);
	channel_count = wav.fmt.channels = *(uint16_t*)buffer2;
	fread(buffer4, sizeof(uint32_t), 1, wf);
	sampling_rate = wav.fmt.sample_rate = *(uint32_t*)buffer4;
	fread(buffer4, sizeof(uint32_t), 1, wf);
	wav.fmt.data_rate = *(uint32_t*)buffer4;
	fread(buffer2, sizeof(uint16_t), 1, wf);
	wav.fmt.byte_align = *(uint16_t*)buffer2;
	fread(buffer2, sizeof(uint16_t), 1, wf);
	wav.fmt.sample_bits = *(uint16_t*)buffer2;
	if (!(wav.fmt.sample_bits == 16 || wav.fmt.sample_bits == 8))
	{
	}
	// (extended wave format data in 18 or 40 byte sizes not supported/used)

	// Find/read data chunk
	data_pos = offsetof(WavHeader, fmt.TAG) + wav.fmt.size;
	fseek(wf, data_pos, SEEK_SET);
	do
	{
		bytesRead = fread(buffer4, TAG_LEN, 1, wf);
		if (bytesRead <= 0)
		{
			message << "Invalid/corrupt WAVE: missing data chunk";
			throw std::runtime_error(message.str());
		}
		data_pos += TAG_LEN;
		if (IsValidTag(&buffer4[0], &wav.DATA_TAG[0])) { break; }
	} while (true);
	fread(buffer4, sizeof(uint32_t), 1, wf);
	wav.data_size = *(uint32_t*)buffer4;
	frame_count = wav.data_size / ((wav.fmt.sample_bits BITS_TO_BYTES) * channel_count);
	fdata.resize(frame_count * channel_count);
	//fseek(wf, data_pos, SEEK_SET);
	uint32_t samples = frame_count * channel_count;
	switch (wav.fmt.sample_bits)
	{
	case 8:
		switch (channel_count)
		{
		case 1:
			for (unsigned i = 0; i < frame_count; ++i)
			{
				fread(&buffer1, 1, 1, wf);
				fdata[i] = (((uint8_t)buffer1) - 128.0f) * 0.00787401574803149606299212598425f;
			}
			break;
		case 2:
			for (unsigned i = 0; i < frame_count; ++i)
			{
				fread(&buffer2[0], 2, 1, wf);
				fdata[2 * i] = (((uint8_t*)buffer2)[0] - 128.0f) * 0.00787401574803149606299212598425f;
				fdata[2 * i + 1] = (((uint8_t*)buffer2)[1] - 128.0f) * 0.00787401574803149606299212598425f;
			}
			break;
		default:
			message << "Invalid/corrupt WAVE: only mono or stereo channels supported";
			throw std::runtime_error(message.str());
		}
		break;
	case 16:
		switch (channel_count)
		{
		case 1:
			for (unsigned i = 0; i < frame_count; ++i)
			{
				fread(&buffer2[0], 2, 1, wf);
				fdata[i] = (*(int16_t*)buffer2) * 0.000030518509475997192297128208258309f;
			}
			break;
		case 2:
			for (unsigned i = 0; i < frame_count; ++i)
			{
				fread(&buffer4[0], 4, 1, wf);
				fdata[2 * i] = ((int16_t*)buffer4)[0] * 0.000030518509475997192297128208258309f;
				fdata[2 * i + 1] = ((int16_t*)buffer4)[1] * 0.000030518509475997192297128208258309f;
			}
			break;
		default:
			message << "Invalid/corrupt WAVE: only mono or stereo channels supported";
			throw std::runtime_error(message.str());
		}
		break;
	default:
		message << "Invalid/corrupt WAVE: only 8 or 16-bit data supported";
		throw std::runtime_error(message.str());
	}

	fclose(wf);

}

/**
\brief
 Look up the proper sample in the interleaved channel data for the given frame and channel number
\param frame
 - [0,frames()-1] index for the desired sample number's offset into the audio data
\param channel
 - optional [0,channels()-1] index to get the given channel sample from the given frame (default 0 => channel[0], ie primary)
*/
float AudioData::sample(unsigned frame, unsigned channel) const
{
	return fdata[frame * channels() + channel];
}

/**
\brief
 Look up the proper sample in the interleaved channel data for the given frame and channel number
\param frame
 - [0,frames()-1] index for the desired sample number's offset into the audio data
\param channel
 - optional [0,channels()-1] index to get the given channel sample from the given frame (default 0 => channel[0], ie primary)
*/
float& AudioData::sample(unsigned frame, unsigned channel)
{
	return fdata[frame * channels() + channel];
}

/**
\brief
 Set the data in AudioData ad to a given normalized max dB range (DC offset obligatory)
\param ad
 - AudioData to have it's data modified to become normalized to the new dB volume
\param dB
 - optional decibels to re-calibrate maximum dB in resultant ad data set (0 default => [-1,1 range])
*/
void normalize(AudioData& ad, float dB)
{
	// Shortcut when data set is null in length (already normalized; prevent 0 division)
	if (ad.frames() == 0 || ad.channels() == 0) { return; }

	float max = 0.0f; // absolute maximumum sample value in all channels
	size_t channel = 0; // which channel subscript the maxima was found in
	std::vector<float> DC(ad.channels()); // DC Offset per channel[#]
	// Calculate DC offsets per channel
	float sum; // running sample sum of the current channel
	float rate; // current sample's temp store of absolute value, then gain ratio after max is found
	unsigned total_samples = ad.frames() * ad.channels();
	for (unsigned i = 0; i < ad.channels(); ++i)
	{
		// Iterate through channel's interleaved samples
		sum = 0.0;
		for (unsigned s = i; s < total_samples; s += ad.channels())
		{
			sum += ad.data()[s]; // Sum channel samples per frame
			// Concurrently check each sample for global absolute maxima
			rate = AbsF(ad.data()[s]);
			if (max < rate)
			{
				channel = i;
				max = rate;
			}
		}
		// Record that channel's DC offset (arithmetic mean)
		DC[i] = sum / ad.frames();
	}
	max -= DC[channel]; // Offset maxima for its representative channel

	// Recalibrate each channel's samples to individually sum to 0
	for (unsigned i = 0; i < ad.channels(); ++i)
	{
		// Iterate through channel's interleaved samples
		for (unsigned s = i; s < total_samples; s += ad.channels())
		{
			ad.data()[s] = ad.data()[s] - DC[i]; // remove offset per sample per channel
		}
	}

	// Calculate non-clipping gain factor to use from max & desired dB
	rate = dB / 20;
	rate = (float)pow(10, rate);
	rate = rate / max;

	// Multiply each sample by the given input's gain factor ratio, scaled for existing data
	for (unsigned s = 0; s < total_samples; ++s)
	{
		ad.data()[s] = ad.data()[s] * rate;
	}
}

/**
\brief
 Export ad's data to be written in the given bits rate .wav file format at fname path
\param fname
 - path string with file name at which output .wav file is to be written
\param ad
 - data container to be exported to a .wav file
\param bits
 - data written in bit width 8 ([0,255]), 16 ([-32768,32767]) or 32 ([-1.0, 1.0])
\return
 True when wave data is written successfully, false if input settings are invalid
*/
bool waveWrite(const char* fname, const AudioData& ad, unsigned bits)
{
	FILE* wf;
	// Attempt to open the file
	fopen_s(&wf, fname, "wb");
	if (!wf)
	{
		// (signify write privelege access error)
		return false;
	}
	// Write the header fields to file head
	WavHeader wav(ad.frames(), ad.rate(), ad.channels(), bits);
	fwrite(&wav, sizeof(wav), 1, wf); // data_size

	// Write the samples / data to file body
	int16_t buff2;
	uint8_t buff1;
	switch (wav.fmt.sample_bits)
	{
	case 8:
		switch (ad.channels())
		{
		case 1:
			for (unsigned i = 0; i < ad.frames(); ++i)
			{
				buff1 = (uint8_t)(ad.data()[i] * 127 + 128);
				fwrite(&buff1, 1, 1, wf);
			}
			break;
		case 2:
			for (unsigned i = 0; i < ad.frames(); ++i)
			{
				buff1 = (uint8_t)(ad.data()[2 * i] * 127 + 128);
				fwrite(&buff1, 1, 1, wf);
				buff1 = (uint8_t)(ad.data()[2 * i + 1] * 127 + 128);
				fwrite(&buff1, 1, 1, wf);
			}
			break;
		default:
			return false; // only mono & stereo data supported
		}
		break;
	case 16:
		switch (ad.channels())
		{
		case 1:
			for (unsigned i = 0; i < ad.frames(); ++i)
			{
				buff2 = (int16_t)(ad.data()[i] * 32767);
				fwrite(&buff2, 2, 1, wf);
			}
			break;
		case 2:
			for (unsigned i = 0; i < ad.frames(); ++i)
			{
				buff2 = (int16_t)(ad.data()[2 * i] * 32767);
				fwrite(&buff2, 2, 1, wf);
				buff2 = (int16_t)(ad.data()[2 * i + 1] * 32767);
				fwrite(&buff2, 2, 1, wf);
			}
			break;
		default:
			return false; // only mono & stereo data supported
		}
		break;
	default:
		return false; // only 8 or 16 bit data supported
	}

	fclose(wf); // Close the written file
	return true;
}
