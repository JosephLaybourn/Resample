// speed.cpp
// -- speed up effect using resampling
// jsh 2/17
//
// usage:
//   speed <file> <factor>
// where:
//   <file>   -- WAVE file (16 bit mono)
//   <factor> -- speed factor (< 1 for slower, > 1 for faster)
// output:
//   'speed.wav' (16 bit mono)

#include <fstream>
#include <cstdlib>
#include <cassert>
using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 3)
    return 0;

  float factor = atof(argv[2]);

  fstream in(argv[1],ios_base::in|ios_base::binary);
  char header[44];
  in.read(header,sizeof(header));
  unsigned data_in_size = *reinterpret_cast<unsigned*>(header+40),
           data_in_count = data_in_size/2;
  char *data_in = new char[data_in_size];
  in.read(data_in,data_in_size);

  fstream out("speed.wav",ios_base::binary|ios_base::out);
  unsigned data_out_count = unsigned((data_in_count-1)/factor)+1,
           data_out_size  = 2*data_out_count;
  *reinterpret_cast<unsigned*>(header+4) = 36U + data_out_size;
  *reinterpret_cast<unsigned*>(header+40) = data_out_size;
  out.write(header,44);

  char *data_out = new char[data_out_size];
  short *sample_in = reinterpret_cast<short*>(data_in),
        *sample_out = reinterpret_cast<short*>(data_out);
  for (unsigned j=0; j < data_out_count; ++j) {
    float s = j*factor;
    int k = int(s),
        kp1 = (k+1 < data_in_count) ? k+1 : k;
    sample_out[j] = short(sample_in[k] + (s-k)*(sample_in[kp1]-sample_in[k]));
  }

  out.write(data_out,data_out_size);
  delete[] data_in;
  delete[] data_out;
  return 0;
}
