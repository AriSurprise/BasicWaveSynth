// AudioData.h
// -- basic audio data manipulation
// cs245 1/18

#ifndef CS245_AUDIODATA_H
#define CS245_AUDIODATA_H


#include <vector>


class AudioData {
public:
    // These functions implemented in assignment #2:
    AudioData(unsigned nframes, unsigned R = 44100, unsigned nchannels = 1);
    float sample(unsigned frame, unsigned channel = 0) const;
    float& sample(unsigned frame, unsigned channel = 0);

    float* data(void) { return &fdata[0]; }
    const float* data(void) const { return &fdata[0]; }
    unsigned frames(void) const { return frame_count; }
    unsigned rate(void) const { return sampling_rate; }
    unsigned channels(void) const { return channel_count; }

    // This function implemented in assignment #3:
    AudioData(const char* fname);

private:
    std::vector<float> fdata;
    unsigned frame_count,
        sampling_rate,
        channel_count;
};


// Implemented in assignment #2:
void normalize(AudioData& ad, float dB = 0);


// Implemented in assignment #3:
bool waveWrite(const char* fname, const AudioData& ad, unsigned bits = 16);


#endif

