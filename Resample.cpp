#include "Resample.h"
#include <iostream>
#include <cmath>

static bool isLooping;

Resample::Resample(const AudioData * ad_ptr, unsigned channel, float factor, unsigned loop_bgn, unsigned loop_end) :
  audio_data(ad_ptr), 
  ichannel(channel), 
  findex_multiplier(factor), 
  iloop_bgn(loop_bgn), 
  iloop_end(loop_end), 
  findex(0), 
  findex_increment(1)
{
  if (loop_bgn >= loop_end)
  {
    isLooping = false;
  }
  else
  {
    isLooping = true;
  }
}

void Resample::setTime(float t)
{
  double totalSpeedup = findex_multiplier * findex_increment;

  findex = t * totalSpeedup * audio_data->rate();
}

float Resample::getValue(void)
{
  if (findex >= audio_data->frames() - 1)
  {
    return 0;
  }

  unsigned flooredValue = static_cast<unsigned>(findex);
  if (isLooping && (flooredValue > iloop_end))
  {
    flooredValue = iloop_end;
  }
  float temp1 = audio_data->sample(flooredValue, ichannel);
  //std::cout << "Floored Value: " << flooredValue << std::endl;
  
  // + 1 won't fly for looped values
  float temp2;
  if (isLooping && (findex >= iloop_end))
  {
    temp2 = audio_data->sample(iloop_bgn, ichannel);
  }
  else
  {
    temp2 = audio_data->sample(flooredValue + 1, ichannel);
  }
  

  float tempMultiplier = static_cast<float>(findex) - static_cast<float>(flooredValue);

  return temp1 + (tempMultiplier * (temp2 - temp1));
}

void Resample::incrementTime(void)
{
  findex += findex_increment * findex_multiplier;

  if (isLooping && (findex > iloop_end))
  {
    double offset = findex - static_cast<double>(iloop_end);
    findex = static_cast<double>(iloop_bgn) + offset;
  }
}

void Resample::offsetPitch(float cents)
{
  findex_increment = pow(2.0, static_cast<double>(cents) / 1200.0);
}
