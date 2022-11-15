// linear_interpolate.cpp
// -- resampling using linear resampling
// cs245 2/17
//
// usage:
//   linear_interpolate <file> <rate>
// where:
//   <file> -- WAVE file (16 mono)
//   <rate> -- target sampling rate
// output:
//   'linear_interpolate.wav'

#include <fstream>
#include <cmath>
#include <cstdlib>
using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 3)
    return 0;

  // read input audio data
  fstream in(argv[1],ios_base::binary|ios_base::in);
  char header[44];
  in.read(header,44);
  unsigned in_size = *reinterpret_cast<unsigned*>(header+40),
           in_count = in_size/2,
           in_rate = *reinterpret_cast<unsigned*>(header+24);
  short *in_samples = new short[in_count];
  in.read(reinterpret_cast<char*>(in_samples),in_size);

  // output audio data parameters
  unsigned out_rate = atoi(argv[2]);
  float alpha = float(in_rate)/float(out_rate);
  unsigned out_count = unsigned(floor((in_count-1)/alpha) + 1),
           out_size = 2*out_count;
  *reinterpret_cast<unsigned*>(header+4) = 36 + out_size;
  *reinterpret_cast<unsigned*>(header+24) = out_rate;
  *reinterpret_cast<unsigned*>(header+28) = 2*out_rate;
  *reinterpret_cast<unsigned*>(header+40) = out_size;

  // compute linear-interpolated output audio data
  short *out_samples = new short[out_count];
  for (int n=0; n < out_count; ++n) {
    float s = alpha*n;
    int k = int(s),
        kp1 = (k+1 < in_count) ? k+1 : k;
    out_samples[n] = short(in_samples[k] + (s-k)*(in_samples[kp1]-in_samples[k]));
  }

  fstream out("linear_interpolate.wav",ios_base::binary|ios_base::out);
  out.write(header,44);
  out.write(reinterpret_cast<char*>(out_samples),out_size);

  delete[] out_samples;
  delete[] in_samples;
  return 0;
}

