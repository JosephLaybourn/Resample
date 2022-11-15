// Resample.h
// cs245 5/18

#ifndef CS245_RESAMPLE
#define CS245_RESAMPLE


#include "AudioData.h"


class Resample {
  public:
    explicit Resample(const AudioData *ad_ptr=0, unsigned channel=0,
                      float factor=1, unsigned loop_bgn=0, unsigned loop_end=0);
    void setTime(float t);
    float getValue(void);
    void incrementTime(void);
    void offsetPitch(float cents);
  private:
    const AudioData *audio_data;
    unsigned ichannel;
    double findex, findex_increment;
    float findex_multiplier;
    unsigned iloop_bgn, iloop_end;
};


#endif
