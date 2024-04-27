// Resample.h
// cs245 2024.02

#ifndef CS245_RESAMPLE_H
#define CS245_RESAMPLE_H


#include "AudioData.h"


class Resample {
  public:
    explicit Resample(const AudioData *ad_ptr=0, unsigned channel=0,
                      float factor=1, unsigned loop_bgn=0, unsigned loop_end=0);
    float output(void);
    void next(void);
    void pitchOffset(float cents);
    void reset(void);
  private:
    const AudioData *audio_data;
    unsigned ichannel;
    double findex;
    float speedup,
          multiplier;
    unsigned iloop_bgn, iloop_end;
};


#endif

