// ResampleTest.cpp
// -- simple test of Resample class
// cs245 5/18

#include <fstream>
#include <cmath>
#include "Resample.h"
using namespace std;


int main(void) {
  const float TWO_PI = 8.0f * atan(1.0f);

  unsigned rate = 44100;
  float dtime = 1.0f/rate;

  // input data
  float time1 = 0.125f, time2 = 0.250f, time3 = 0.450f,
        freq0 = 200.0f, freq1 = 300.0f, freq2 = 450.0f,
                        freq3 = 400.0f, freq4 = 500.0f;
  unsigned in_count = unsigned(time3*rate);
  AudioData data_in(in_count,rate,2);
  for (unsigned i=0; i < in_count; ++i) {
    data_in.sample(i,0) = 0.8f * sin(TWO_PI*freq0*i*dtime);
    float t = fmod(freq0*i*dtime,1.0f);
    data_in.sample(i,1) = 0.9f * (t < 0.5f ? 4*t-1 : 3-4*t);
  }

  // output data: 4 parts (one second each)
  unsigned out_count = 4*rate;
  AudioData data_out(out_count,rate,1);

  // (1) resampled, no loop, no pitch offset
  Resample resample(&data_in,0,freq1/freq0,0,0);
  for (unsigned i=0; i < rate; ++i) {
    resample.setTime(i*dtime);
    data_out.sample(i) = resample.getValue();
  }

  // (2) resampled, loop, no pitch offset
  unsigned loop_bgn = unsigned(time1*rate),
           loop_end = unsigned(time2*rate);
  resample = Resample(&data_in,1,freq2/freq0,loop_bgn,loop_end);
  for (unsigned i=0; i < rate; ++i) {
    data_out.sample(rate+i) = resample.getValue();
    resample.incrementTime();
  }

  // (3) resampled, no loop, pitch offset
  resample = Resample(&data_in,0,freq3/freq0,0,0);
  for (unsigned i=0; i < rate; ++i) {
    float pitch = 700*i*dtime;
    resample.offsetPitch(pitch);
    data_out.sample(2*rate+i) = resample.getValue();
    resample.incrementTime();
  }

  // (4) resampled, loop, pitch offset
  resample = Resample(&data_in,1,freq4/freq0,loop_bgn,loop_end);
  for (unsigned i=0; i < rate; ++i) {
    float pitch = 300*sin(TWO_PI*2*i*dtime);
    resample.offsetPitch(pitch);
    data_out.sample(3*rate+i) = resample.getValue();
    resample.incrementTime();
  }

  // write output data
  waveWrite("ResampleTest.wav",data_out);

  return 0;
}

